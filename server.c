#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include <inttypes.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <semaphore.h>
#include <poll.h>

#include <net-core/net_api.h>
#include <net-core/net_crypto.h>
#include <net-core/net_msg.h>
#include <btrees/btrb.h>
#include <heapq/heap.h>

#include "server_ctx.h"
#include "server_client.h"
#include "server_command.h"
#include "con_worker.h"
#include "work_horse.h"


#define KEY_SIZE 32


void disable_nagle(net_api_t *n)
{
        int flag = 1;
        int result = setsockopt(n->server_addr.sock, IPPROTO_TCP,
                        TCP_NODELAY, (char *)&flag, sizeof(int));
        if (result < 0) {
                printf("Error, could not disable nagle algorithm on server sock\n");
                fflush(stdout);
        }
        result = setsockopt(n->client_addr.sock, IPPROTO_TCP,
                            TCP_NODELAY, (char *)&flag, sizeof(int));
        if (result < 0) {
                printf("Error, could not disable nagle algorithm on client sock\n");
                fflush(stdout);
        }
}


/* four horses per default */
const int n_work_horses = 4;


server_ctx_t sct;


static int server_init(server_ctx_t *s)
{
        int res;

        /* Initialize TCP/IP server */
        res = net_server_init(&s->server, SOCK_TYPE_TCP, 0, 28760);
        if (res != NET_CORE_OK) {
                printf("Net server init failed\n");
                fflush(stdout);
                net_core_shutdown();
                return -1;
        }

        /* buffer up to 8192 commands */
        if (cmd_queue_init(&s->main_cmd_queue, 8192, cmd_queue_prioritizer)) {
                printf("Fatal: out of memory\n");
                net_core_shutdown();
                return -1;
        }

        /* Initialize client registry */
        client_tree_init(&s->clt);
        
        s->server.state.send_timeout.tv_sec = 1;
        s->server.state.recv_timeout.tv_sec = 1;

        s->server_running = true;
        return 0;
}


static void server_destroy(server_ctx_t *s)
{
        /* Free command queue */
        cmd_queue_destroy(&s->main_cmd_queue);

        /* Free client registry */
        client_tree_destroy(&s->clt);
}


int main(int argc, char **argv)
{
        pthread_t horses[n_work_horses];
        int res;

        if (net_core_init() != NET_CORE_OK) {
                printf("Problems with your stupid OS? :)\n");
                return -1;
        }

        if (server_init(&sct)) {
                return -1;
        }

        /* Create connection worker thread */
        pthread_t conwo_thread;
        res = pthread_create(&conwo_thread, NULL, connection_worker,
                             &sct);

        /* Create additional horse power */
        for (int i = 0; i < n_work_horses - 1; i++) {
                res = pthread_create(&horses[i], NULL, main_cmd_work_horse,
                                     &sct);
        }

        /* just call the work horse function once, so that main thread
           is converted into a workhorse itself */
        (void)main_cmd_work_horse(&sct);

        /* Connection worker finishes first */
        pthread_join(conwo_thread, NULL);

        /* Drop all non-handled commands from queue */
        cmd_queue_drop_all(&sct.main_cmd_queue);

        /* Wait for all additional work horses to exit */
        for (int i = 0; i < n_work_horses - 1; i++) {
                printf("Waiting for horse %d to return.\n", i);
                pthread_join(horses[i], NULL);
        }

        server_destroy(&sct);

        net_server_shutdown(&sct.server);

        return 0;
}

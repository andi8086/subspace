#include "con_worker.h"

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
#include "client_worker.h"


void cli_thread_joiner(btrb_node_t *cli_node)
{
        client_ctx_t *cli = (client_ctx_t *)cli_node->user_data;
        pthread_join(cli->thread, NULL);
}


void *connection_worker(void *p)
{
        server_ctx_t *s = (server_ctx_t *)p;
        client_tree_t *clt = &s->clt;
        uint64_t client_id = 0;
        do {
                client_ctx_t *cli;

                /* before we call await_cli, which would block, we
                   poll for read events on the socket so that we
                   can abort in case of shutdown */

                struct pollfd fd;
                fd.fd = s->server.server_addr.sock;
                fd.events = POLLIN;
                int res = poll(&fd, 1, CON_POLL_MAX_MS);
                if (res == 0) {
                        /* timeout waiting for connection, again */
                        continue;        
                }
                if (res == -1) {
                        /* server socket closed, this is an error */
                        printf("Error accepting on server socket\n");
                        fflush(stdout);
                        break;
                }

                /* connection now incoming, allocate a new client
                   tree node */
                client_ctx_t *cli_ctx = malloc(sizeof(client_ctx_t));
                if (!cli_ctx) {
                        perror("Fatal: Out of memory\n");
                        net_core_shutdown();
                        exit(-1);
                }
                cli_ctx->clt = clt; /* back reference to tree context */
                cli_ctx->cmdq = &s->main_cmd_queue;
                cli_ctx->server_running = &s->server_running;
                cli_ctx->cli_node = (btrb_node_t *)malloc(sizeof(btrb_node_t));
                cli_ctx->cli_id = ++client_id;
                if (!cli_ctx->cli_node) {
                        /* out of memory, terminate process */
                        perror("Fatal: Out of memory\n");
                        net_core_shutdown();
                        exit(-1);        
                }
                btrb_insert(&clt->root, cli_ctx->cli_id,
                            cli_ctx, cli_ctx->cli_node);  
                s->server.await_cli(&s->server, &cli_ctx->client);
                /* new client connected, start new thread */
                res = pthread_create(&cli_ctx->thread, NULL,
                                     client_worker, cli_ctx);
//                disable_nagle(&client);
        } while (s->server_running);

        /* joins threads of client nodes */
        pthread_mutex_lock(&clt->lock);
        btrb_iterate_in_order(clt->root, cli_thread_joiner);
        pthread_mutex_unlock(&clt->lock);

        return NULL;
}

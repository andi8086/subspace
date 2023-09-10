#include "client_worker.h"

#include <inttypes.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <string.h>

#include <net-core/net_api.h>
#include <net-core/net_msg.h>
#include "btrees/btrb.h"

#include "server_client.h"


const char *test_message = "Hello world!\n";


void *client_worker(void *p)
{
        client_ctx_t *ctx = (client_ctx_t *)p;        

        printf("Client connected\n");
        fflush(stdout);
        if (net_diffie_hellman_x25519(&ctx->client, &ctx->crypt_ctx)) {
                printf("Error in key exchange\n");
                goto error_exit;
        }
        net_send_single_crypto(&ctx->client, &ctx->crypt_ctx,
                               test_message,
                               strlen(test_message) + 1);
        bool connection_valid = true;
        do {
                uint8_t type;
                int res = net_recv_packet(
                                &ctx->client, NULL, NULL, &type);

                if (res == -1) {
                        /* connection lost */
                        connection_valid = false;
                        printf("Client connection lost.\n");
                        fflush(stdout);
                        continue;
                }
                if (res <= 0) {
                        /* garbage or no cmd received */
                        continue;
                }

                switch (process_cmd(&ctx->client, type)) {
                case PROCESS_CMD_QUIT:
                        printf("QUIT command received\n");
                        fflush(stdout);
                        connection_valid = false;
                        if (!*ctx->server_running) {
                                /* already shut down by another thread */
                                break;
                        }

                        main_cmd_t cmd;
                        cmd.prio = 1;
                        cmd.cmd = MAIN_CMD_QUIT; 
                        cmd.status = QCMD_PENDING;
                        cmd.cli_id = ctx->cli_id;

                        if (cmd_enqueue(ctx->cmdq, &cmd) < 0) {
                                printf("Client id=%"PRIu64" error, could not"
                                       " enqueue command.\n");
                                break;
                        }
                        sem_wait(&cmd.done);
                        if (cmd.status != QCMD_DONE) {
                                printf("Client id=%"PRIu64" error, command"
                                       " not processed.\n", ctx->cli_id);
                        }
                        break;
                default:
                        break;
                }

        } while (connection_valid);
error_exit:
        close(ctx->client.client_addr.sock);

        pthread_mutex_lock(&ctx->clt->lock);
        /* free this cli_node */
        uint64_t id = ctx->cli_id;
        btrb_delete(&ctx->clt->root, ctx->cli_node);
        free(ctx->cli_node);
        free(ctx);
        pthread_mutex_unlock(&ctx->clt->lock);

        printf("LOG: Client id=%"PRIu64": Disconnected, memory freed.\n", id);
        fflush(stdout);

        return NULL;
}


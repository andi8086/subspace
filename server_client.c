#include "server_client.h"

#include <inttypes.h>


void client_tree_init(client_tree_t *clt)
{
        pthread_mutex_init(&clt->lock, NULL);
}


void client_tree_destroy(client_tree_t *clt)
{
        while (clt->root != btrb_nil()) {
                btrb_node_t *tmp = clt->root;
                btrb_delete(&clt->root, clt->root);
                client_ctx_t *tmp_ctx = (client_ctx_t *)tmp->user_data;
                uint64_t id = tmp_ctx->cli_id;
               
                net_close_socket(tmp_ctx->client.client_addr.sock);

                free(tmp->user_data);
                free(tmp);

                printf("LOG: Disconnected and freed client id=%"PRIu64"\n",
                       id);
        }
}


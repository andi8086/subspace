#ifndef SERVER_CLIENT_H
#define SERVER_CLIENT_H

#include <stdint.h>
#include <pthread.h>

#include <btrees/btrb.h>
#include <net-core/net_crypto.h>

#include "server_command.h"


typedef struct {
        btrb_node_t *root;
        pthread_mutex_t lock;
} client_tree_t;


typedef struct {
        pthread_t thread;
        net_api_t client;
        uint64_t cli_id;
        btrb_node_t *cli_node;
        net_crypt_ctx_t crypt_ctx;
        client_tree_t *clt;
        bool *server_running;
        cmd_queue_t *cmdq;
} client_ctx_t;


void client_tree_init(client_tree_t *clt);
void client_tree_destroy(client_tree_t *clt);


#endif

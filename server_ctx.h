#ifndef SERVER_CTX_H
#define SERVER_CTX_H

#include <stdbool.h>
#include <net-core/net_api.h>

#include "server_client.h"
#include "server_command.h"


typedef struct {
        net_api_t server;
        client_tree_t clt;
        bool server_running;
        cmd_queue_t main_cmd_queue;
} server_ctx_t;

#endif

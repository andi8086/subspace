#include <stdio.h>
#include <stdbool.h>

#include <net-core/net_api.h>
#include <net-core/net_msg.h>
#include <net-core/net_crypto.h>

#include "server_command.h"

#define KEY_SIZE 32
uint8_t shared_secret[KEY_SIZE]; /* NOT a key!! */
uint8_t key1[KEY_SIZE], key2[KEY_SIZE];

net_api_t client;

dhprot_t header;

net_crypt_ctx_t crypt_ctx;

void disable_nagle(net_api_t *n)
{
        int flag = 1;
        int result = setsockopt(n->client_addr.sock, IPPROTO_TCP,
                                TCP_NODELAY, (char *)&flag, sizeof(int));
        if (result < 0) {
                printf("Error, could not disable nagle algorithm on client sock\n");
                fflush(stdout);
        }
}


int main(int argc, char **argv)
{
#ifdef _WIN32
                WSADATA wsaData;
                int iResult;
                iResult = WSAStartup(MAKEWORD(2,2), &wsaData);
                if (iResult != 0) {
                        printf("WinSock2 init failed\n");
                        return -1;
                }
#endif
        int res = net_client_init(&client, SOCK_TYPE_TCP,
                        ntohl(inet_addr("127.0.0.1")), 28760);

        if (res != NET_CORE_OK) {
#ifdef _WIN32
                WSACleanup();
#endif
                return -1;
        }

        client.state.recv_timeout.tv_sec = 10;

        res = client.connect(&client);
        if (res != NET_CORE_OK) {
                printf("Could not connect to server\n");
#ifdef _WIN32
                WSACleanup();
#endif
                return -1;
        }

        if (net_diffie_hellman_x25519(&client, &crypt_ctx)) {
                printf("Error in key exchange\n");
                goto error_exit;
        }
        //disable_nagle(&client);
        char test_message[14];
        net_recv_single_crypto(&client, &crypt_ctx, test_message,
                               14);

        printf("%s\n", test_message);

        net_send_packet(&client, NULL, 0, DHCMD_QUIT);
        /* wait for ack */
        uint8_t type; 
        res = net_recv_packet(&client, NULL, NULL, &type);
        if (res <= 0) {
                if (res == 0) {
                        printf("timeout\n");
                } else if (res == -1) {
                        printf("socket error\n");
                } else {
                        printf("packet error\n");
                }
#ifdef _WIN32
                WSACleanup();
#endif
                return -1;
        } 

        if (type != DHCMD_ACK) {
                printf("invalid ACK\n");
        }
error_exit:
        net_close_socket(client.server_addr.sock);

#ifdef _WIN32
        WSACleanup();

#endif
        return 0;
}

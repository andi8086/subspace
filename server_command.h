#ifndef SERVER_COMMAND_H
#define SERVER_COMMAND_H


#include <stdint.h>
#include <semaphore.h>

#include <net-core/net_api.h>
#include <heapq/heap.h>


#define QCMD_PENDING 0
#define QCMD_DONE    1
#define QCMD_DROPPED 2
#define QCMD_IGNORED 3

#define MAIN_CMD_NOP  0
#define MAIN_CMD_QUIT 1


typedef struct {
        uint64_t cmd;
        uint64_t cli_id;
        void *params;
        uint64_t status;
        sem_t done;
        int prio;
} main_cmd_t;


typedef enum {
        PROCESS_CMD_OK,
        PROCESS_CMD_QUIT
} process_cmd_t;


typedef enum {
        DHCMD_ID,
        DHCMD_QUIT,
        DHCMD_ACK
} dhcmd_t;


typedef struct {
        dhcmd_t cmd;
} dhprot_t;


typedef struct {
        heapq_t heap;
        sem_t sem_cmd_available;
        pthread_mutex_t lock;
} cmd_queue_t;


process_cmd_t process_cmd(net_api_t *n, dhcmd_t cmd);

void cmd_queue_drop_all(cmd_queue_t *q);
bool cmd_queue_prioritizer(void *a, void *b);

int cmd_queue_init(cmd_queue_t *queue, size_t queue_size,
                   heapq_is_lt_func_t cmpf);
int cmd_enqueue(cmd_queue_t *q, main_cmd_t *mc);
main_cmd_t *cmd_dequeue(cmd_queue_t *q);
void cmd_queue_destroy(cmd_queue_t *q);


#endif

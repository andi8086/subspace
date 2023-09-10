#include "server_command.h"

#include <pthread.h>
#include <stdlib.h>

#include <net-core/net_api.h>
#include <net-core/net_msg.h>


process_cmd_t process_cmd(net_api_t *n, dhcmd_t cmd)
{
        switch(cmd) {
        case DHCMD_QUIT:
                net_send_packet(n, NULL, 0, DHCMD_ACK);
                shutdown(n->client_addr.sock, SHUT_WR);
                return PROCESS_CMD_QUIT;
        default:
                return PROCESS_CMD_OK;
        }
        return PROCESS_CMD_OK;
}


void cmd_queue_drop_all(cmd_queue_t *q)
{
        pthread_mutex_lock(&q->lock);
        
        main_cmd_t *mc;

        while (mc = heapq_pop(&q->heap)) {
                mc->status = QCMD_DROPPED;
                sem_post(&mc->done);
        }

        pthread_mutex_unlock(&q->lock);
}


bool cmd_queue_prioritizer(void *a, void *b)
{
        main_cmd_t *cmd_a = (main_cmd_t *)a;
        main_cmd_t *cmd_b = (main_cmd_t *)b;

        return cmd_a->prio < cmd_b->prio;
}


int cmd_queue_init(cmd_queue_t *queue, size_t queue_size,
                   heapq_is_lt_func_t cmpf)
{
        if (!heapq_init(&queue->heap, queue_size, cmpf, NULL)) {
                return -1;
        }

        /* Only one thread at a time may access the cmd queue */
        pthread_mutex_init(&queue->lock, NULL);

        /* this signals all work horses that there is a command, first
           popped, first served */
        sem_init(&queue->sem_cmd_available, 0, 0);

        return 0;
}


int cmd_enqueue(cmd_queue_t *q, main_cmd_t *mc)
{
        pthread_mutex_lock(&q->lock);
        main_cmd_t *dropped = (main_cmd_t *)heapq_push(&q->heap, mc); 
        sem_init(&mc->done, 0, 0);
        pthread_mutex_unlock(&q->lock);
        if (dropped) {
                dropped->status = QCMD_DROPPED;
                sem_post(&dropped->done);
                return -1;
        }
        sem_post(&q->sem_cmd_available);
        return 0;
}


main_cmd_t *cmd_dequeue(cmd_queue_t *q)
{
        pthread_mutex_lock(&q->lock);
        main_cmd_t *cmd = (main_cmd_t *)heapq_pop(&q->heap);
        pthread_mutex_unlock(&q->lock);
        return cmd;
}
        

void cmd_queue_destroy(cmd_queue_t *q)
{
        free(q->heap.elements);
        sem_destroy(&q->sem_cmd_available);
}


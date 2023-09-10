#include "work_horse.h"

#include "server_ctx.h"
#include "server_command.h"


void *main_cmd_work_horse(void *p)
{
        server_ctx_t *s = (server_ctx_t *)p;
        cmd_queue_t *q = &s->main_cmd_queue;
        do {
                struct timespec timeout = {
                        .tv_sec = 0,
                        .tv_nsec = WH_CMDQ_TIMEOUT_NS
                };
                sem_timedwait(&q->sem_cmd_available, &timeout);
                main_cmd_t *cmd = cmd_dequeue(q);
                if (!cmd) {
                        continue;
                }
                if (cmd->status != QCMD_PENDING) {
                        printf("LOG: work horse is confused.\n");
                        continue;
                }

                switch (cmd->cmd) {
                case MAIN_CMD_QUIT:
                        s->server_running = false;
                        cmd->status = QCMD_DONE;
                        break;
                default:
                        cmd->status = QCMD_IGNORED;
                        break;
                }
                /* tell the cmd issure we are finished */
                sem_post(&cmd->done);
        } while (s->server_running);
}


#ifndef DHPROT_H
#define DHPROT_H

typedef enum {
        DHCMD_ID,
        DHCMD_QUIT,
        DHCMD_ACK
} dhcmd_t;


typedef struct {
        dhcmd_t cmd;
} dhprot_t;

#endif

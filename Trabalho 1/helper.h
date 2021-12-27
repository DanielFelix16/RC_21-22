#ifndef _HELPER__H_
#define _HELPER__H_

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <termios.h>
#include <stdio.h>
#include "frame_protocol.h"

struct data_buffer
{
    size_t size;
    unsigned int i;
    unsigned char buffer[MAX_DATA_BYTES];
};

int flag_rcv(unsigned char rcv);

int a_rcv(unsigned char rcv);

int control_rcv(unsigned char rcv, unsigned char control);

int bcc_rcv(unsigned char rcv, unsigned char control);

int rr_rcv(unsigned char rcv);

int c_rcv(unsigned char rcv);

void print_array(unsigned char arr[]);

#endif
#pragma once
#ifndef _FRAME_PROTOCOL__H_
#define _FRAME_PROTOCOL__H_

unsigned int curr_n;

#define C_SET           0x03
#define C_UA            0x07
#define C_DISC          0x0B
#define FLAG            0x7E
#define A_SND           0x03
#define A_RCV           0x01
#define BCC_I(n)        (A_SND ^ C_DATA(n))
#define C_DATA(n)       (n << 6)
#define RR(n)           ((n << 7) | 0x05)
#define REJ(n)          ((n << 7) | 0x01)

#define NEED_FIRST_FLAG 0
#define NEED_A          1
#define NEED_SET        2
#define NEED_UA         3
#define NEED_BCC        4
#define NEED_LAST_FLAG  5
#define NEED_C          6
#define NEED_BCC2       7
#define NEED_RR         8
#define READ_DATA       9
#define CHECK_BCC       10
#define NEED_DISC       11
#define NEED_CTRL       12

#define STUFFING_BYTE   0x20
#define ESCAPE_BYTE     0x7d

#define MAX_DATA_BYTES  65000

#endif
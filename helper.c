#include "helper.h"
#include "frame_protocol.h"

extern unsigned int curr_n;

int flag_rcv(unsigned char rcv)
{
    if (rcv == FLAG)
    {
        return 0;
    }
    return 1;
}

int a_rcv(unsigned char rcv)
{
    if (rcv == A_SND)
    {
        return 0;
    }
    return 1;
}

int control_rcv(unsigned char rcv, unsigned char control)
{
    if (rcv == control)
    {
        return 0;
    }
    return 1;
}

int bcc_rcv(unsigned char rcv, unsigned char control)
{
    if (rcv == (A_SND ^ control))
    {
        return 0;
    }
    return 1;
}

int bcci_rcv(unsigned char rcv)
{
    if (rcv == BCC_I(curr_n))
    {
        return 0;
    }
    return 1;
}

/*int bcc2_rcv(unsigned char rcv)
{

    if (rcv == BCC)
    {
        return 0;
    }

    return 1;
}*/

int rr_rcv(unsigned char rcv)
{
    if (rcv == RR(curr_n))
    {
        return 0;
    }

    else if (rcv == REJ(curr_n))
    {
        return 1;
    }

    return 1;
}

int c_rcv(unsigned char rcv)
{

    if (rcv == C_DATA(curr_n))
    {
        return 0;
    }

    return 1;
}

void print_array(unsigned char arr[])
{
    for (size_t i = 0; i < 15; i++)
    {
        printf("%x ", arr[i]);
    }
}
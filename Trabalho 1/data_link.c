#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <termios.h>
#include <stdio.h>
#include "data_link.h"
#include "sender.h"
#include "receiver.h"
#include "frame_protocol.h"

struct termios newtio, oldtio;
extern unsigned int curr_n;
extern unsigned int num_of_rej;

int llopen(char *port, enum port_type type)
{
    int fd;

    curr_n = 1;

    serial_port_setup(port, &fd);

    if (type == TRANSMITTER)
    {
        if (write_set(fd) == -1)
        {
            printf("Error sending SET\n");
            llclose(fd, TRANSMITTER);
            return -1;
        }

        if (nn_sender_sm(fd, C_SET) == -1)
        {
            printf("Error receiving SET UA\n");
            llclose(fd, TRANSMITTER);
            return -1;
        }
    }

    else if (type == RECEIVER)
    {
        if (nn_receiver_sm(fd, C_SET) == -1)
        {
            printf("Error receiving SET\n");
            llclose(fd, RECEIVER);
            return -1;
        }

        if (write_nn_ack(fd) == -1)
        {
            printf("Error sending SET UA\n");
            llclose(fd, RECEIVER);
            return -1;
        }

        num_of_rej = 0;
    }

    return fd;
}

int llclose(int fd, enum port_type type)
{

    if (type == TRANSMITTER)
    {
        if (write_disc(fd) == -1)
        {
            printf("Error ending connection\n");
            return -1;
        }

        if (nn_sender_sm(fd, C_DISC) == -1)
        {
            printf("Error ending connection\n");
            return -1;
        }

        if (write_ack_disc(fd) == -1)
        {
            printf("Error ending connection\n");
            return -1;
        }
    }

    else if (type == RECEIVER)
    {
        if (nn_receiver_sm(fd, C_DISC) == -1)
        {
            printf("Error ending connection\n");
            return -1;
        }

        if (write_disc_rcv(fd) == -1)
        {
            printf("Error ending connection\n");
            return -1;
        }

        if (nn_receiver_sm(fd, C_UA) == -1)
        {
            printf("Error ending connection\n");
            return -1;
        }
    }

    serial_port_close(fd);
    printf("Number of rejected frames: %d\n", num_of_rej);

    return 0;
}

int llwrite(int fd, char *buffer, int length)
{
    curr_n = 1 - curr_n;

    int res;
    res = send_i_frame(fd, buffer, length);

    if (data_ack_sm(fd, buffer, length) == -1)
    {
        llclose(fd, TRANSMITTER);
        return -1;
    }

    return res;
}

int llread(int fd, char *buffer)
{
    curr_n = 1 - curr_n;

    if (i_receiver_sm(fd, buffer) == -1)
    {
        llclose(fd, RECEIVER);
        return -1;
    }

    return MAX_DATA_BYTES;
}

void serial_port_setup(char *port, int *fd)
{

    *fd = open(port, O_RDWR | O_NOCTTY);
    if (*fd < 0)
    {
        perror(port);
        exit(-1);
    }

    if (tcgetattr(*fd, &oldtio) == -1)
    { /* save current port settings */
        perror("tcgetattr");
        exit(-1);
    }

    bzero(&newtio, sizeof(newtio));
    newtio.c_cflag = BAUDRATE | CS8 | CLOCAL | CREAD;
    newtio.c_iflag = IGNPAR;
    newtio.c_oflag = 0;

    /* set input mode (non-canonical, no echo,...) */
    newtio.c_lflag = 0;

    newtio.c_cc[VTIME] = 30; /* inter-character timer unused */
    newtio.c_cc[VMIN] = 0;   /* blocking read until 1 chars received */

    /* 
    VTIME e VMIN devem ser alterados de forma a proteger com um temporizador a 
    leitura do(s) pr�ximo(s) caracter(es)
  */

    tcflush(fd, TCIOFLUSH);
    sleep(1);
    if (tcsetattr(*fd, TCSANOW, &newtio) == -1)
    {
        perror("tcsetattr");
        exit(-1);
    }

    printf("New termios structure set\n");
}

void serial_port_close(int fd)
{
    sleep(1);
    if (tcsetattr(fd, TCSANOW, &oldtio) == -1)
    {
        perror("tcsetattr");
        exit(-1);
    }
}

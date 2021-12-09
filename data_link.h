#ifndef _DATA_LINK__H_
#define _DATA_LINK__H_

#define BAUDRATE B1200
#define _POSIX_SOURCE 1 /* POSIX compliant source */
#define FALSE 0
#define TRUE 1

enum port_type
{
    TRANSMITTER,
    RECEIVER
};

int llopen(char *port, enum port_type type);

int llclose(int fd, enum port_type type);

int llwrite(int fd, char *buffer, int length);

int llread(int fd, char *buffer);

void serial_port_setup(char *port, int *fd);

void serial_port_close(int fd);

#endif
#ifndef _S_FRAME_SENDER__H_
#define _S_FRAME_SENDER__H_

int nn_sender_sm(int fd, char sent);

int write_set(int fd);

int write_disc(int fd);

int write_ack_disc(int fd);

void stuffing(unsigned char *data, struct data_buffer *stuffed_data, size_t data_size);

int send_i_frame(int fd, unsigned char *data, size_t data_size);

int data_ack_sm(int fd, unsigned char *data, size_t data_size);

#endif
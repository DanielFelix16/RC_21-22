#ifndef _S_FRAME_RECEIVER__H_
#define _S_FRAME_RECEIVER__H_

unsigned int num_of_rej;

int nn_receiver_sm(int fd, char received);

int write_nn_ack(int fd);

int write_disc_rcv(int fd);

int i_receiver_sm(int fd, char *received_data);

void destuffing(unsigned char data[], struct data_buffer *destuffed_data, unsigned int data_size);

unsigned char calc_bcc(struct data_buffer *data);

int answer_data(int fd, unsigned char rr_or_rej);

#endif
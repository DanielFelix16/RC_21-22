#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <termios.h>
#include <stdio.h>
#include "frame_protocol.h"
#include "helper.h"
#include "receiver.h"

int nn_receiver_sm(int fd, char received)
{

    unsigned int curr_state = NEED_FIRST_FLAG;
    unsigned char frameBuffer[1];
    unsigned char bcc_control;

    while (1)
    {
        if (read(fd, frameBuffer, 1) == 0)
        {
            return -1;
        }

        switch (curr_state)
        {

        case NEED_FIRST_FLAG:
        {
            if (flag_rcv(frameBuffer[0]) == 0)
            {
                curr_state = NEED_A;
                //printf("first flag\n");
            }
            break;
        }

        case NEED_A:
        {
            if (received == C_UA)
            {
                if (frameBuffer[0] == A_RCV)
                {
                    curr_state = NEED_CTRL;
                    //printf("a disc\n");
                }

                else if (flag_rcv(frameBuffer[0]) == 0)
                {
                    curr_state = NEED_A;
                }

                else
                {
                    curr_state = NEED_FIRST_FLAG;
                }
            }

            else if (received != C_UA)
            {
                if (a_rcv(frameBuffer[0]) == 0)
                {
                    curr_state = NEED_CTRL;
                    //printf("a\n");
                }

                else if (flag_rcv(frameBuffer[0]) == 0)
                {
                    curr_state = NEED_A;
                }

                else
                {
                    curr_state = NEED_FIRST_FLAG;
                }
            }

            break;
        }

        case NEED_CTRL:
        {
            if (control_rcv(frameBuffer[0], received) == 0)
            {
                bcc_control = received;
                curr_state = NEED_BCC;
                //printf("%0x\n", received);
            }

            /*else if (control_rcv(frameBuffer[0], C_SET) == 0)
            {
                bcc_control = C_SET;
                curr_state = NEED_BCC;
                printf("set\n");
            }

            else if (control_rcv(frameBuffer[0], C_UA) == 0)
            {
                bcc_control = C_UA;
                curr_state = NEED_BCC;
                printf("disc ua\n");
            }*/

            else if (flag_rcv(frameBuffer[0]) == 0)
            {
                curr_state = NEED_A;
            }

            else
            {
                curr_state = NEED_FIRST_FLAG;
            }

            break;
        }

        case NEED_BCC:
        {
            if (received == C_UA)
            {
                if (frameBuffer[0] == (A_RCV ^ received))
                {
                    curr_state = NEED_LAST_FLAG;
                    //printf("bcc disc\n");
                }

                else if (flag_rcv(frameBuffer[0]) == 0)
                {
                    curr_state = NEED_A;
                }

                else
                {
                    curr_state = NEED_FIRST_FLAG;
                }
            }

            else if (received != C_UA)
            {
                if (bcc_rcv(frameBuffer[0], bcc_control) == 0)
                {
                    curr_state = NEED_LAST_FLAG;
                    //printf("bcc\n");
                }

                else if (flag_rcv(frameBuffer[0]) == 0)
                {
                    curr_state = NEED_A;
                }

                else
                {
                    curr_state = NEED_FIRST_FLAG;
                }
            }

            break;
        }

        case NEED_LAST_FLAG:
        {
            if (flag_rcv(frameBuffer[0]) == 0)
            {
                //printf("final flag\n");
                return 0;
            }

            else
            {
                curr_state = NEED_FIRST_FLAG;
            }

            break;
        }
        }
    }
}

/*
int s_receiver_sm(int fd)
{
    unsigned char frameBuffer[1];
    unsigned int curr_state = NEED_FIRST_FLAG;

    while (1)
    {
        read(fd, frameBuffer, 1);
        printf("read\n");

        switch (curr_state)
        {

        case NEED_FIRST_FLAG:
        {
            if (flag_rcv(frameBuffer[0]) == 0)
            {
                curr_state = NEED_A;
                printf("first flag\n");
            }
            break;
        }

        case NEED_A:
        {
            if (a_rcv(frameBuffer[0]) == 0)
            {
                curr_state = NEED_SET;
                printf("a\n");
            }

            else if (flag_rcv(frameBuffer[0]) == 0)
            {
                curr_state = NEED_A;
            }

            else
            {
                curr_state = NEED_FIRST_FLAG;
            }

            break;
        }

        case NEED_SET:
        {
            if (set_rcv(frameBuffer[0]) == 0)
            {
                curr_state = NEED_BCC;
                printf("set\n");
            }

            else if (flag_rcv(frameBuffer[0]) == 0)
            {
                curr_state = NEED_A;
            }

            else
            {
                curr_state = NEED_FIRST_FLAG;
            }

            break;
        }

        case NEED_BCC:
        {
            if (bcc_rcv(frameBuffer[0]) == 0)
            {
                curr_state = NEED_LAST_FLAG;
                printf("bcc\n");
            }

            else if (flag_rcv(frameBuffer[0]) == 0)
            {
                curr_state = NEED_A;
            }

            else
            {
                curr_state = NEED_FIRST_FLAG;
            }

            break;
        }

        case NEED_LAST_FLAG:
        {
            if (flag_rcv(frameBuffer[0]) == 0)
            {
                printf("last flag\n");
                return 0;
            }

            else
            {
                curr_state = NEED_FIRST_FLAG;
            }

            break;
        }
        }
    }

    return 1;
}*/

int write_nn_ack(int fd)
{
    int frame_res;
    unsigned char frameAckBuffer[5];

    frameAckBuffer[0] = FLAG;
    frameAckBuffer[1] = A_SND;
    frameAckBuffer[2] = C_UA;
    frameAckBuffer[3] = (A_SND ^ C_UA);
    frameAckBuffer[4] = FLAG;

    frame_res = write(fd, frameAckBuffer, 5);

    return frame_res;
}

int write_disc_rcv(int fd)
{
    int write_res;
    unsigned char frameBuffer[5];

    frameBuffer[0] = FLAG;
    frameBuffer[1] = A_RCV;
    frameBuffer[2] = C_DISC;
    frameBuffer[3] = (A_RCV ^ C_DISC);
    frameBuffer[4] = FLAG;

    write_res = write(fd, frameBuffer, 5);

    return write_res;
}

extern unsigned int curr_n;

int i_receiver_sm(int fd, char *received_data)
{
    unsigned char data[MAX_DATA_BYTES];
    unsigned char iFrameBuffer[1];
    unsigned int curr_state = NEED_FIRST_FLAG;
    unsigned char check_bcc = 0;
    unsigned int data_bytes_read = 0;

    while (1)
    {
        if (read(fd, iFrameBuffer, 1) == 0)
        {
            return -1;
        }

        switch (curr_state)
        {

        case NEED_FIRST_FLAG:
        {
            if (flag_rcv(iFrameBuffer[0]) == 0)
            {
                curr_state = NEED_A;
                //printf("first flag\n");
            }
            break;
        }

        case NEED_A:
        {
            if (a_rcv(iFrameBuffer[0]) == 0)
            {
                curr_state = NEED_C;
                //printf("a\n");
            }

            else if (flag_rcv(iFrameBuffer[0]) == 0)
            {
                curr_state = NEED_A;
            }

            else
            {
                curr_state = NEED_FIRST_FLAG;
            }

            break;
        }

        case NEED_C:
        {
            if (c_rcv(iFrameBuffer[0]) == 0)
            {
                curr_state = NEED_BCC;
                //printf("c\n");
            }

            else if (flag_rcv(iFrameBuffer[0]) == 0)
            {
                curr_state = NEED_A;
            }

            else
            {
                curr_state = NEED_FIRST_FLAG;
            }

            break;
        }

        case NEED_BCC:
        {
            if (bcci_rcv(iFrameBuffer[0]) == 0)
            {
                curr_state = READ_DATA;
                //printf("bcc\n");
            }

            else if (flag_rcv(iFrameBuffer[0]) == 0)
            {
                curr_state = NEED_A;
            }

            else
            {
                curr_state = NEED_FIRST_FLAG;
            }

            break;
        }

        case READ_DATA:
        {
            if (iFrameBuffer[0] != FLAG)
            {
                data[data_bytes_read] = iFrameBuffer[0];
                data_bytes_read++;
            }
            else if (iFrameBuffer[0] == FLAG)
            {
                struct data_buffer destuffed_data = {.size = 0, .i = 0};
                destuffing(data, &destuffed_data, data_bytes_read);
                //print_array(destuffed_data.buffer);
                if (calc_bcc(&destuffed_data) == 0)
                {
                    answer_data(fd, RR(curr_n));
                    printf("Received frame\n");
                    for (size_t i = 0; i < destuffed_data.size - 1; i++)
                    {
                        received_data[i] = destuffed_data.buffer[i];
                    }
                }
                else
                {
                    answer_data(fd, REJ(curr_n));
                    data_bytes_read = 0;
                    curr_state = NEED_FIRST_FLAG;
                    printf("Rejected frame\n");
                    break;
                }

                //printf("returned here\n");
                return 0;
            }
        }
        }
    }
}

void destuffing(unsigned char data[], struct data_buffer *destuffed_data, unsigned int data_size)
{

    for (unsigned int i = 0; i < data_size; i++)
    {
        if (data[i] == ESCAPE_BYTE)
        {
            if (data[i + 1] == (FLAG ^ STUFFING_BYTE))
            {
                destuffed_data->buffer[destuffed_data->i] = FLAG;
                destuffed_data->i++;
                destuffed_data->size++;
                i++;
            }

            else if (data[i + 1] == (ESCAPE_BYTE ^ STUFFING_BYTE))
            {
                destuffed_data->buffer[destuffed_data->i] = ESCAPE_BYTE;
                destuffed_data->i++;
                destuffed_data->size++;
                i++;
            }
        }

        else
        {
            destuffed_data->buffer[destuffed_data->i] = data[i];
            destuffed_data->i++;
            destuffed_data->size++;
        }
    }
}

unsigned char calc_bcc(struct data_buffer *data)
{
    unsigned char res = 0;
    for (size_t i = 0; i < data->size - 1; i++)
    {
        res = res ^ data->buffer[i];
    }

    if (res == data->buffer[data->size - 1])
    {
        return 0;
    }

    return 1;
}

int answer_data(int fd, unsigned char rr_or_rej)
{
    int frame_res;
    unsigned char answerBuffer[5];

    answerBuffer[0] = FLAG;
    answerBuffer[1] = A_SND;
    answerBuffer[2] = rr_or_rej;
    answerBuffer[3] = (A_SND ^ rr_or_rej);
    answerBuffer[4] = FLAG;

    frame_res = write(fd, answerBuffer, 5);

    return frame_res;
}

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <termios.h>
#include <stdio.h>
#include "helper.h"
#include "frame_protocol.h"
#include "sender.h"

int nn_sender_sm(int fd, char sent)
{

    unsigned int curr_state = NEED_FIRST_FLAG;
    int num_tries = 0;
    unsigned char frameAckBuffer[1];
    unsigned char bcc_control;

    while (1)
    {
        if (read(fd, frameAckBuffer, 1) == 0)
        {
            printf("Couldn't read buffer\n");
            num_tries += 1;
            if (num_tries == 4)
            {
                return -1;
            }

            if (sent == C_SET)
            {
                if (write_set(fd) == -1)
                {
                    printf("Failed to write set frame\n");
                    return -1;
                }
            }

            else if (sent == C_DISC)
            {
                if (write_disc(fd) == -1)
                {
                    printf("Failed to write disconnect frame\n");
                    return -1;
                }
            }

            curr_state = NEED_FIRST_FLAG;
            continue;
        }

        switch (curr_state)
        {

        case NEED_FIRST_FLAG:
        {
            if (flag_rcv(frameAckBuffer[0]) == 0)
            {
                curr_state = NEED_A;
            }
            break;
        }

        case NEED_A:
        {
            if (sent != C_DISC)
            {
                if (a_rcv(frameAckBuffer[0]) == 0)
                {
                    curr_state = NEED_CTRL;
                }

                else if (flag_rcv(frameAckBuffer[0]) == 0)
                {
                    curr_state = NEED_A;
                }

                else
                {
                    curr_state = NEED_FIRST_FLAG;
                }
            }

            else if (sent == C_DISC)
            {
                if (frameAckBuffer[0] == A_RCV)
                {
                    curr_state = NEED_CTRL;
                }

                else if (flag_rcv(frameAckBuffer[0]) == 0)
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
            if (control_rcv(frameAckBuffer[0], C_DISC) == 0)
            {
                bcc_control = C_DISC;
                curr_state = NEED_BCC;
            }

            else if (control_rcv(frameAckBuffer[0], C_UA) == 0)
            {
                bcc_control = C_UA;
                curr_state = NEED_BCC;
            }

            else if (flag_rcv(frameAckBuffer[0]) == 0)
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
            if (sent != C_DISC)
            {
                if (bcc_rcv(frameAckBuffer[0], bcc_control) == 0)
                {
                    curr_state = NEED_LAST_FLAG;
                }

                else if (flag_rcv(frameAckBuffer[0]) == 0)
                {
                    curr_state = NEED_A;
                }

                else
                {
                    curr_state = NEED_FIRST_FLAG;
                }
            }

            else if (sent == C_DISC)
            {
                if (frameAckBuffer[0] == (A_RCV ^ C_DISC))
                {
                    curr_state = NEED_LAST_FLAG;
                }

                else if (flag_rcv(frameAckBuffer[0]) == 0)
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
            if (flag_rcv(frameAckBuffer[0]) == 0)
            {
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

int write_set(int fd)
{
    int write_res;
    unsigned char frameBuffer[5];

    frameBuffer[0] = FLAG;
    frameBuffer[1] = A_SND;
    frameBuffer[2] = C_SET;
    frameBuffer[3] = (A_SND ^ C_SET);
    frameBuffer[4] = FLAG;

    write_res = write(fd, frameBuffer, 5);

    return write_res;
}

int write_disc(int fd)
{
    int write_res;
    unsigned char frameBuffer[5];

    frameBuffer[0] = FLAG;
    frameBuffer[1] = A_SND;
    frameBuffer[2] = C_DISC;
    frameBuffer[3] = (A_SND ^ C_DISC);
    frameBuffer[4] = FLAG;

    write_res = write(fd, frameBuffer, 5);

    return write_res;
}

int write_ack_disc(int fd)
{
    int frame_res;
    unsigned char frameAckBuffer[5];

    frameAckBuffer[0] = FLAG;
    frameAckBuffer[1] = A_RCV;
    frameAckBuffer[2] = C_UA;
    frameAckBuffer[3] = (A_RCV ^ C_UA);
    frameAckBuffer[4] = FLAG;

    frame_res = write(fd, frameAckBuffer, 5);

    return frame_res;
}

void stuffing(unsigned char *data, struct data_buffer *stuffed_data, size_t data_size)
{
    unsigned char bcc2 = 0;

    for (unsigned int i = 0; i < data_size; i++)
    {
        bcc2 = bcc2 ^ data[i];
        if (data[i] == FLAG)
        {
            stuffed_data->buffer[stuffed_data->i] = ESCAPE_BYTE;
            stuffed_data->i++;
            stuffed_data->size++;
            stuffed_data->buffer[stuffed_data->i] = (data[i] ^ STUFFING_BYTE);
            stuffed_data->i++;
            stuffed_data->size++;
        }

        else if (data[i] == ESCAPE_BYTE)
        {
            stuffed_data->buffer[stuffed_data->i] = ESCAPE_BYTE;
            stuffed_data->i++;
            stuffed_data->size++;
            stuffed_data->buffer[stuffed_data->i] = (data[i] ^ STUFFING_BYTE);
            stuffed_data->i++;
            stuffed_data->size++;
        }

        else
        {
            stuffed_data->buffer[stuffed_data->i] = data[i];
            stuffed_data->i++;
            stuffed_data->size++;
        }
    }

    if (bcc2 == FLAG)
    {
        stuffed_data->buffer[stuffed_data->i] = ESCAPE_BYTE;
        stuffed_data->i++;
        stuffed_data->size++;
        stuffed_data->buffer[stuffed_data->i] = (bcc2 ^ STUFFING_BYTE);
        stuffed_data->i++;
        stuffed_data->size++;
    }

    else if (bcc2 == ESCAPE_BYTE)
    {
        stuffed_data->buffer[stuffed_data->i] = ESCAPE_BYTE;
        stuffed_data->i++;
        stuffed_data->size++;
        stuffed_data->buffer[stuffed_data->i] = (bcc2 ^ STUFFING_BYTE);
        stuffed_data->i++;
        stuffed_data->size++;
    }

    else
    {
        stuffed_data->buffer[stuffed_data->i] = bcc2;
        stuffed_data->i++;
        stuffed_data->size++;
    }
}

int send_i_frame(int fd, unsigned char *data, size_t data_size)
{
    //usleep(1000000); // para simular atraso na propagação
    unsigned char i_frame[MAX_DATA_BYTES];
    struct data_buffer stuffed_data = {.size = 0, .i = 0};
    int write_res;

    if (((rand() % 10000) + 1) == 1)
    {
        i_frame[0] = 0x6e;
    }
    else
    {
        i_frame[0] = FLAG;
    }
    //i_frame[0] = FLAG;
    i_frame[1] = A_SND;
    i_frame[2] = C_DATA(curr_n);
    i_frame[3] = BCC_I(curr_n);
    stuffing(data, &stuffed_data, data_size);
    for (size_t i = 0; i < stuffed_data.size; i++)
    {
        i_frame[i + 4] = stuffed_data.buffer[i];
    }
    i_frame[stuffed_data.size + 4] = FLAG;

    write_res = write(fd, i_frame, stuffed_data.size + 5);

    return write_res;
}

int data_ack_sm(int fd, unsigned char *data, size_t data_size)
{
    unsigned int curr_state = NEED_FIRST_FLAG;
    int num_tries = 0;
    unsigned char frameAckBuffer[1];
    unsigned char control;
    unsigned int rr_or_rej;

    while (1)
    {
        if (read(fd, frameAckBuffer, 1) == 0)
        {
            num_tries += 1;
            if (num_tries == 4)
            {
                return -1;
            }
            if (send_i_frame(fd, data, data_size) == -1)
            {
                printf("Failed to write information frame\n");
                return -1;
            }
            curr_state = NEED_FIRST_FLAG;
            continue;
        }

        switch (curr_state)
        {

        case NEED_FIRST_FLAG:
        {
            if (flag_rcv(frameAckBuffer[0]) == 0)
            {
                curr_state = NEED_A;
            }
            break;
        }

        case NEED_A:
        {
            if (a_rcv(frameAckBuffer[0]) == 0)
            {
                curr_state = NEED_RR;
            }

            else if (flag_rcv(frameAckBuffer[0]) == 0)
            {
                curr_state = NEED_A;
            }

            else
            {
                curr_state = NEED_FIRST_FLAG;
            }

            break;
        }

        case NEED_RR:
        {
            rr_or_rej = rr_rcv(frameAckBuffer[0]);

            if (rr_or_rej == 0)
            {
                curr_state = NEED_BCC;
                control = RR(curr_n);
                printf("Frame was received sucessfully by receiver\n");
            }

            else if (rr_or_rej == 1)
            {
                curr_state = NEED_BCC;
                control = REJ(curr_n);
                printf("Frame was rejected by receiver\n");
                send_i_frame(fd, data, data_size);
                curr_state = NEED_FIRST_FLAG;
            }

            else if (flag_rcv(frameAckBuffer[0]) == 0)
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
            if (bcc_rcv(frameAckBuffer[0], control) == 0)
            {
                curr_state = NEED_LAST_FLAG;
            }

            else if (flag_rcv(frameAckBuffer[0]) == 0)
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
            if (flag_rcv(frameAckBuffer[0]) == 0)
            {
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

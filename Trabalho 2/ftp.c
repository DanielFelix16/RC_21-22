#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <unistd.h>
#include <netdb.h>
#include <string.h>
#include "ftp.h"
#include "helper.h"

static struct url_info info;

int open_socket(int *sockfd, struct hostent *h, struct sockaddr_in server_addr, int port)
{
    /*server address handling*/
    bzero((char *)&server_addr, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = inet_addr(info.ip); /*32 bit Internet address network byte ordered*/
    server_addr.sin_port = htons(port);               /*server TCP port must be network byte ordered */

    /*open a TCP socket*/
    if ((*sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        perror("socket()");
        return 1;
    }
    /*connect to the server*/
    if (connect(*sockfd,
                (struct sockaddr *)&server_addr,
                sizeof(server_addr)) < 0)
    {
        perror("connect()");
        return 1;
    }

    if (port == SERVER_PORT)
    {
        char not_needed[1024];

        if (read_answer_code(*sockfd, &not_needed) == '2')
        {
            return 0;
        }
    }

    return 0;
}

int close_socket(int sockfd)
{
    if (close(sockfd) < 0)
    {
        perror("close()");
        return 1;
    }

    return 0;
}

int get_ip(struct hostent *h)
{
    if (((h = gethostbyname(info.host)) == NULL))
    {
        herror("gethostbyname()");
        return 1;
    }

    printf("Host name  : %s\n", h->h_name);
    printf("IP Address : %s\n", inet_ntoa(*((struct in_addr *)h->h_addr)));

    info.ip = inet_ntoa(*((struct in_addr *)h->h_addr));

    return 0;
}

int parse_url_arg(char *url_arg)
{
    // allocate memory for all the info that'll be retrieved from the argument
    info.user = (char *)malloc(MAX_INFO_SIZE);
    info.password = (char *)malloc(MAX_INFO_SIZE);
    info.host = (char *)malloc(MAX_INFO_SIZE);
    info.url_path = (char *)malloc(MAX_INFO_SIZE);
    info.filename = (char *)malloc(MAX_INFO_SIZE);
    info.ip = (char *)malloc(MAX_IP_ADDRESS_SIZE);

    char url_ftp_part[6];
    unsigned int state = FTP_PART;
    unsigned int i = 0;
    unsigned int field_i = 0;
    unsigned int arg_length = strlen(url_arg);
    char ftp_part[] = "ftp://";

    for (unsigned int j = 0; j < 6; j++)
    {
        url_ftp_part[j] = url_arg[j];
    }

    while (1)
    {
        switch (state)
        {
        // the program's argument has to begin with "ftp://"
        case FTP_PART:
        {
            for (unsigned int k = 0; k < 6; k++)
            {
                if (url_ftp_part[k] != ftp_part[k])
                {
                    printf("Error writing 'ftp://'\n");
                    return 1;
                }
            }

            i += 6;

            // if there is no '@' in the argument, the user is logging in anonymously
            if (count_occurrences(url_arg, '@') == 0)
            {
                state = HOST_PART;
                strcpy(info.user, "anonymous");
                strcpy(info.password, "anonymous");
            }

            else
            {
                state = USER_PART;
            }

            break;
        }

        case USER_PART:
        {
            do
            {
                info.user[field_i] = url_arg[i];
                field_i++;
                i++;
            } while (url_arg[i] != ':');

            state = PW_PART;
            field_i = 0;
            i++;
            print_array(info.user);
            break;
        }

        case PW_PART:
        {
            do
            {
                info.password[field_i] = url_arg[i];
                field_i++;
                i++;
            } while (url_arg[i] != '@');

            state = HOST_PART;
            field_i = 0;
            i++;
            print_array(info.password);
            break;
        }

        case HOST_PART:
        {
            do
            {
                info.host[field_i] = url_arg[i];
                field_i++;
                i++;
            } while (url_arg[i] != '/');

            state = PATH_PART;
            field_i = 0;
            break;
        }

        case PATH_PART:
        {
            do
            {
                info.url_path[field_i] = url_arg[i];
                field_i++;
                i++;
            } while (i < arg_length);

            state = FILENAME_PARSING;
            printf("All information retrieved\n");
            break;
        }

        case FILENAME_PARSING:
        {
            get_filename();
            printf("Got name of the file to download\n");
            return 0;
        }
        }
    }
}

int write_cmd(int sockfd, char *cmd, char *cmd_param)
{
    size_t cmd_size = strlen(cmd);
    size_t cmd_param_size = strlen(cmd_param);

    char enter[1];
    enter[0] = '\n';

    if (write(sockfd, cmd, cmd_size) == -1)
    {
        return 1;
    }

    // not every command sent to the server has a parameter
    if (cmd_param_size != 0)
    {
        if (write(sockfd, cmd_param, cmd_param_size) == -1)
        {
            return 1;
        }
    }

    if (write(sockfd, &enter, 1) == -1)
    {
        return 1;
    }

    char full_answer[1024];

    while (1)
    {
        char first = read_answer_code(sockfd, &full_answer);

        switch (first)
        {
        case '1':
        {
            //read_answer_code(sockfd, &full_answer);
            break;
        }

        case '2':
        {
            printf("Command received\n");
            if (cmd_param_size == 0)
            {
                int psv_port = get_psv_port(sockfd, &full_answer);
                return psv_port;
            }
            return 0;
        }

        case '3':
        {
            return 0;
            break;
        }

        case '4':
        {
            printf("Command not received, trying again\n");
            if (write(sockfd, cmd, cmd_size) == -1)
            {
                return 1;
            }

            if (cmd_param_size != 0)
            {
                if (write(sockfd, cmd_param, cmd_param_size) == -1)
                {
                    return 1;
                }
            }

            if (write(sockfd, &enter, 1) == -1)
            {
                return 1;
            }
            break;
        }

        case '5':
        {
            printf("Command not received, connection will close\n");
            if (close_socket(sockfd) == 1)
            {
                printf("Error closing socket\n");
                exit(-1);
            }
            return 1;
        }
        }
    }

    return 0;
}

char read_answer_code(int sockfd, char *full_answer)
{
    while (1)
    {
        for (unsigned int i = 0; i < 1024; i++)
        {
            read(sockfd, &full_answer[i], 1);
            if (full_answer[i] == '\n')
            {
                break;
            }
        }

        if (full_answer[3] == ' ')
        {
            break;
        }

        memset(full_answer, 0, 1024);
    }

    return full_answer[0];
}

int login(int sockfd)
{
    if (write_cmd(sockfd, USER_CMD, info.user) == 1)
    {
        printf("Login failed\n");
        return 1;
    }

    printf("Sent username\n");

    if (write_cmd(sockfd, PW_CMD, info.password) == 1)
    {
        printf("Login failed\n");
        return 1;
    }

    printf("Sent password\n");

    return 0;
}

int quit(int sockfd)
{
    if (write_cmd(sockfd, QUIT_CMD, info.user) == 1)
    {
        printf("Failed to quit\n");
        return 1;
    }

    return 0;
}

int enter_psv_mode(int sockfd, int *new_port)
{
    *new_port = write_cmd(sockfd, PASV_CMD, "");

    if (*new_port == 1)
    {
        printf("Failed to enter passive mode\n");
        return 1;
    }

    return 0;
}

int get_psv_port(int sockfd, char *answer)
{
    int first_byte;
    int second_byte;
    int n_i;

    sscanf(answer, "227 Entering Passive Mode (%d, %d, %d, %d, %d, %d)", &n_i, &n_i, &n_i, &n_i, &first_byte, &second_byte);

    return (first_byte * 256) + second_byte;
}

void get_filename()
{
    unsigned int path_length = strlen(info.url_path);
    unsigned int i = 0;
    unsigned int j = 0;

    for (; i < path_length; i++)
    {
        if (info.url_path[i] == '/')
        {
            memset(info.filename, 0, MAX_INFO_SIZE);
            j = 0;
        }
        else
        {
            info.filename[j] = info.url_path[i];
            j++;
        }
    }
}

int download_file(int sockfd, int sockfd_file)
{
    char file_content[FILE_READING_SIZE];
    unsigned int size_read;

    if (write_cmd(sockfd, RETR_CMD, info.url_path) == 1)
    {
        printf("Error retrieving file\n");
        return 1;
    }

    FILE *fp = fopen(info.filename, "wb+");

    if (fp == NULL)
    {
        printf("Error creating new file\n");
        return 1;
    }

    do
    {
        size_read = read(sockfd_file, file_content, FILE_READING_SIZE);
        write_to_file(fp, &file_content, size_read);
    } while (size_read > 0);

    fclose(fp);

    return 0;
}

void write_to_file(FILE *fp, char *data, int data_size)
{
    fseek(fp, 0, SEEK_END);
    fwrite(data, sizeof(char), data_size, fp);
}

int main(int argc, char **argv)
{
    struct hostent *h;
    struct sockaddr_in server_addr;
    struct sockaddr_in server_addr_psv;
    int sockfd;
    int sockfd_psv;
    int psv_port;

    if (parse_url_arg(argv[1]) == 1)
    {
        printf("Error parsing argument\n");
        exit(-1);
    }

    printf("Parsed argument\n");

    if (get_ip(h) == 1)
    {
        printf("Error getting IP address\n");
        exit(-1);
    }

    printf("Got IP address\n");

    if (open_socket(&sockfd, h, server_addr, SERVER_PORT) == 1)
    {
        printf("Error opening socket\n");
        exit(-1);
    }

    printf("Opened socket\n");

    if (login(sockfd) == 1)
    {
        printf("Error logging in\n");
        exit(-1);
    }

    printf("Logged in\n");

    if (enter_psv_mode(sockfd, &psv_port) == 1)
    {
        printf("Error entering passive mode\n");
        exit(-1);
    }

    printf("Entered passive mode and got new port\n");

    if (open_socket(&sockfd_psv, h, server_addr_psv, psv_port) == 1)
    {
        printf("Error opening passive mode socket\n");
        exit(-1);
    }

    printf("Opened passive mode socket\n");

    if (download_file(sockfd, sockfd_psv) == 1)
    {
        printf("Error downloading file\n");
        exit(-1);
    }

    printf("Downloaded requested file\n");

    if (quit(sockfd) == -1)
    {
        printf("Error logging out\n");
        exit(-1);
    }

    printf("Logged out\n");

    if (close_socket(sockfd_psv) == 1)
    {
        printf("Error closing passive mode socket\n");
        exit(-1);
    }

    if (close_socket(sockfd) == 1)
    {
        printf("Error closing socket\n");
        exit(-1);
    }

    printf("Closed sockets\n");

    return 0;
}
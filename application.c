#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <termios.h>
#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include "application.h"
#include "data_link.h"

struct file file, new_file;

int build_start_end_package(unsigned char *package, char *filename, char *filesize, int filename_size, int filesize_size, int start_or_end)
{

    if (start_or_end == 0)
    {
        package[0] = 2;
    }

    else if (start_or_end == 1)
    {
        package[0] = 3;
    }
    package[1] = 0;
    package[2] = filesize_size;
    for (size_t i = 0; i < filesize_size; i++)
    {
        package[i + 3] = filesize[i];
    }
    package[filesize_size + 3] = 1;
    package[filesize_size + 4] = filename_size;
    for (size_t i = 0; i < filename_size; i++)
    {
        package[i + filesize_size + 5] = filename[i];
    }

    return 5 + filename_size + filesize_size;
}

int build_data_package(unsigned char *package, char *data, int data_size)
{
    package[0] = 1;
    //printf("data size changed?: %d\n", data_size);
    static char sequence_number = 0;
    package[1] = sequence_number;
    sequence_number++;
    sequence_number %= 255;

    unsigned int l2 = data_size / 256;
    unsigned int l1 = data_size % 256;

    //printf("l2: %d\n", l2);
    //printf("l1: %d\n", l1);

    package[2] = l2;
    package[3] = l1;

    for (size_t i = 0; i < data_size; i++)
    {
        package[i + 4] = data[i];
    }

    return data_size + 4;
}

int send_start_end_package(int fd, int filesize_size, int start_or_end)
{
    int package_max_size = 1 + 4 + strlen(file.name) + filesize_size;
    unsigned char package[package_max_size];

    //printf("got here before building package\n");

    int package_size = build_start_end_package(&package, file.name, file.size_char, strlen(file.name), filesize_size, start_or_end);

    //printf("start package size: %d\n", package_size);
    if (llwrite(fd, &package, package_size) == -1)
    {
        return -1;
    }

    return 0;
}

int send_data_package(int fd, char *data, int data_size)
{
    unsigned char package[data_size + 4];

    int package_size = build_data_package(&package, data, data_size);

    //printf("data package size: %d\n", package_size);
    if (llwrite(fd, &package, package_size) == -1)
    {
        return -1;
    }

    return 0;
}

int get_file_size(FILE *fp)
{
    int res;

    fseek(fp, 0L, SEEK_END);
    res = ftell(fp);
    fseek(fp, 0L, SEEK_SET);

    return res;
}

int get_package(int fd)
{
    unsigned char store_data[MAX_BYTES_PACKET];
    int store_data_size;

    store_data_size = llread(fd, &store_data);

    //printf("after llread\n");

    if (store_data[0] == 1)
    {
        unsigned char data[PACKAGE_SIZE];
        int data_size = extract_data_from_package(&data, &store_data);
        write_data_to_file(&data, data_size);
        printf("Wrote data received to file\n");
        return 0;
    }

    else if (store_data[0] == 2)
    {
        parse_file_info(&store_data, &new_file);
        new_file.fp = fopen(new_file.name, "wb+");
        printf("Opened file with info received\n");
        return 0;
    }

    else if (store_data[0] == 3)
    {
        //printf("before parse_file_info\n");
        parse_file_info(&store_data, &new_file);
        //printf("after parse_file_info\n");
        //check_file_size_on_end();
        fclose(new_file.fp);
        printf("Closed file\n");
        return 1;
    }

    return -1;
}

void parse_file_info(char *data, struct file *file_to_get_info)
{
    size_t curr_i = 0;
    int filesize_size = data[2];
    char size[filesize_size];

    for (; curr_i < filesize_size; curr_i++)
    {
        size[curr_i] = data[curr_i + 3];
    }

    file_to_get_info->size = atoi(size);

    curr_i = curr_i + 4;

    int filename_size = data[curr_i];
    //printf("filename size: %d", filename_size);
    file_to_get_info->name = (char *)malloc(filename_size + 1);
    file_to_get_info->name[filename_size] = '\0';

    for (size_t i = 0; i < filename_size; i++, curr_i++)
    {
        file_to_get_info->name[i] = data[curr_i + 1];
        //printf("name: %0x\n", file_to_get_info->name[i]);
    }
}

int read_file_send_data(int fd)
{

    char file_data[PACKAGE_SIZE];

    fseek(file.fp, 0, SEEK_SET);

    while (1)
    {
        int size_read = fread(&file_data, sizeof(char), PACKAGE_SIZE, file.fp);

        //printf("size_read: %d\n", size_read);

        if (size_read > 0)
        {
            if (send_data_package(fd, &file_data, size_read) == -1)
            {
                return -1;
            }
            printf("Sent data package\n");
        }

        if (feof(file.fp))
        {
            printf("Reached end of file\n");
            break;
        }
    }

    return 0;
}

int extract_data_from_package(unsigned char *data, unsigned char *package)
{
    int data_size = 256 * package[2] + package[3];

    //printf("data size: %d\n", data_size);
    //printf("package 2: %d\n", package[2]);
    //printf("package 3: %d\n", package[3]);

    for (size_t i = 0; i < data_size; i++)
    {
        data[i] = package[i + 4];
        //printf("data[%d]: %0x\n", i, data[i]);
    }

    return data_size;
}

void write_data_to_file(char *data, int data_size)
{
    fseek(new_file.fp, 0, SEEK_END);
    fwrite(data, sizeof(char), data_size, new_file.fp);
}

/*
void check_file_size_on_end()
{
    new_file.size = get_file_size(new_file.fp);
    printf("new file size: %d\n", new_file.size);
    printf("file size: %d\n", file.size);

    if (new_file.size == file.size)
    {
        printf("File has the same size as the one transferred\n");
    }

    else
    {
        printf("File doesn't have the same size as the one transferred\n");
    }
}*/

int main(int argc, char **argv)
{
    struct timeval tv;
    gettimeofday(&tv, NULL);
    unsigned long begin = 1000000 * tv.tv_sec + tv.tv_usec;

    srand(time(NULL));

    if ((argc < 3) ||
        ((strcmp("/dev/ttyS0", argv[1]) != 0) &&
         (strcmp("/dev/ttyS1", argv[1]) != 0) &&
         (strcmp("/dev/ttyS10", argv[1]) != 0) &&
         (strcmp("/dev/ttyS11", argv[1]) != 0)))
    {
        printf("Usage:\tnserial SerialPort\n\tex: nserial /dev/ttyS1\n");
        exit(1);
    }

    int fd;

    if (strcmp("write", argv[2]) == 0)
    {
        fd = llopen(argv[1], TRANSMITTER);

        if (fd == -1)
        {
            exit(-1);
        }

        //printf("Got here b4 check\n");

        if (argv[3] == NULL)
        {
            printf("No file name\n");
            exit(-1);
        }

        //printf("Got here b4 name\n");

        file.name = argv[3];

        file.fp = fopen(file.name, "rb");

        //printf("Got here\n");

        if (file.fp == NULL)
        {
            printf("Couldn't open file with given name\n");
            exit(-1);
        }

        printf("File to transfer opened succesfully\n");

        file.size = get_file_size(file.fp);

        //printf("Got file size\n");

        if (file.size == -1)
        {
            printf("Couldn't get file size\n");
            exit(-1);
        }

        int filesize_size = 0;
        int temp_size = file.size;

        //printf("file size: %0x", file.size);

        while (temp_size != 0)
        {
            temp_size /= 256;
            filesize_size += 1;
        }

        //printf("got here before for\n");

        file.size_char = (char *)malloc(filesize_size);

        for (int i = filesize_size - 1, j = 0; i >= 0; i--, j++)
        {
            file.size_char[j] = file.size >> (i * 8);
        }

        //printf("strlen filename size: %d\n", strlen(file.name));

        if (send_start_end_package(fd, filesize_size, 0) == -1)
        {
            exit(-1);
        }

        printf("Sent start package\n");

        if (read_file_send_data(fd) == -1)
        {
            printf("Failed in sending file data through serial port\n");
            exit(-1);
        }

        printf("Sent all data to receiver\n");

        if (send_start_end_package(fd, filesize_size, 1) == -1)
        {
            exit(-1);
        }

        printf("Sent end package\n");

        if (llclose(fd, TRANSMITTER) == -1)
        {
            exit(-1);
        }

        printf("Connection ended successfully\n");
    }

    else if (strcmp("read", argv[2]) == 0)
    {
        fd = llopen(argv[1], RECEIVER);

        //printf("got here 2\n");

        if (fd == -1)
        {
            exit(-1);
        }

        int stop_getting_package = 0;

        while (stop_getting_package == 0)
        {
            stop_getting_package = get_package(fd);

            if (stop_getting_package == -1)
            {
                printf("File transfer not successful\n");
                exit(-1);
            }
        }

        //printf("didnt llclose\n");

        if (llclose(fd, RECEIVER) == -1)
        {
            exit(-1);
        }

        //printf("llclosed\n");

        printf("Connection ended successfully\n");
    }

    gettimeofday(&tv, NULL);
    unsigned long end = 1000000 * tv.tv_sec + tv.tv_usec;

    printf("Time elapsed: %lu microseconds", (end - begin));

    return 0;
}
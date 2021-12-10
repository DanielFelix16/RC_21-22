#ifndef _APPLICATION__H_
#define _APPLICATION__H_

#define MAX_BYTES_PACKET 65000
#define PACKAGE_SIZE 512

struct file
{
    FILE *fp;
    int size;
    char *name;
    char *size_char;
};

int build_start_end_package(unsigned char *package, char *filename, char *filesize, int filename_size, int filesize_size, int start_or_end);

int build_data_package(unsigned char *package, char *data, int data_size);

int send_data_package(int fd, char *data, int data_size);

int get_file_size(FILE *fp);

int get_package(int fd);

void parse_file_info(char *data, struct file *file_to_get_info);

int read_file_send_data(int fd);

int extract_data_from_package(unsigned char *data, unsigned char *package);

void write_data_to_file(char *data, int data_size);

void check_file_size_on_end();

int main(int argc, char **argv);

#endif
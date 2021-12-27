#ifndef _FTP__H_
#define _FTP__H_

// General macros
#define MAX_INFO_SIZE 1000
#define MAX_IP_ADDRESS_SIZE 16
#define SERVER_PORT 21
#define FILE_READING_SIZE 512

// Macros for state machine that parses argument
#define FTP_PART 0
#define USER_PART 1
#define PW_PART 2
#define HOST_PART 3
#define PATH_PART 4
#define FILENAME_PARSING 5
#define URL_FTP_PART "ftp://"

// Communication macros
#define USER_CMD "user "
#define PW_CMD "pass "
#define QUIT_CMD "quit "
#define PASV_CMD "PASV"
#define RETR_CMD "retr "

struct url_info
{
    char *user;
    char *password;
    char *host;
    char *url_path;
    char *filename;
    char *ip;
};

int open_socket(int *sockfd, struct hostent *h, struct sockaddr_in server_addr, int port);

int close_socket(int sockfd);

int get_ip(struct hostent *h);

int parse_url_arg(char *url_arg);

int write_cmd(int sockfd, char *cmd, char *cmd_param);

char read_answer_code(int sockfd, char *full_answer);

int login(int sockfd);

int quit(int sockfd);

int enter_psv_mode(int sockfd, int *new_port);

int get_psv_port(int sockfd, char *answer);

void get_filename();

int download_file(int sockfd, int sockfd_file);

#endif
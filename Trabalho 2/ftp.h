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

// struct that saves important info needed throughout the program
struct url_info
{
    char *user;
    char *password;
    char *host;
    char *url_path;
    char *filename;
    char *ip;
};

/**
 * @brief Opens a TCP socket connected to the given server
 * 
 * @param sockfd socket's file descriptor
 * @param h host
 * @param server_addr address of the server
 * @param port TCP port
 * @return int 0 on success, 1 on failure 
 */
int open_socket(int *sockfd, struct hostent *h, struct sockaddr_in server_addr, int port);

/**
 * @brief Closes a TCP socket
 * 
 * @param sockfd socket's file descriptor
 * @return int 0 on success, 1 on failure
 */
int close_socket(int sockfd);

/**
 * @brief Gets a host's IP address
 * 
 * @param h host
 * @return int 0 on success, 1 on failure
 */
int get_ip(struct hostent *h);

/**
 * @brief Parses the URL given as the program's argument, to extract the relevant information
 * 
 * @param url_arg program's argument 
 * @return int 0 on success, 1 on failure
 */
int parse_url_arg(char *url_arg);

/**
 * @brief Writes a command to the TCP server through a socket, also interprets response read in read_answer_code()
 * 
 * @param sockfd socket's file descriptor
 * @param cmd command to send
 * @param cmd_param parameter of the command to send (if it needs any)
 * @return int 0 on success, 1 on failure
 */
int write_cmd(int sockfd, char *cmd, char *cmd_param);

/**
 * @brief Reads the TCP server's 3-digit answer code, that the server sends after it receives a command
 * 
 * @param sockfd socket's file descriptor
 * @param full_answer parameter to return the server's full answer 
 * @return char the first char of the 3-digit answer code
 */
char read_answer_code(int sockfd, char *full_answer);

/**
 * @brief Logs in to the TCP server using the credentials given by the user
 * 
 * @param sockfd socket's file descriptor
 * @return int 0 on success, 1 on failure
 */
int login(int sockfd);

/**
 * @brief Quits the TCP server
 * 
 * @param sockfd scoket's file descriptor
 * @return int 0 on success, 1 on failure
 */
int quit(int sockfd);

/**
 * @brief Enters passive mode on the TCP server
 * 
 * @param sockfd socket's file descriptor
 * @param new_port parameter to return the TCP port provided for passive mode
 * @return int 0 on success, 1 on failure
 */
int enter_psv_mode(int sockfd, int *new_port);

/**
 * @brief Parses the response the server provides after attempting to enter passive mode
 * 
 * @param sockfd socket's file descriptor
 * @param answer server's full answer to the passive mode command
 * @return int the new TCP port provided for passive mode
 */
int get_psv_port(int sockfd, char *answer);

/**
 * @brief Parses the path provided by the user in the program's argument to extract the name of the file to download
 * 
 */
void get_filename();

/**
 * @brief Downloads the requested file from the TCP server
 * 
 * @param sockfd socket's file descriptor
 * @param sockfd_file passive mode socket's file descriptor
 * @return int 0 on success, 1 on failure
 */
int download_file(int sockfd, int sockfd_file);

/**
 * @brief Appends data to a file
 * 
 * @param fp file pointer of the file to append the data to
 * @param data data to append to the file
 * @param data_size size of the data to append to the file
 */
void write_to_file(FILE *fp, char *data, int data_size);

#endif
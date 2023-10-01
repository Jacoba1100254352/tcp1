#ifndef TCP_CLIENT_H_
#define TCP_CLIENT_H_

#include <arpa/inet.h>
#include <errno.h>
#include <getopt.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#define TCP_CLIENT_BAD_SOCKET (-1)
#define TCP_CLIENT_DEFAULT_PORT "8080"
#define TCP_CLIENT_DEFAULT_HOST "localhost"
#define TCP_CLIENT_MAX_INPUT_SIZE 1024

/**
 * Contains all of the information needed to connect to the server and send it a message.
 */
typedef struct Config {
    char *port;
    char *host;
    char *action;
    char *message;
} Config;

/**
 * @brief Parses the commandline arguments and options given to the program.
 *
 * @param argc The amount of arguments provided to the program (provided by the main function)
 * @param argv The array of arguments provided to the program (provided by the main function)
 * @param config An empty Config struct that will be filled in by this function.
 * @return Returns a 1 on failure, 0 on success
 */
int tcp_client_parse_arguments(int argc, char *argv[], Config *config);

///////////////////////////////////////////////////////////////////////
/////////////////////// SOCKET RELATED FUNCTIONS //////////////////////
///////////////////////////////////////////////////////////////////////

/**
 * @brief Creates a TCP socket and connects it to the specified host and port.
 *
 * @param config A config struct with the necessary information.
 * @return Returns the socket file descriptor or -1 if an error occurs.
 */
int tcp_client_connect(Config config);

/**
 * @brief Creates and sends a request to the server using the socket and configuration.
 *
 * @param sockfd Socket file descriptor
 * @param config A config struct with the necessary information.
 * @return Returns a 1 on failure, 0 on success
 */
int tcp_client_send_request(int sockfd, Config config);

/**
 * @brief Receives the response from the server. The caller must provide an already allocated buffer.
 *
 * @param sockfd Socket file descriptor
 * @param buf An already allocated buffer to receive the response in
 * @param buf_size The size of the allocated buffer
 * @return Returns a 1 on failure, 0 on success
 */
int tcp_client_receive_response(int sockfd, char *buf, int buf_size);

/**
 * @brief Closes the given socket.
 *
 * @param sockfd Socket file descriptor
 * @return Returns a 1 on failure, 0 on success
 */
int tcp_client_close(int sockfd);

#endif

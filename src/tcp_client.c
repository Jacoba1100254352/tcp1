#include "tcp_client.h"
#include "log.h"
#include <string.h>
#include <unistd.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <getopt.h>

#define NUM_VALID_ACTIONS 5

extern int verbose_flag;  // External reference to the global verbose flag declared in main.c

static void printHelpOption(char *argv[]) {
    fprintf(stderr, "Usage: %s [--help] [-v] [-h HOST] [-p PORT] ACTION MESSAGE\n"
                    "\nArguments:\n"
                    "  ACTION   Must be uppercase, lowercase, reverse,\n"
                    "           shuffle, or random.\n"
                    "  MESSAGE  Message to send to the server\n"
                    "\nOptions:\n"
                    "\t--help\n"
                    "\t-v, --verbose\n"
                    "\t--host HOSTNAME, -h HOSTNAME\n"
                    "\t--port PORT, -p PORT\n", argv[0]);
}

// Parses the commandline arguments and options given to the program.
int tcp_client_parse_arguments(int argc, char *argv[], Config *config) {
    int opt;
    static struct option long_options[] = {
            {"help",    no_argument, 0,       0},
            {"host",    required_argument, 0, 'h'},
            {"port",    required_argument, 0, 'p'},
            {"verbose", no_argument, 0,       'v'},
            {0, 0, 0,                         0}
    };

    static char* validActions[NUM_VALID_ACTIONS] = {
            "uppercase",
            "lowercase",
            "reverse",
            "shuffle",
            "random"
    };

    // Loop over all the options
    int long_index = 0;
    while ((opt = getopt_long(argc, argv, "h:p:v", long_options, &long_index)) != -1) {
        switch (opt) {
            case 0: // --help
                printHelpOption(argv);
                exit(EXIT_SUCCESS);
            case 'h':  // --host or -h
                config->host = optarg;
                break;
            case 'p':  // --port or -p
            {
                long port;
                char *endptr;
                port = strtol(optarg, &endptr, 10);
                if (*endptr != '\0' || port <= 0 || port > 65535) {
                    log_error("Invalid port number provided. Port must be a number between 1 and 65535.");
                    return EXIT_FAILURE;
                }
                config->port = optarg;
            }
                break;
            case 'v':  // --verbose or -v
                verbose_flag = 1;
                break;
            default: // An unrecognized option
                log_error("Invalid arguments provided.");
                return EXIT_FAILURE;
        }
    }

    // Collect additional arguments that are not options
    if (optind < argc) {
        config->action = argv[optind++];
        int i = 0;
        for ( ; i < NUM_VALID_ACTIONS; i++)
            if (strcmp(config->action, validActions[i]) == 0) // Compare the strings
                break;

        if (i == NUM_VALID_ACTIONS) { // Not found in validActions
            log_error("Invalid Action provided");
            return EXIT_FAILURE;
        }

    }
    if (optind < argc) config->message = argv[optind++];

    // If the required arguments are not provided, log error and return 1
    if (config->action == NULL || config->message == NULL) {
        log_error("Required arguments not provided. Need ACTION and MESSAGE.");
        return EXIT_FAILURE;
    } else if (optind < argc) {
        log_error("Unknown argument provided");
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}


// Creates a TCP socket and connects it to the specified host and port.
int tcp_client_connect(Config config) {
    int sockfd;
    struct sockaddr_in server_address;
    struct hostent *server;

    if (verbose_flag)
        log_log(LOG_INFO, __FILE__, __LINE__, "Connecting to %s:%s", config.host, config.port);

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        log_log(LOG_ERROR, __FILE__, __LINE__, "Could not create socket");
        return TCP_CLIENT_BAD_SOCKET;
    }

    server = gethostbyname(config.host);
    if (server == NULL) {
        log_log(LOG_ERROR, __FILE__, __LINE__, "No such host");
        return TCP_CLIENT_BAD_SOCKET;
    }

    memset(&server_address, 0, sizeof(server_address));
    server_address.sin_family = AF_INET;
    memcpy(&server_address.sin_addr.s_addr, server->h_addr, server->h_length);
    server_address.sin_port = htons(atoi(config.port));

    if (connect(sockfd, (struct sockaddr *) &server_address, sizeof(server_address)) < 0) {
        log_log(LOG_ERROR, __FILE__, __LINE__, "Could not connect");
        return TCP_CLIENT_BAD_SOCKET;
    }

    if (verbose_flag)
        log_log(LOG_DEBUG, __FILE__, __LINE__, "Connected to server!");

    return sockfd;  // Return the socket descriptor
}

// Creates and sends a request to the server using the socket and configuration.
int tcp_client_send_request(int sockfd, Config config) {
    // Inform of sending status
    if (verbose_flag)
        log_log(LOG_DEBUG, __FILE__, __LINE__, "Sending: %s %lu %s", config.action, strlen(config.message), config.message);

    // Initialize payload
    char payload[TCP_CLIENT_MAX_INPUT_SIZE];

    // Print info
    sprintf(payload, "%s %ld %s", config.action, strlen(config.message), config.message);

    int payloadLength = (int)strlen(payload);
    ssize_t bytesSent, totalBytesSent = 0;

    // Send bytes to the server until done
    while (totalBytesSent < payloadLength) {
        bytesSent = send(sockfd, payload, payloadLength - totalBytesSent, 0);
        if (bytesSent == -1) {
            log_error("Sent failed");
            return EXIT_FAILURE;
        }
        totalBytesSent += bytesSent;
    }

    // Inform of bytes sent
    if (verbose_flag)
        log_log(LOG_DEBUG, __FILE__, __LINE__, "Bytes sent: %lu (%lu/%lu)", strlen(config.message), strlen(config.message), strlen(config.message));

    return EXIT_SUCCESS;
}

// Receives the response from the server. The caller must provide an already allocated buffer.
int tcp_client_receive_response(int sockfd, char *buf, int buf_size) {
    // Variable initialization
    int bytesReceived = 0;
    int totalBytesReceived = 0;

    // Fill the buffer with info from the server
    while (totalBytesReceived < buf_size) {
        // Check for errors
        if ((bytesReceived = (int)recv(sockfd, buf + totalBytesReceived, buf_size - totalBytesReceived, 0)) < 0) {
            log_error("receive failed");
            return EXIT_FAILURE;
        } else if (!bytesReceived) // The server closed the connection
            break;
        else totalBytesReceived += bytesReceived;
    }

    // Update buffer with end symbol
    buf[totalBytesReceived] = '\0';

    // Inform of bytes read
    if (verbose_flag)
        log_log(LOG_DEBUG, __FILE__, __LINE__, "Bytes read: %d", bytesReceived);

    return EXIT_SUCCESS;
}

// Closes the given socket.
int tcp_client_close(int sockfd) {
    if (close(sockfd) < 0) {
        log_error("Could not close socket");
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}

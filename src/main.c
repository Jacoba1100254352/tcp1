#include "tcp_client.h"
#include "log.h"

int verbose_flag = 0; // Global variable for the verbose flag

// Function to print usage instructions when --help is used or invalid arguments are provided
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

int main(int argc, char *argv[]) {
    // Set the default host and port to connect to
    Config config = {
            .host = TCP_CLIENT_DEFAULT_HOST,
            .port = TCP_CLIENT_DEFAULT_PORT
    };

    // Parse command-line arguments and configure the client accordingly
    if (tcp_client_parse_arguments(argc, argv, &config)) {
        printHelpOption(argv);
        exit(EXIT_FAILURE);  // Exit if argument parsing fails
    }

    // Establish a connection to the server
    int sockfd = tcp_client_connect(config);
    if (sockfd == TCP_CLIENT_BAD_SOCKET)
        exit(EXIT_FAILURE);  // Exit if connection fails

    // Log connection details if verbose flag is set
    if (verbose_flag)
        log_log(LOG_INFO, __FILE__, __LINE__, "Connected to %s:%s", config.host, config.port);

    // Send the request to the server
    if (tcp_client_send_request(sockfd, config)) {
        tcp_client_close(sockfd);  // Close the socket if sending fails
        exit(EXIT_FAILURE);  // Exit with a failure status if sending fails
    }

    // Log the sent message if verbose flag is set
    if (verbose_flag)
        log_log(LOG_DEBUG, __FILE__, __LINE__, "Message sent: %s", config.message);

    // Buffer to store the server response
    char buf[TCP_CLIENT_MAX_INPUT_SIZE];
    if (tcp_client_receive_response(sockfd, buf, sizeof(buf))) {
        tcp_client_close(sockfd);  // Close the socket if receiving fails
        exit(EXIT_FAILURE);  // Exit if receiving fails
    }

    // Log the received response if verbose flag is set
    if (verbose_flag)
        log_log(LOG_DEBUG, __FILE__, __LINE__, "Response received: %s", buf);

    printf("%s\n", buf);  // Print the received message

    // Close the connection
    if (tcp_client_close(sockfd))
        exit(EXIT_FAILURE);  // Exit if socket closing fails

    if (verbose_flag)
        log_log(LOG_DEBUG, __FILE__, __LINE__, "Connection closed.");

    exit(EXIT_SUCCESS);  // Exit successfully
}

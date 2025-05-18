#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

// #define directive
#define PORT 8080
#define REQUEST_BUFFER_SIZE 1024

// "in" part means "Internet" and it is for IPv4 (Internet Protocol Version 4)
typedef struct sockaddr_in sockaddr_in;
// it is for IPv6 (Internet Protocol Version 6)
typedef struct sockaddr_in6 sockaddr_in6;

int main(int argc, char *argv[])
{
    // A file descriptor is an integer that uniquely identifies an open file or socket in a process.
    int server_fd;
    int new_socket;
    sockaddr_in address;
    socklen_t addressLength = sizeof(sockaddr_in);
    char requestBuffer[REQUEST_BUFFER_SIZE] = {0};

    // create a socket file descriptor
    server_fd = socket(AF_INET, SOCK_STREAM, IPPROTO_IP);
    if (server_fd < 0)
    {
        perror("Socket failed");
        exit(EXIT_FAILURE);
    }

    int opt = 1;
    // set socket options
    int socket_options_config = setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    if (socket_options_config < 0)
    {
        perror("Socket options config failed.");
        exit(EXIT_FAILURE);
    }

    return 0;
}
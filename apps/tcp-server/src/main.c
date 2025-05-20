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
    char request_buffer[REQUEST_BUFFER_SIZE] = {0};

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

    // structure in C used to specify an endpoint address for IPv4.
    sockaddr_in address;
    socklen_t address_length = sizeof(sockaddr_in);
    // "sin" comes from the struct name "sockaddr_in"
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    // need to cast the pointer to the struct "sockaddr_in" to a pointer to the struct "sockaddr"
    // because the bind function expects a pointer to the generic struct "sockaddr"
    struct sockaddr *address_ptr = (struct sockaddr *)&address;
    int is_bound = bind(server_fd, address_ptr, sizeof(address));
    if (is_bound < 0)
    {
        perror("Bind failed");
        exit(EXIT_FAILURE);
    }

    // listen for incoming connections
    int is_listening = listen(server_fd, 3);
    if (is_listening < 0)
    {
        perror("Listen failed");
        exit(EXIT_FAILURE);
    }
    printf("Listening on port %d\n", PORT);
    // accept a connection
    int new_socket = accept(server_fd, address_ptr, &address_length);
    if (new_socket < 0)
    {
        perror("Accept failed");
        exit(EXIT_FAILURE);
    }
    printf("Connection accepted\n");
    // read data from the socket

    int bytes_read = read(new_socket, request_buffer,
                          // subtract 1 for the null
                          // terminator at the end
                          REQUEST_BUFFER_SIZE - 1);
    
    printf("Bytes read: %d\n", bytes_read);

    char* response = "Hello from server";
    send(new_socket, response, strlen(response), 0);
    printf("Response sent\n");

    // close the socket
    close(new_socket);
    close(server_fd);
    printf("Socket closed\n");

    return 0;
}
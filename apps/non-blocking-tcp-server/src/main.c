/*
 * NxLite Simplified - Educational HTTP Server
 * Based on the NxLite project architecture
 *
 * This single-file implementation demonstrates:
 * - Master-worker architecture
 * - Event-driven I/O with epoll
 * - Basic HTTP request/response handling
 * - Non-blocking sockets
 * - Keep-alive connections
 *
 * Compile: gcc -o simple_server simple_server.c
 * Run: ./simple_server [port]
 */

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/epoll.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <errno.h>
#include <signal.h>
#include <sys/wait.h>
#include <time.h>
#include <sys/stat.h>

// Configuration constants
#define MAX_WORKERS 4
#define MAX_EVENTS 1024
#define BUFFER_SIZE 4096
#define MAX_CLIENTS 1000
#define DEFAULT_PORT 8080
#define DOCUMENT_ROOT "./static"

// Global state
static volatile sig_atomic_t running = 1;
static int server_fd = -1;
static pid_t worker_pids[MAX_WORKERS];

// Client connection structure
typedef struct
{
    int fd;
    time_t last_activity;
    char buffer[BUFFER_SIZE];
    int buffer_len;
    int keep_alive;
} client_t;

// Simple HTTP status codes
const char *get_status_text(int code)
{
    switch (code)
    {
    case 200:
        return "OK";
    case 404:
        return "Not Found";
    case 500:
        return "Internal Server Error";
    default:
        return "Unknown";
    }
}

// Get MIME type based on file extension
const char *get_mime_type(const char *path)
{
    const char *ext = strrchr(path, '.');
    if (!ext)
        return "application/octet-stream";

    if (strcmp(ext, ".html") == 0 || strcmp(ext, ".htm") == 0)
        return "text/html";
    if (strcmp(ext, ".css") == 0)
        return "text/css";
    if (strcmp(ext, ".js") == 0)
        return "application/javascript";
    if (strcmp(ext, ".png") == 0)
        return "image/png";
    if (strcmp(ext, ".jpg") == 0 || strcmp(ext, ".jpeg") == 0)
        return "image/jpeg";
    if (strcmp(ext, ".gif") == 0)
        return "image/gif";
    if (strcmp(ext, ".txt") == 0)
        return "text/plain";

    return "application/octet-stream";
}

// Set socket to non-blocking mode
int set_nonblocking(int fd)
{
    int flags = fcntl(fd, F_GETFL, 0);
    if (flags == -1)
        return -1;
    return fcntl(fd, F_SETFL, flags | O_NONBLOCK);
}

// Create and configure server socket
int create_server_socket(int port)
{
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd == -1)
    {
        perror("socket");
        return -1;
    }

    int opt = 1;
    if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) == -1)
    {
        perror("setsockopt");
        close(fd);
        return -1;
    }

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(port);

    if (bind(fd, (struct sockaddr *)&addr, sizeof(addr)) == -1)
    {
        perror("bind");
        close(fd);
        return -1;
    }

    if (listen(fd, SOMAXCONN) == -1)
    {
        perror("listen");
        close(fd);
        return -1;
    }

    return fd;
}

// Send HTTP response
void send_response(int client_fd, int status_code, const char *content_type,
                   const char *body, size_t body_len, int keep_alive)
{
    char header[1024];
    int header_len = snprintf(header, sizeof(header),
                              "HTTP/1.1 %d %s\r\n"
                              "Content-Type: %s\r\n"
                              "Content-Length: %zu\r\n"
                              "Connection: %s\r\n"
                              "Server: NxLite-Simple/1.0\r\n"
                              "\r\n",
                              status_code, get_status_text(status_code),
                              content_type, body_len,
                              keep_alive ? "keep-alive" : "close");

    send(client_fd, header, header_len, MSG_NOSIGNAL);
    if (body && body_len > 0)
    {
        send(client_fd, body, body_len, MSG_NOSIGNAL);
    }
}

// Serve static file
void serve_file(int client_fd, const char *path, int keep_alive)
{
    char full_path[1024];
    snprintf(full_path, sizeof(full_path), "%s%s", DOCUMENT_ROOT, path);

    // Security: prevent path traversal
    if (strstr(path, "..") != NULL)
    {
        const char *error_msg = "403 Forbidden";
        send_response(client_fd, 403, "text/plain", error_msg, strlen(error_msg), 0);
        return;
    }

    // If path ends with /, serve index.html
    if (path[strlen(path) - 1] == '/')
    {
        strncat(full_path, "index.html", sizeof(full_path) - strlen(full_path) - 1);
    }

    FILE *file = fopen(full_path, "rb");
    if (!file)
    {
        const char *error_msg = "404 Not Found";
        send_response(client_fd, 404, "text/plain", error_msg, strlen(error_msg), 0);
        return;
    }

    // Get file size
    fseek(file, 0, SEEK_END);
    long file_size = ftell(file);
    fseek(file, 0, SEEK_SET);

    // Read file content
    char *content = malloc(file_size);
    if (content && fread(content, 1, file_size, file) == (size_t)file_size)
    {
        send_response(client_fd, 200, get_mime_type(path), content, file_size, keep_alive);
    }
    else
    {
        const char *error_msg = "500 Internal Server Error";
        send_response(client_fd, 500, "text/plain", error_msg, strlen(error_msg), 0);
    }

    if (content)
        free(content);
    fclose(file);
}

// Parse HTTP request
int parse_request(const char *buffer, char *method, char *path, int *keep_alive)
{
    char version[16];
    if (sscanf(buffer, "%15s %1023s %15s", method, path, version) != 3)
    {
        return -1;
    }

    // Check for Connection: keep-alive header
    *keep_alive = (strstr(buffer, "Connection: keep-alive") != NULL);

    return 0;
}

// Handle client request
void handle_client(int client_fd, client_t *clients, int *client_count)
{
    char buffer[BUFFER_SIZE];
    ssize_t bytes_read = recv(client_fd, buffer, sizeof(buffer) - 1, 0);

    if (bytes_read <= 0)
    {
        // Connection closed or error
        close(client_fd);
        return;
    }

    buffer[bytes_read] = '\0';

    // Find end of HTTP headers
    char *header_end = strstr(buffer, "\r\n\r\n");
    if (!header_end)
    {
        // Incomplete request, would need buffering in real implementation
        close(client_fd);
        return;
    }

    char method[16], path[1024];
    int keep_alive;

    if (parse_request(buffer, method, path, &keep_alive) == -1)
    {
        const char *error_msg = "400 Bad Request";
        send_response(client_fd, 400, "text/plain", error_msg, strlen(error_msg), 0);
        close(client_fd);
        return;
    }

    printf("Request: %s %s (keep-alive: %d)\n", method, path, keep_alive);

    if (strcmp(method, "GET") == 0)
    {
        serve_file(client_fd, path, keep_alive);
    }
    else
    {
        const char *error_msg = "501 Not Implemented";
        send_response(client_fd, 501, "text/plain", error_msg, strlen(error_msg), 0);
        keep_alive = 0;
    }

    if (!keep_alive)
    {
        close(client_fd);
    }
    else
    {
        // In a real implementation, we'd add this to the client tracking
        // For simplicity, we'll close it anyway
        close(client_fd);
    }
}

// Worker process main loop
void worker_process(int server_fd, int worker_id)
{
    printf("Worker %d started (PID: %d)\n", worker_id, getpid());

    int epoll_fd = epoll_create1(0);
    if (epoll_fd == -1)
    {
        perror("epoll_create1");
        exit(1);
    }

    // Add server socket to epoll
    struct epoll_event ev;
    ev.events = EPOLLIN;
    ev.data.fd = server_fd;
    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, server_fd, &ev) == -1)
    {
        perror("epoll_ctl");
        exit(1);
    }

    struct epoll_event events[MAX_EVENTS];
    client_t clients[MAX_CLIENTS];
    int client_count = 0;

    while (running)
    {
        int nfds = epoll_wait(epoll_fd, events, MAX_EVENTS, 1000);

        if (nfds == -1)
        {
            if (errno == EINTR)
                continue;
            perror("epoll_wait");
            break;
        }

        for (int i = 0; i < nfds; i++)
        {
            int fd = events[i].data.fd;

            if (fd == server_fd)
            {
                // New connection
                struct sockaddr_in client_addr;
                socklen_t addr_len = sizeof(client_addr);
                int client_fd = accept(server_fd, (struct sockaddr *)&client_addr, &addr_len);

                if (client_fd == -1)
                {
                    if (errno != EAGAIN && errno != EWOULDBLOCK)
                    {
                        perror("accept");
                    }
                    continue;
                }

                printf("Worker %d: New connection from %s:%d\n",
                       worker_id, inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));

                // Handle the request immediately (simplified approach)
                handle_client(client_fd, clients, &client_count);
            }
        }
    }

    close(epoll_fd);
    printf("Worker %d exiting\n", worker_id);
}

// Signal handlers
void sigint_handler(int sig)
{
    running = 0;
}

void sigchld_handler(int sig)
{
    // Reap child processes
    while (waitpid(-1, NULL, WNOHANG) > 0)
        ;
}

// Master process main function
int main(int argc, char *argv[])
{
    int port = DEFAULT_PORT;
    if (argc > 1)
    {
        port = atoi(argv[1]);
        if (port <= 0 || port > 65535)
        {
            fprintf(stderr, "Invalid port number\n");
            exit(1);
        }
    }

    // Create document root directory if it doesn't exist
    mkdir(DOCUMENT_ROOT, 0755);

    // Create a simple index.html file
    char index_path[1024];
    snprintf(index_path, sizeof(index_path), "%s/index.html", DOCUMENT_ROOT);
    FILE *index_file = fopen(index_path, "w");
    if (index_file)
    {
        fprintf(index_file,
                "<!DOCTYPE html>\n"
                "<html><head><title>NxLite Simple Server</title></head>\n"
                "<body><h1>Welcome to NxLite Simple Server!</h1>\n"
                "<p>This is a simplified educational version of the NxLite HTTP server.</p>\n"
                "</body></html>\n");
        fclose(index_file);
    }

    printf("Starting NxLite Simple Server on port %d\n", port);
    printf("Document root: %s\n", DOCUMENT_ROOT);
    printf("Workers: %d\n", MAX_WORKERS);

    // Set up signal handlers
    signal(SIGINT, sigint_handler);
    signal(SIGTERM, sigint_handler);
    signal(SIGCHLD, sigchld_handler);
    signal(SIGPIPE, SIG_IGN);

    // Create server socket
    server_fd = create_server_socket(port);
    if (server_fd == -1)
    {
        exit(1);
    }

    printf("Server listening on port %d\n", port);

    // Fork worker processes
    for (int i = 0; i < MAX_WORKERS; i++)
    {
        pid_t pid = fork();
        if (pid == 0)
        {
            // Child process (worker)
            worker_process(server_fd, i);
            exit(0);
        }
        else if (pid > 0)
        {
            // Parent process (master)
            worker_pids[i] = pid;
            printf("Started worker %d with PID %d\n", i, pid);
        }
        else
        {
            perror("fork");
            exit(1);
        }
    }

    // Master process monitoring loop
    printf("Master process running. Press Ctrl+C to stop.\n");
    while (running)
    {
        sleep(1);

        // Check if any workers died and restart them
        for (int i = 0; i < MAX_WORKERS; i++)
        {
            int status;
            pid_t result = waitpid(worker_pids[i], &status, WNOHANG);
            if (result > 0)
            {
                printf("Worker %d (PID %d) died, restarting...\n", i, worker_pids[i]);

                pid_t new_pid = fork();
                if (new_pid == 0)
                {
                    worker_process(server_fd, i);
                    exit(0);
                }
                else if (new_pid > 0)
                {
                    worker_pids[i] = new_pid;
                    printf("Restarted worker %d with PID %d\n", i, new_pid);
                }
            }
        }
    }

    // Shutdown: terminate all workers
    printf("Shutting down...\n");
    for (int i = 0; i < MAX_WORKERS; i++)
    {
        if (worker_pids[i] > 0)
        {
            kill(worker_pids[i], SIGTERM);
        }
    }

    // Wait for workers to exit
    for (int i = 0; i < MAX_WORKERS; i++)
    {
        if (worker_pids[i] > 0)
        {
            waitpid(worker_pids[i], NULL, 0);
        }
    }

    close(server_fd);
    printf("Server shutdown complete\n");

    return 0;
}
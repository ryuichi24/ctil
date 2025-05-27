#include <signal.h>
#include <stdio.h>
#include <stdlib.h>

#include <sys/stat.h> // mkdir
#include <unistd.h>   // for sleep, fork

#include <sys/socket.h> // socket functions
#include <netinet/in.h> // sockaddr_in

// general constants
#define MAX_PATH_LENGTH 1024

// server constants
#define DEFAULT_PORT 8080
#define DOCUMENT_ROOT "./static"
#define MAX_WORKERS 4

// Global state
static volatile sig_atomic_t running = 1;
static pid_t worker_pids[MAX_WORKERS];

void handle_sigint(int signal)
{
    printf("Received SIGINT, shutting down...\n");
    running = 0;
}

void handle_sigchld(int signal)
{
    printf("Received SIGCHLD\n");
}

int is_valid_port(int port)
{
    if (port <= 0 || port > 65535)
    {
        fprintf(stderr, "Invalid port number: %d\n", port);
        return 1;
    }

    return 0;
}

void worker_process(int server_fd, int worker_id)
{

    printf("Worker %d started with kqueue (PID: %d)\n", worker_id, getpid());
    while (running)
    {
        sleep(1); // Simulate work
        printf("Worker %d is running...\n", worker_id);
    }
}

int main(int argc, char *argv[])
{
    // set socket port
    int port = DEFAULT_PORT;

    // Check if a port number is provided as an argument
    // if so, convert it to an integer, and check if it is valid
    if (argc > 1)
    {
        // atoi literally means "ASCII to integer"
        // convert the first argument to an integer
        port = atoi(argv[1]);

        if (is_valid_port(port) != 0)
        {
            fprintf(stderr, "Invalid port number: %d\n", port);
            exit(EXIT_FAILURE);
        }
    }

    // create static folder if it does not exist
    mkdir(DOCUMENT_ROOT, 0755);

    /**
     * Create a simple index.html file in the static folder
     */
    char index_html_path[MAX_PATH_LENGTH];
    snprintf(index_html_path, sizeof(index_html_path), "%s/index.html", DOCUMENT_ROOT);
    FILE *index_html_file = fopen(index_html_path, "w");
    if (!index_html_file)
    {
        perror("Failed to create index.html");
        exit(EXIT_FAILURE);
    }
    fprintf(index_html_file,
            "<!DOCTYPE html>\n"
            "<html><head><title>C Server</title></head>\n"
            "<body><h1>Welcome to C Server!</h1>\n"
            "<p>Running Port: %d</p>\n"
            "</body></html>\n",
            port);
    fclose(index_html_file);

    /**
     * Set up signal handlers
     */
    signal(SIGINT, handle_sigint);   // Ctrl+c
    signal(SIGCHLD, handle_sigchld); // Child process termination

    /**
     * Create a server socket
     */
    // Create a socket file descriptor
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0)
    {
        perror("Failed to create server socket");
        exit(EXIT_FAILURE);
    }

    // Set socket options
    int opt = 1;
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0)
    {
        perror("Failed to set socket options");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    // Define the server address
    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;         // IPv4
    server_addr.sin_addr.s_addr = INADDR_ANY; // Accept connections from any address
    server_addr.sin_port = htons(port);       // Convert port to network byte order

    // Bind the socket to the address and port
    if (bind(server_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
    {
        perror("Failed to bind server socket");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    // Start listening for incoming connections
    if (listen(server_fd, SOMAXCONN) < 0)
    {
        perror("Failed to listen on server socket");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    /**
     * Fork worker processes
     */
    for (int i = 0; i < MAX_WORKERS; i++)
    {
        // 0 => Child process
        // > 0 => Parent process (master)
        pid_t pid = fork();

        if (pid == 0)
        {
            // Child process (worker)
            // start the worker process loop
            worker_process(server_fd, i);

            exit(EXIT_SUCCESS);
        }
        else if (pid > 0)
        {
            // Parent process (master)
            printf("Started worker %d with PID %d\n", i, pid);
            worker_pids[i] = pid; // Store worker PID
        }
        else
        {
            perror("Failed to fork worker process");
            close(server_fd);
            exit(EXIT_FAILURE);
        }
    }

    // Master process main loop
    while (running)
    {
        printf("Server is running...\n");
        sleep(1);
    }

    // Clean up: close the server socket file descriptor
    close(server_fd);
    printf("Server has shut down.\n");

    return EXIT_SUCCESS;
}

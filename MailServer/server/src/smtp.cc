#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <errno.h>
#include <ctype.h>
#include <pthread.h>
#include <string>
#include <algorithm>
#include <iostream>
#include <vector>
#include <signal.h>
#include "email.h"
#include <grpcpp/grpcpp.h>
#include "proto/coordinator.grpc.pb.h"
#include "Client.h"
#include <grpcpp/create_channel.h>
#include <grpcpp/client_context.h>
#include <iostream>
#include <string>
#include "ComposeService.h"
#include "GRPCCalls.h"

using namespace std;

using grpc::Channel;
using grpc::ClientContext;
using grpc::ServerBuilder;
using grpc::Status;

bool process_command(int client_fd, string &command, Email &email, string &user);
void *worker(void *arg);
void handle_shutdown(int signum);
string trim(string &str);
string InitializeServerAddress(string &user);

struct ThreadArgs
{
    int client_fd;
    string user;
    // string server_address;
};

// Vectors to store thread IDs and client socket file descriptors
vector<pthread_t> thread_ids;
vector<int> client_fds;

pthread_mutex_t vector_mutex = PTHREAD_MUTEX_INITIALIZER;
int listen_fd;
bool verbose = false;
string mail_dir;

void startServer(string server_address)
{
    ComposeService service;

    ServerBuilder builder;
    builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
    builder.RegisterService(&service);

    unique_ptr<grpc::Server> server(builder.BuildAndStart());
    cout << "gRPC Server is running on " << server_address << endl;
    server->Wait(); // Block until the server shuts down
}

int main(int argc, char *argv[])
{
    // Signal for Ctrl+C
    signal(SIGINT, handle_shutdown);

    // Default configurations
    int server_index = -1;
    string config_file;
    vector<string> server_addresses;

    if (argc != 3)
    {
        fprintf(stderr, "Usage: %s <config_file> <server_index>\n", argv[0]);
        return 1;
    }

    // Parse arguments
    config_file = argv[1];
    server_index = atoi(argv[2]);

    // Read server addresses from config file
    FILE *file = fopen(config_file.c_str(), "r");
    if (!file)
    {
        fprintf(stderr, "Failed to open configuration file: %s\n", config_file.c_str());
        return 1;
    }

    char buffer[256];
    while (fgets(buffer, sizeof(buffer), file))
    {
        string address(buffer);
        address = trim(address); // Remove any leading/trailing spaces or newline characters
        if (!address.empty())
        {
            server_addresses.push_back(address);
        }
    }
    fclose(file);

    // Validate server_index
    if (server_index < 0 || server_index >= server_addresses.size())
    {
        fprintf(stderr, "Invalid server_index. Must be between 0 and %lu\n", server_addresses.size() - 1);
        return 1;
    }

    // Determine server IP and port
    string server_address = server_addresses[server_index];
    size_t colon_pos = server_address.find(":");
    
    string ip = server_address.substr(0, colon_pos);
    int port = stoi(server_address.substr(colon_pos + 1));

    if (verbose)
    {
        printf("Starting SMTP server on %s:%d\n", ip.c_str(), port);
    }
    std::cout << "STARTED SERVER IN SMTP.CC" << server_address << std::endl;

    startServer(server_address);
    return 0;
}

void *worker(void *arg)
{
    ThreadArgs *args = (ThreadArgs *)arg;
    int client_fd = args->client_fd;
    string user = args->user;
    delete args;
    // string server_address = InitializeServerAddress(user);
    string server_address = "localhost:50053";

    // Sends greeting messsage
    const char *message = "220 localhost SMTP server is ready\r\n";
    int messageLength = strlen(message);

    if (write(client_fd, message, messageLength) < 0)
    { // Send bytes
        fprintf(stderr, "error sending greeting\n");
        exit(1);
    }

    if (verbose)
    {
        fprintf(stderr, "[%d] S: 220 localhost SMTP server is ready\n", client_fd); // Verbose: Server ready message
    }

    // Instantiates the Email object after the verbose log message
    Email email("", "", "");

    // email.displayEmailInfo();

    char read_buffer[2000];
    ssize_t bytes_read;

    // Clears the buffer before reading
    memset(read_buffer, 0, sizeof(read_buffer));
    string buffer;

    while (true)
    {
        // Reads data from the client socket
        bytes_read = read(client_fd, read_buffer, sizeof(read_buffer) - 1);
        if (bytes_read < 0)
        {
            fprintf(stderr, "read failed");
            break;
        }

        // Null terminates the buffer
        read_buffer[bytes_read] = '\0';

        // Appends the received data to the buffer
        buffer.append(read_buffer, bytes_read);

        // Processes all complete lines in the buffer
        size_t pos;
        while ((pos = buffer.find("\n")) != string::npos)
        {
            // Extracts the line (excluding the newline character)
            string line = buffer.substr(0, pos);

            // Removes carriage return if present (handles CRLF)
            if (!line.empty() && line.back() == '\r')
            {
                line.pop_back();
            }

            if (verbose)
            {
                fprintf(stderr, "[%d] C: %s\n", client_fd, line.c_str()); // Verbose: Command received
            }

            bool result = process_command(client_fd, line, email, server_address);
            // Removes the processed line from the buffer
            buffer.erase(0, pos + 1);

            if (!result)
            {
                if (verbose)
                {
                    fprintf(stderr, "[%d] Closing connection\n", client_fd); // Verbose: Connection closed
                }

                // Locks mutex before modifying the shared vectors
                pthread_mutex_lock(&vector_mutex);

                // Removes thread ID and client FD from the vectors
                auto it = find(client_fds.begin(), client_fds.end(), client_fd);
                if (it != client_fds.end())
                {
                    int index = distance(client_fds.begin(), it);
                    thread_ids.erase(thread_ids.begin() + index);
                    client_fds.erase(it);
                }

                // Unlocks mutex after modification
                pthread_mutex_unlock(&vector_mutex);

                close(client_fd);
                pthread_exit(NULL);
                break;
            }
        }
    }
    if (verbose)
    {
        fprintf(stderr, "[%d] Connection closed\n", client_fd); // Verbose: Connection closed
    }

    close(client_fd);
    pthread_exit(NULL);
}

bool process_command(int client_fd, string &command, Email &email, string &server_address)
{
    command = trim(command); // Trims the command

    // Finds the position of the first space to split the command and its argument
    size_t space_pos = command.find(' ');

    // Extracts the command (before the space) and the argument (after the space)
    string cmd = (space_pos != string::npos) ? command.substr(0, space_pos) : command;
    string argument = (space_pos != string::npos) ? command.substr(space_pos + 1) : "";

    // Converts the command part to uppercase for case insensitivity
    transform(cmd.begin(), cmd.end(), cmd.begin(), ::toupper);

    if (cmd == "HELO")
    {
        email.process_HELO(argument, client_fd);
        return true;
    }
    else if (cmd == "MAIL")
    {
        email.process_MAILFROM(argument, client_fd);
        return true;
    }
    else if (cmd == "RCPT")
    {
        email.process_RCPTTO(argument, client_fd, mail_dir);
        return true;
    }
    else if (cmd == "DATA")
    {
        email.process_DATA(client_fd, argument, mail_dir);
        return true;
    }
    else if (cmd == "RSET")
    {
        email.process_RSET(client_fd, argument);
        return true;
    }
    else if (cmd == "NOOP")
    {
        email.process_NOOP(client_fd, argument);
        return true;
    }
    else if (cmd == "QUIT" || cmd == "QUIT\r\n")
    {
        email.process_QUIT(client_fd, argument);
        return false;
    }
    else
    {
        // Handles unknown commands
        string response = "500 Syntax error, command unrecognized\r\n";
        if (write(client_fd, response.c_str(), response.length()) < 0)
        {
            fprintf(stderr, "Error sending unknown command response\n");
        }

        if (verbose)
        {
            fprintf(stderr, "[%d] S: 500 Syntax error, command unrecognized\n", client_fd); // Verbose: Server ready message
        }
        return true;
    }
}

// Signal handler for SIGINT (Ctrl+C)
void handle_shutdown(int signum)
{
    printf("\nReceived shutdown signal (Ctrl+C), shutting down server...\n");

    // Locks mutex before accessing shared vectors
    pthread_mutex_lock(&vector_mutex);

    // Iterates through the client socket file descriptors
    for (int client_fd : client_fds)
    {
        const char *shutdown_message = "-ERR Server shutting down\n";
        write(client_fd, shutdown_message, strlen(shutdown_message)); // Send shutdown message
        close(client_fd);                                             // Closes the client connection
    }

    // Closes the listening socket
    close(listen_fd);

    // Clears the vectors
    client_fds.clear();
    thread_ids.clear();

    // Unlocks mutex after modifications
    pthread_mutex_unlock(&vector_mutex);

    printf("Server shutdown complete.\n");
    exit(0); // Terminates the program
}

// Function to trim leading and trailing whitespaces
string trim(string &str)
{
    string result = str;

    // Removes leading whitespaces
    result.erase(result.begin(), find_if(result.begin(), result.end(), [](unsigned char ch) -> bool
                                         { return !isspace(ch); }));

    // Removes trailing whitespaces
    result.erase(find_if(result.rbegin(), result.rend(), [](unsigned char ch) -> bool
                         { return !isspace(ch); })
                     .base(),
                 result.end());

    return result;
}
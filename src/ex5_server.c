// Exercise 5: Fork with Timeout (Server)
// Builds on the multi-message loop from Exercise 3, but each child uses
// select() to wait at most 10 seconds for the next message. If the timeout
// expires with no data, the child closes the connection and exits.
// The client (ex1_client.c) is unchanged.
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/select.h>
#include <sys/socket.h>

#define PORT 8080
#define TIMEOUT_SECONDS 10

void handle_client(int client_sock) {
    char buffer[1024];
    fd_set read_fds;
    struct timeval timeout;

    while (1) {
        FD_ZERO(&read_fds);
        FD_SET(client_sock, &read_fds);
        timeout.tv_sec = TIMEOUT_SECONDS;
        timeout.tv_usec = 0;

        int ready = select(client_sock + 1, &read_fds, NULL, NULL, &timeout);

        if (ready == 0) {
            printf("Client timed out after %d seconds. Closing connection.\n", TIMEOUT_SECONDS);
            break;
        } else if (ready < 0) {
            perror("select");
            break;
        }

        memset(buffer, 0, sizeof(buffer));
        int n = read(client_sock, buffer, sizeof(buffer) - 1);
        if (n <= 0) {
            break; // client disconnected
        }

        printf("Received: %s\n", buffer);
        write(client_sock, "Hello from server", 17);
    }

    close(client_sock);
}

int main() {
    int server_sock, client_sock;
    struct sockaddr_in server_addr, client_addr;
    socklen_t addr_size;

    server_sock = socket(AF_INET, SOCK_STREAM, 0);

    int opt = 1;
    setsockopt(server_sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);

    bind(server_sock, (struct sockaddr*)&server_addr, sizeof(server_addr));
    listen(server_sock, 5);

    printf("Server listening on port %d...\n", PORT);

    while (1) {
        addr_size = sizeof(client_addr);
        client_sock = accept(server_sock, (struct sockaddr*)&client_addr, &addr_size);
        if (fork() == 0) {
            close(server_sock);
            handle_client(client_sock);
            exit(0);
        }
        close(client_sock);
    }
    return 0;
}

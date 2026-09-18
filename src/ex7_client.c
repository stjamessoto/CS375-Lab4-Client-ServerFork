// Exercise 7: Fork with Broadcast (Client)
// Uses select() to multiplex between keyboard input (to send) and the
// socket (to receive broadcasts from other clients), since both can arrive
// at any time and a single blocking read() would only ever watch one.
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/select.h>

#define PORT 8080

int main() {
    int sock;
    struct sockaddr_in server_addr;
    char buffer[1024];

    sock = socket(AF_INET, SOCK_STREAM, 0);
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    inet_pton(AF_INET, "127.0.0.1", &server_addr.sin_addr);

    connect(sock, (struct sockaddr*)&server_addr, sizeof(server_addr));
    printf("Connected. Type a message and press Enter to broadcast it. Ctrl+D to quit.\n");

    while (1) {
        fd_set read_fds;
        FD_ZERO(&read_fds);
        FD_SET(STDIN_FILENO, &read_fds);
        FD_SET(sock, &read_fds);
        int max_fd = (sock > STDIN_FILENO ? sock : STDIN_FILENO) + 1;

        if (select(max_fd, &read_fds, NULL, NULL, NULL) < 0) {
            perror("select");
            break;
        }

        if (FD_ISSET(STDIN_FILENO, &read_fds)) {
            if (fgets(buffer, sizeof(buffer), stdin) == NULL) {
                break; // Ctrl+D
            }
            buffer[strcspn(buffer, "\n")] = '\0';
            send(sock, buffer, strlen(buffer), 0);
        }

        if (FD_ISSET(sock, &read_fds)) {
            memset(buffer, 0, sizeof(buffer));
            int n = read(sock, buffer, sizeof(buffer) - 1);
            if (n <= 0) {
                printf("Server closed the connection.\n");
                break;
            }
            printf("Broadcast: %s\n", buffer);
        }
    }

    close(sock);
    return 0;
}

// Exercise 4: Fork with Client Counter (Server)
// Uses a System V shared memory segment so every forked child process can
// increment/decrement one shared counter of active clients. Compatible with
// ex2_client.cpp (protocol is unchanged: one message in, one reply out).
#include <iostream>
#include <unistd.h>
#include <netinet/in.h>
#include <cstring>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/wait.h>
#include <sys/socket.h>

#define PORT 8080

void handle_client(int client_sock, int *active_clients) {
    // Increment: a new client is now connected.
    (*active_clients)++;
    std::cout << "Client connected. Active clients: " << *active_clients << std::endl;

    char buffer[1024] = {0};
    read(client_sock, buffer, sizeof(buffer));
    std::cout << "Received: " << buffer << std::endl;
    write(client_sock, "Hello from C++ server", 22);
    close(client_sock);

    // Decrement: this client is disconnecting.
    (*active_clients)--;
}

int main() {
    int server_sock, client_sock;
    struct sockaddr_in server_addr, client_addr;
    socklen_t addr_size;

    // Create a shared memory segment for a single int counter, readable and
    // writable by the parent and every child it forks.
    int shm_id = shmget(IPC_PRIVATE, sizeof(int), IPC_CREAT | 0666);
    int *active_clients = (int *)shmat(shm_id, nullptr, 0);
    *active_clients = 0;

    server_sock = socket(AF_INET, SOCK_STREAM, 0);

    int opt = 1;
    setsockopt(server_sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);

    bind(server_sock, (struct sockaddr*)&server_addr, sizeof(server_addr));
    listen(server_sock, 5);

    std::cout << "Server listening on port " << PORT << "..." << std::endl;

    while (true) {
        addr_size = sizeof(client_addr);
        client_sock = accept(server_sock, (struct sockaddr*)&client_addr, &addr_size);

        pid_t pid = fork();
        if (pid == 0) {
            close(server_sock);
            handle_client(client_sock, active_clients);
            shmdt(active_clients);
            exit(0);
        }

        close(client_sock);
        // Reap any children that have already finished so they don't
        // accumulate as zombies.
        while (waitpid(-1, nullptr, WNOHANG) > 0) {}
    }

    shmdt(active_clients);
    shmctl(shm_id, IPC_RMID, nullptr);
    return 0;
}

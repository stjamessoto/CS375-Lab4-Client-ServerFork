// Exercise 7: Fork with Broadcast (Server)
//
// A shared-memory array of client socket descriptors lets every forked
// child look up who else is currently connected and forward a message to
// them. The parent process never closes an accepted client socket, so its
// descriptor table only ever grows; every child that is forked after a
// given client connects therefore inherits that client's fd as a valid,
// usable copy pointing at the same open file description.
//
// Known limitation: this is still fork()-per-client, so a child can only
// write to a client's socket fd if that fd already existed in the parent's
// table at the moment the child itself was forked. In practice this means
// a message from an OLDER client reaches every NEWER client, but a message
// from a NEWER client cannot reach an OLDER client's socket, because the
// older child was forked before that newer client's fd existed. A fully
// general broadcast server would instead hand off client sockets to a
// single process (e.g. via select()/epoll) or pass fds between processes
// with SCM_RIGHTS over a UNIX domain socket.
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <sys/wait.h>
#include <sys/socket.h>

#define PORT 8080
#define MAX_CLIENTS 32

static int sem_id;

void sem_lock(void) {
    struct sembuf op = {0, -1, 0};
    semop(sem_id, &op, 1);
}

void sem_unlock(void) {
    struct sembuf op = {0, 1, 0};
    semop(sem_id, &op, 1);
}

// Broadcast a message to every client socket in the shared table except
// the sender itself.
void broadcast(int *clients, int sender_sock, const char *msg, int len) {
    sem_lock();
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (clients[i] != -1 && clients[i] != sender_sock) {
            write(clients[i], msg, len);
        }
    }
    sem_unlock();
}

void handle_client(int client_sock, int *clients) {
    char buffer[1024];

    while (1) {
        memset(buffer, 0, sizeof(buffer));
        int n = read(client_sock, buffer, sizeof(buffer) - 1);
        if (n <= 0) {
            break; // client disconnected
        }

        printf("Received from fd %d: %s\n", client_sock, buffer);
        broadcast(clients, client_sock, buffer, n);
    }

    // Remove this client from the shared table.
    sem_lock();
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (clients[i] == client_sock) {
            clients[i] = -1;
            break;
        }
    }
    sem_unlock();

    close(client_sock);
}

int main() {
    int server_sock, client_sock;
    struct sockaddr_in server_addr, client_addr;
    socklen_t addr_size;

    // Shared memory: table of currently connected client socket fds.
    int shm_id = shmget(IPC_PRIVATE, MAX_CLIENTS * sizeof(int), IPC_CREAT | 0666);
    int *clients = (int *)shmat(shm_id, NULL, 0);
    for (int i = 0; i < MAX_CLIENTS; i++) {
        clients[i] = -1;
    }

    // Semaphore to serialize access to the shared client table.
    sem_id = semget(IPC_PRIVATE, 1, IPC_CREAT | 0666);
    semctl(sem_id, 0, SETVAL, 1);

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

        // Record the new client BEFORE forking, so the child (and every
        // later child) sees it in the shared table immediately.
        sem_lock();
        for (int i = 0; i < MAX_CLIENTS; i++) {
            if (clients[i] == -1) {
                clients[i] = client_sock;
                break;
            }
        }
        sem_unlock();

        if (fork() == 0) {
            close(server_sock);
            handle_client(client_sock, clients);
            exit(0);
        }

        while (waitpid(-1, NULL, WNOHANG) > 0) {}
    }

    shmdt(clients);
    shmctl(shm_id, IPC_RMID, NULL);
    semctl(sem_id, 0, IPC_RMID);
    return 0;
}

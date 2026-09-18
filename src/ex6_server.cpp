// Exercise 6: Fork with Error Handling (Server)
// Every socket call and fork() is checked; failures are logged with a
// timestamp to server_errors.log instead of printed to the console. A
// failure inside one child (or a failed fork/accept) never brings down the
// server's accept loop. The client (ex2_client.cpp) is unchanged.
#include <iostream>
#include <fstream>
#include <unistd.h>
#include <netinet/in.h>
#include <cstring>
#include <ctime>
#include <sys/wait.h>
#include <sys/socket.h>

#define PORT 8080
#define LOG_FILE "server_errors.log"

void log_error(const std::string &message) {
    std::ofstream log(LOG_FILE, std::ios::app);
    if (!log.is_open()) {
        return; // nothing more we can do if even the log can't be opened
    }
    time_t now = time(nullptr);
    char timestamp[64];
    strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", localtime(&now));
    log << "[" << timestamp << "] " << message << " (errno " << errno
        << ": " << strerror(errno) << ")" << std::endl;
}

void handle_client(int client_sock) {
    char buffer[1024] = {0};
    if (read(client_sock, buffer, sizeof(buffer)) < 0) {
        log_error("read() failed");
        close(client_sock);
        return;
    }
    std::cout << "Received: " << buffer << std::endl;

    if (write(client_sock, "Hello from C++ server", 22) < 0) {
        log_error("write() failed");
    }
    close(client_sock);
}

int main() {
    int server_sock, client_sock;
    struct sockaddr_in server_addr, client_addr;
    socklen_t addr_size;

    server_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (server_sock < 0) {
        log_error("socket() failed");
        return 1; // fatal: server cannot run without a socket
    }

    int opt = 1;
    setsockopt(server_sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);

    if (bind(server_sock, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        log_error("bind() failed");
        close(server_sock);
        return 1; // fatal: can't listen on the port
    }

    if (listen(server_sock, 5) < 0) {
        log_error("listen() failed");
        close(server_sock);
        return 1; // fatal
    }

    std::cout << "Server listening on port " << PORT << "..." << std::endl;

    while (true) {
        addr_size = sizeof(client_addr);
        client_sock = accept(server_sock, (struct sockaddr*)&client_addr, &addr_size);
        if (client_sock < 0) {
            log_error("accept() failed");
            continue; // transient; keep serving other clients
        }

        pid_t pid = fork();
        if (pid < 0) {
            log_error("fork() failed");
            close(client_sock);
            continue; // transient; keep serving other clients
        }

        if (pid == 0) {
            close(server_sock);
            handle_client(client_sock);
            exit(0);
        }

        close(client_sock);
        while (waitpid(-1, nullptr, WNOHANG) > 0) {}
    }

    close(server_sock);
    return 0;
}

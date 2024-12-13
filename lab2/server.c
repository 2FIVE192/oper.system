#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/select.h>

#define PORT 8080
#define BACKLOG 10
#define BUFFER_SIZE 1024

volatile sig_atomic_t was_sighup = 0;

void handle_sighup(int sig) {
    was_sighup = 1;
}

int setup_signal_handling(sigset_t *orig_mask) {
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = handle_sighup;
    sa.sa_flags = SA_RESTART;
    sigemptyset(&sa.sa_mask);

    if (sigaction(SIGHUP, &sa, NULL) == -1) {
        perror("sigaction");
        return -1;
    }

    sigset_t blocked_mask;
    sigemptyset(&blocked_mask);
    sigaddset(&blocked_mask, SIGHUP);

    if (sigprocmask(SIG_BLOCK, &blocked_mask, orig_mask) == -1) {
        perror("sigprocmask");
        return -1;
    }

    return 0;
}

int setup_server_socket() {
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == -1) {
        perror("socket");
        return -1;
    }

    int opt = 1;
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) == -1) {
        perror("setsockopt");
        close(server_fd);
        return -1;
    }

    struct sockaddr_in server_addr = {
        .sin_family = AF_INET,
        .sin_port = htons(PORT),
        .sin_addr.s_addr = INADDR_ANY
    };

    if (bind(server_fd, (struct sockaddr*)&server_addr, sizeof(server_addr)) == -1) {
        perror("bind");
        close(server_fd);
        return -1;
    }

    if (listen(server_fd, BACKLOG) == -1) {
        perror("listen");
        close(server_fd);
        return -1;
    }

    return server_fd;
}

int main() {
    sigset_t orig_mask;
    if (setup_signal_handling(&orig_mask) == -1) {
        exit(EXIT_FAILURE);
    }

    int server_fd = setup_server_socket();
    if (server_fd == -1) {
        exit(EXIT_FAILURE);
    }

    printf("Server listening on port %d...\n", PORT);

    int client_fd = -1;
    fd_set read_fds;
    int signal_or_connection_count = 0;

    while (signal_or_connection_count < 3) {
        FD_ZERO(&read_fds);
        FD_SET(server_fd, &read_fds);
        if (client_fd != -1) {
            FD_SET(client_fd, &read_fds);
        }

        int max_fd = (client_fd > server_fd ? client_fd : server_fd);

        if (pselect(max_fd + 1, &read_fds, NULL, NULL, NULL, &orig_mask) == -1) {
            if (errno == EINTR) {
                if (was_sighup) {
                    printf("SIGHUP received.\n");
                    was_sighup = 0;
                    signal_or_connection_count++;
                }
                continue;
            } else {
                perror("pselect");
                break;
            }
        }

        if (FD_ISSET(server_fd, &read_fds)) {
            struct sockaddr_in client_addr;
            socklen_t client_len = sizeof(client_addr);
            int new_client_fd = accept(server_fd, (struct sockaddr*)&client_addr, &client_len);
            if (new_client_fd == -1) {
                perror("accept");
                continue;
            }

            printf("New connection from %s:%d\n",
                   inet_ntoa(client_addr.sin_addr),
                   ntohs(client_addr.sin_port));

            if (client_fd == -1) {
                client_fd = new_client_fd;
            } else {
                printf("Connection immediately closed\n");
                close(new_client_fd);
            }
        }

        if (client_fd != -1 && FD_ISSET(client_fd, &read_fds)) {
            char buffer[BUFFER_SIZE];
            ssize_t bytes_read = read(client_fd, buffer, sizeof(buffer));
            if (bytes_read > 0) {
                printf("Received %zd bytes\n", bytes_read);
            } else if (bytes_read == 0 || (bytes_read == -1 && errno != EINTR)) {
                printf("Client disconnected\n");
                close(client_fd);
                client_fd = -1;
                signal_or_connection_count++;
            }
        }
    }

    if (sigprocmask(SIG_SETMASK, &orig_mask, NULL) == -1) {
        perror("sigprocmask");
    }

    close(server_fd);
    return 0;
}

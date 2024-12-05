#include <iostream>
#include <csignal>
#include <cstring>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <fcntl.h>
#include <sys/select.h>
#include <sys/types.h>

using namespace std;
constexpr int PORT = 8080;

// Глобальная переменная для управления завершением работы
volatile bool running = true;

// Обработчик сигналов
void signal_handler(int sig) {
    if (sig == SIGINT) {
        running = false;
    }
}

int main() {
    // Установка обработчика сигнала SIGINT
    struct sigaction sa{};
    sa.sa_handler = signal_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    if (sigaction(SIGINT, &sa, nullptr) == -1) {
        perror("sigaction");
        return 1;
    }

    // Создание серверного сокета
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == -1) {
        perror("socket");
        return 1;
    }

    int opt = 1;
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) == -1) {
        perror("setsockopt");
        close(server_fd);
        return 1;
    }

    struct sockaddr_in address{};
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) == -1) {
        perror("bind");
        close(server_fd);
        return 1;
    }

    if (listen(server_fd, 5) == -1) {
        perror("listen");
        close(server_fd);
        return 1;
    }

    std::cout << "Server is listening on port " << PORT << std::endl;

    fd_set read_fds;
    int accepted_fd = -1;

    while (running) {
        FD_ZERO(&read_fds);
        FD_SET(server_fd, &read_fds);

        int max_fd = server_fd;
        if (accepted_fd != -1) {
            FD_SET(accepted_fd, &read_fds);
            max_fd = std::max(max_fd, accepted_fd);
        }

        struct timeval timeout{};
        timeout.tv_sec = 1; // Тайм-аут для выхода из pselect и проверки состояния

        int activity = select(max_fd + 1, &read_fds, nullptr, nullptr, &timeout);
        if (activity == -1) {
            if (errno == EINTR) {
                // Прерывание из-за сигнала, проверяем флаг завершения
                continue;
            }
            perror("select");
            break;
        }

        // Обработка новых подключений
        if (FD_ISSET(server_fd, &read_fds)) {
            struct sockaddr_in client_address{};
            socklen_t client_len = sizeof(client_address);
            int new_fd = accept(server_fd, (struct sockaddr *)&client_address, &client_len);
            if (new_fd == -1) {
                perror("accept");
                continue;
            }

            cout << "New connection accepted" << endl;

            if (accepted_fd == -1) {
                accepted_fd = new_fd;
            } else {
                cout << "Closing additional connection" << endl;
                close(new_fd);
            }
        }

        // Обработка данных от принятого соединения
        if (accepted_fd != -1 && FD_ISSET(accepted_fd, &read_fds)) {
            char buffer[1024];
            ssize_t bytes_read = read(accepted_fd, buffer, sizeof(buffer));
            if (bytes_read > 0) {
                cout << "Received " << bytes_read << " bytes" << endl;
            } else if (bytes_read == 0) {
                cout << "Connection closed by client" << endl;
                close(accepted_fd);
                accepted_fd = -1;
            } else {
                perror("read");
                close(accepted_fd);
                accepted_fd = -1;
            }
        }
    }

    // Освобождение ресурсов перед завершением
    cout << "Shutting down server..." << endl;
    close(server_fd);
    if (accepted_fd != -1) {
        close(accepted_fd);
    }

    return 0;
}

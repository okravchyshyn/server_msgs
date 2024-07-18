#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/select.h>

#define PORT 8080
#define BUF_SIZE 1024

void set_nonblocking(int fd) {
    int flags = fcntl(fd, F_GETFL, 0);
    fcntl(fd, F_SETFL, flags | O_NONBLOCK);
}

int main() {
    int sock = 0;
    struct sockaddr_in serv_addr;
    char buffer[BUF_SIZE] = {0};
    char send_buffer[BUF_SIZE + 50];
    char nickname[50];

    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("Socket creation error");
        return -1;
    }

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT);

    if (inet_pton(AF_INET, "192.168.0.179", &serv_addr.sin_addr) <= 0) {
        perror("Invalid address / Address not supported");
        return -1;
    }

    if (connect(sock, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0) {
        perror("Connection Failed");
        return -1;
    }

    // Введення нікнейму
    printf("Enter your nickname: ");
    fgets(nickname, sizeof(nickname), stdin);
    nickname[strcspn(nickname, "\n")] = 0; // Видалення символу нового рядка

    printf("Welcome, %s! You can now send messages. Type 'exit' to leave.\n", nickname);

    // Встановлення сокету і стандартного введення у неблокуючий режим
    set_nonblocking(sock);
    set_nonblocking(STDIN_FILENO);

    fd_set read_fds;
    int valread;

    while (1) {
        FD_ZERO(&read_fds);
        FD_SET(sock, &read_fds);
        FD_SET(STDIN_FILENO, &read_fds);

        int max_sd = sock > STDIN_FILENO ? sock : STDIN_FILENO;

        // Виклик select
        select(max_sd + 1, &read_fds, NULL, NULL, NULL);

        // Перевірка, чи є дані, які надійшли з сервера
        if (FD_ISSET(sock, &read_fds)) {
            valread = read(sock, buffer, BUF_SIZE);
            if (valread > 0) {
                buffer[valread] = '\0';
                printf("%s\n", buffer);
                memset(buffer, 0, BUF_SIZE);
            } else if (valread == 0) {
                printf("Server disconnected. Exiting...\n");
                close(sock);
                exit(0);
            } else if (valread < 0 && errno != EWOULDBLOCK) {
                perror("Read error");
                close(sock);
                exit(1);
            }
        }

        // Перевірка, чи є введення з клавіатури
        if (FD_ISSET(STDIN_FILENO, &read_fds)) {
            if (fgets(buffer, BUF_SIZE, stdin) != NULL) {
                if (strcmp(buffer, "exit\n") == 0) {
                    printf("You have exited.\n");
                    close(sock);
                    exit(0);
                }
                snprintf(send_buffer, sizeof(send_buffer), "%s: %s", nickname, buffer);
                send(sock, send_buffer, strlen(send_buffer), 0);
                memset(buffer, 0, BUF_SIZE);
                memset(send_buffer, 0, sizeof(send_buffer));
            }
        }
    }

    close(sock);
    return 0;
}
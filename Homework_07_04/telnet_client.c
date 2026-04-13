#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#define BUFFER_SIZE 2048

int main() {
    int client_socket;
    struct sockaddr_in server_address;
    char buffer[BUFFER_SIZE];

    client_socket = socket(AF_INET, SOCK_STREAM, 0);
    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(8080);
    server_address.sin_addr.s_addr = inet_addr("127.0.0.1");

    if (connect(client_socket, (struct sockaddr*)&server_address, sizeof(server_address)) == -1) {
        perror("Kết nối thất bại");
        exit(EXIT_FAILURE);
    }

    printf("--- TELNET CLIENT ---\n");
    printf("Nhap [username] [password]: ");
    fgets(buffer, sizeof(buffer), stdin);
    send(client_socket, buffer, strlen(buffer), 0);

    memset(buffer, 0, BUFFER_SIZE);
    recv(client_socket, buffer, BUFFER_SIZE, 0);

    if (strncmp(buffer, "OK", 2) == 0) {
        printf("Dang nhap thanh cong!\n");
        while (1) {
            printf("\ntelnet> ");
            fgets(buffer, sizeof(buffer), stdin);
            if (strncmp(buffer, "exit", 4) == 0) break;

            send(client_socket, buffer, strlen(buffer), 0);

            memset(buffer, 0, BUFFER_SIZE);
            int n = recv(client_socket, buffer, BUFFER_SIZE, 0);
            if (n > 0) {
                printf("Ket qua tu server:\n%s\n", buffer);
            }
        }
    } else {
        printf("Sai tai khoan hoac mat khau.\n");
    }

    close(client_socket);
    return 0;
}
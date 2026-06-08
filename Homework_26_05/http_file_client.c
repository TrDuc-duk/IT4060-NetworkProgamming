#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>

int main(int argc, char *argv[]) {
    if (argc != 4) {
        printf("Cach su dung: %s <IP_Server> <Port> <Duong_Dan>\n", argv[0]);
        printf("Vi du lay thu muc goc: %s 127.0.0.1 9000 /\n", argv[0]);
        printf("Vi du lay file: %s 127.0.0.1 9000 /test.txt\n", argv[0]);
        return 1;
    }

    char *ip = argv[1];
    int port = atoi(argv[2]);
    char *path = argv[3];

    int sock = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    inet_pton(AF_INET, ip, &server_addr.sin_addr);

    if (connect(sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Loi ket noi den server");
        return 1;
    }

    char request[1024];
    snprintf(request, sizeof(request),
             "GET %s HTTP/1.1\r\n"
             "Host: %s\r\n"
             "Connection: close\r\n\r\n",
             path, ip);

    send(sock, request, strlen(request), 0);
    
    printf("--- KET QUA TU SERVER ---\n");
    char response[4096];
    int bytes_received;
    while ((bytes_received = recv(sock, response, sizeof(response) - 1, 0)) > 0) {
        response[bytes_received] = '\0';
        printf("%s", response);
    }
    printf("\n-------------------------\n");

    close(sock);
    return 0;
}
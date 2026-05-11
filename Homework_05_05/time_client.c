#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define MAX_BUFFER_SIZE 1024
#define SERVER_PORT 8888

int main() {
    int sockfd;
    struct sockaddr_in server_addr;
    char buffer[MAX_BUFFER_SIZE];
    char format[MAX_BUFFER_SIZE];

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(SERVER_PORT);
    server_addr.sin_addr.s_addr = inet_addr("127.0.0.1");

    if (connect(sockfd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("Connect failed");
        return 1;
    }

    printf("Connected to server. Type 'exit' to quit.\n");

    while (1) {
        printf("\nEnter format (dd/mm/yyyy, etc.): ");
        if (fgets(format, sizeof(format), stdin) == NULL) break;
        
        format[strcspn(format, "\n")] = '\0';

        // Thoát nếu người dùng nhập 'exit'
        if (strcmp(format, "exit") == 0) break;

        // Gửi lệnh
        sprintf(buffer, "GET_TIME %s", format);
        send(sockfd, buffer, strlen(buffer), 0);

        // Nhận phản hồi
        memset(buffer, 0, sizeof(buffer));
        int n = recv(sockfd, buffer, sizeof(buffer) - 1, 0);
        if (n <= 0) {
            printf("Server disconnected.\n");
            break;
        }

        printf("Server response: %s\n", buffer);
    }

    close(sockfd);
    return 0;
}
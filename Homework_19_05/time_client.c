#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#define BUFFER_SIZE 1024

int main() {
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in addr = {AF_INET, htons(8888)};
    inet_pton(AF_INET, "127.0.0.1", &addr.sin_addr);

    if (connect(sock, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        perror("Loi ket noi");
        return 1;
    }

    printf("Da ket noi voi Time Server.\n");
    printf("Cu phap hop le: GET_TIME [format]\n");
    printf("Go 'exit' de thoat.\n\n");

    char buffer[BUFFER_SIZE];
    char response[BUFFER_SIZE];

    while (1) {
        printf("Nhap lenh > ");
        if (fgets(buffer, sizeof(buffer), stdin) == NULL) break;

        buffer[strcspn(buffer, "\n")] = 0; // Xóa enter
        
        if (strcmp(buffer, "exit") == 0) break;
        if (strlen(buffer) == 0) continue;

        // Gửi lệnh lên Server
        send(sock, buffer, strlen(buffer), 0);
        
        // Chờ nhận kết quả
        memset(response, 0, sizeof(response));
        int bytes = recv(sock, response, sizeof(response) - 1, 0);
        
        if (bytes <= 0) {
            printf("\nMat ket noi voi Server!\n");
            break;
        }
        
        printf("Server tra ve: %s\n", response);
    }

    close(sock);
    return 0;
}
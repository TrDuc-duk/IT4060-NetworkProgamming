#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#define PORT 8080
#define BUFFER_SIZE 256

int main() {
    int sock = 0, valread;
    struct sockaddr_in serv_addr;
    char buffer[BUFFER_SIZE] = {0};
    char input[BUFFER_SIZE];

    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        printf("\n Loi tao Socket \n");
        return -1;
    }

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT);

    if (inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr) <= 0) {
        printf("\n Dia chi khong hop le \n");
        return -1;
    }

    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        printf("\n Ket noi that bai \n");
        return -1;
    }

    // Vòng lặp nhận yêu cầu từ server và trả lời
    while ((valread = read(sock, buffer, BUFFER_SIZE - 1)) > 0) {
        buffer[valread] = '\0';
        printf("Server: %s", buffer);
        
        // Nếu thông điệp là kết quả trả về email thì thoát
        if (strstr(buffer, "Email cua ban:") != NULL) {
            break;
        }

        // Nhập thông tin từ người dùng và gửi đi
        fgets(input, BUFFER_SIZE, stdin);
        send(sock, input, strlen(input), 0);
        memset(buffer, 0, BUFFER_SIZE);
    }

    close(sock);
    return 0;
}

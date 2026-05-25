#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#define BUFFER_SIZE 1024

int main() {
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in addr = {AF_INET, htons(8080)};
    inet_pton(AF_INET, "127.0.0.1", &addr.sin_addr);

    if (connect(sock, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        perror("Loi: Khong the ket noi den Server");
        return 1;
    }

    // Soạn gói tin HTTP Request chuẩn
    char *request = "GET / HTTP/1.1\r\n"
                    "Host: 127.0.0.1\r\n"
                    "Connection: close\r\n\r\n";
                    
    send(sock, request, strlen(request), 0);
    printf("Da gui HTTP Request len Server.\n");

    // Nhận và in phản hồi từ Server
    char buffer[BUFFER_SIZE];
    int bytes_received;
    
    printf("\n--- Phan hoi tu Server ---\n");
    // Dùng vòng lặp recv phòng trường hợp Server trả về gói dữ liệu lớn hơn BUFFER_SIZE
    while ((bytes_received = recv(sock, buffer, sizeof(buffer) - 1, 0)) > 0) {
        buffer[bytes_received] = '\0';
        printf("%s", buffer);
    }
    printf("\n--------------------------\n");

    close(sock);
    return 0;
}
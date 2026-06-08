#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>

int main(int argc, char *argv[]) {
    // Kiểm tra số lượng tham số đầu vào từ command line
    if (argc != 7) {
        printf("Cach su dung: %s <IP_Server> <Port> <GET/POST> <cmd> <x> <y>\n", argv[0]);
        printf("Vi du: %s 127.0.0.1 9000 GET add 10 5\n", argv[0]);
        printf("Luu y cmd gom: add, subtract, multiply, divide (theo code server cua ban)\n");
        return 1;
    }

    char *ip = argv[1];
    int port = atoi(argv[2]);
    char *method = argv[3];
    char *cmd = argv[4];
    char *x = argv[5];
    char *y = argv[6];

    // Tạo socket
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        perror("Khong the tao socket");
        return 1;
    }

    // Cấu hình địa chỉ server
    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    inet_pton(AF_INET, ip, &server_addr.sin_addr);

    // Kết nối đến server
    if (connect(sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Loi ket noi den server");
        return 1;
    }

    char request[1024];

    // Xây dựng chuỗi HTTP Request tùy theo phương thức GET hoặc POST
    if (strcmp(method, "GET") == 0) {
        snprintf(request, sizeof(request),
                 "GET /?x=%s&y=%s&cmd=%s HTTP/1.1\r\n"
                 "Host: %s\r\n"
                 "Connection: close\r\n\r\n",
                 x, y, cmd, ip);
    } else if (strcmp(method, "POST") == 0) {
        char body[256];
        snprintf(body, sizeof(body), "x=%s&y=%s&cmd=%s", x, y, cmd);
        
        snprintf(request, sizeof(request),
                 "POST / HTTP/1.1\r\n"
                 "Host: %s\r\n"
                 "Content-Type: application/x-www-form-urlencoded\r\n"
                 "Content-Length: %lu\r\n"
                 "Connection: close\r\n\r\n"
                 "%s",
                 ip, strlen(body), body);
    } else {
        printf("Phuong thuc phai la GET hoac POST\n");
        close(sock);
        return 1;
    }

    // Gửi request lên server
    send(sock, request, strlen(request), 0);
    printf("Da gui request:\n%s\n", request);

    // Nhận và in phản hồi từ server
    char response[4096];
    int bytes_received;
    printf("--- Phan hoi tu Server ---\n");
    while ((bytes_received = recv(sock, response, sizeof(response) - 1, 0)) > 0) {
        response[bytes_received] = '\0';
        printf("%s", response);
    }
    printf("\n--------------------------\n");

    close(sock);
    return 0;
}
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#define BUFFER_SIZE 1024

// Luồng xử lý giao tiếp với một Client cụ thể
void *handle_client(void *arg) {
    int client = *(int *)arg;
    free(arg); // Giải phóng con trỏ ngay lập tức để tránh rò rỉ bộ nhớ

    char buf[BUFFER_SIZE];
    
    // Nhận dữ liệu từ client
    int ret = recv(client, buf, sizeof(buf) - 1, 0);
    
    // Chỉ xử lý và phản hồi nếu nhận dữ liệu thành công
    if (ret > 0) {
        buf[ret] = '\0'; // Đóng chuỗi an toàn
        printf("\n--- Request tu Client %d ---\n%s\n", client, buf);

        // Trả lại kết quả HTTP 200 OK cho client
        char *msg = "HTTP/1.1 200 OK\r\n"
                    "Content-Type: text/html\r\n"
                    "Connection: close\r\n\r\n"
                    "<html><body><h1>Xin chao cac ban</h1></body></html>";
                    
        send(client, msg, strlen(msg), 0);
    } else {
        printf("Client %d ngat ket noi hoac bi loi.\n", client);
    }

    // Đóng kết nối
    close(client);
    return NULL;
}

int main() {
    int listener = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in addr = {AF_INET, htons(8080), INADDR_ANY};

    // Cho phép chạy lại server mà không bị kẹt port
    int opt = 1;
    setsockopt(listener, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    if (bind(listener, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        perror("Loi bind");
        return 1;
    }

    listen(listener, 10);
    printf("HTTP Server (Multithread) dang chay tren port 8080...\n");

    while (1) {
        // Chờ kết nối mới
        int client = accept(listener, NULL, NULL);
        if (client < 0) continue;

        printf("New client connected: %d\n", client);

        // Cấp phát động để truyền socket an toàn vào luồng
        int *arg = malloc(sizeof(int));
        *arg = client;

        // Tạo luồng xử lý
        pthread_t tid;
        pthread_create(&tid, NULL, handle_client, arg);
        pthread_detach(tid); // Tách luồng để hệ điều hành tự thu hồi tài nguyên
    }

    close(listener);
    return 0;
}
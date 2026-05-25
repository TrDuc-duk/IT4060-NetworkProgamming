#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#define BUFFER_SIZE 2048

// Luồng chịu trách nhiệm liên tục nhận tin nhắn từ Server và in ra màn hình
void *receive_handler(void *arg) {
    int sock = *(int *)arg;
    char buffer[BUFFER_SIZE];
    
    while (1) {
        memset(buffer, 0, sizeof(buffer));
        int bytes = recv(sock, buffer, sizeof(buffer) - 1, 0);
        if (bytes <= 0) {
            printf("\nMat ket noi voi Server!\n");
            exit(0);
        }
        
        // In nội dung đè lên dòng hiện tại để UI đỡ bị rối khi đang gõ chữ
        printf("\r%s> ", buffer);
        fflush(stdout);
    }
    return NULL;
}

int main() {
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in addr = {AF_INET, htons(8888)};
    inet_pton(AF_INET, "127.0.0.1", &addr.sin_addr);

    if (connect(sock, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        perror("Loi ket noi");
        return 1;
    }

    // Tạo luồng lắng nghe tin nhắn đến
    pthread_t recv_thread;
    pthread_create(&recv_thread, NULL, receive_handler, &sock);

    char msg[BUFFER_SIZE];
    
    // Luồng chính phụ trách lấy input từ bàn phím và gửi lên Server
    while (1) {
        printf("> ");
        fflush(stdout);
        
        if (fgets(msg, sizeof(msg), stdin) == NULL) {
            break; // Thoát nếu người dùng bấm Ctrl+D
        }
        
        if (send(sock, msg, strlen(msg), 0) <= 0) {
            break;
        }
    }

    close(sock);
    return 0;
}
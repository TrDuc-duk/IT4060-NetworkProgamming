#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>

#define BUFFER_SIZE 1024

int sock = 0;

// Luồng liên tục lắng nghe phản hồi và tin nhắn từ Server
void *receive_thread(void *arg) {
    char buffer[BUFFER_SIZE];
    
    while (1) {
        memset(buffer, 0, BUFFER_SIZE);
        int bytes_received = recv(sock, buffer, BUFFER_SIZE - 1, 0);
        
        if (bytes_received <= 0) {
            printf("\n[Disconnected] Server has closed the connection.\n");
            exit(0);
        }
        
        // In tin nhắn nhận được
        printf("\r%s", buffer);
        // In lại dấu nhắc lệnh để giao diện không bị rối
        printf("Enter command: ");
        fflush(stdout);
    }
    return NULL;
}

int main(int argc, char *argv[]) {
    // 1. Kiểm tra tham số đầu vào
    if (argc != 3) {
        printf("Usage: %s <server_ip> <port>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    char *server_ip = argv[1];
    int port = atoi(argv[2]);
    struct sockaddr_in server_addr;

    // 2. Khởi tạo socket TCP
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("Socket creation error");
        exit(EXIT_FAILURE);
    }

    // 3. Cấu hình địa chỉ Server để kết nối
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    if (inet_pton(AF_INET, server_ip, &server_addr.sin_addr) <= 0) {
        perror("Invalid address / Address not supported");
        exit(EXIT_FAILURE);
    }

    // 4. Kết nối tới Server
    if (connect(sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Connection failed");
        exit(EXIT_FAILURE);
    }

    printf("=== Connected to Pub/Sub Server ===\n");
    printf("- Commands:\n");
    printf("  SUB <topic>\n");
    printf("  PUB <topic> <message>\n");
    printf("-----------------------------------\n\n");

    // 5. Tạo luồng để nhận dữ liệu từ server
    pthread_t tid;
    if (pthread_create(&tid, NULL, receive_thread, NULL) != 0) {
        perror("Failed to create receive thread");
        close(sock);
        exit(EXIT_FAILURE);
    }

    // 6. Luồng chính: Đọc từ bàn phím và gửi lên Server
    char input[BUFFER_SIZE];
    while (1) {
        printf("Enter command: ");
        if (fgets(input, BUFFER_SIZE, stdin) == NULL) {
            break; // Thoát nếu gặp EOF (Ctrl+D)
        }

        // Gửi lệnh lên server
        send(sock, input, strlen(input), 0);
    }

    // Dọn dẹp
    close(sock);
    return 0;
}
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>

#define BUF_SIZE 1024

int sock;
struct sockaddr_in remote_addr;

// Luồng chuyên dùng để nhận tin nhắn
void *receive_thread(void *arg) {
    char buffer[BUF_SIZE];
    struct sockaddr_in sender_addr;
    socklen_t addr_len = sizeof(sender_addr);

    while (1) {
        memset(buffer, 0, BUF_SIZE);
        // recvfrom sẽ block cho đến khi có dữ liệu đến
        int recv_len = recvfrom(sock, buffer, BUF_SIZE - 1, 0, 
                               (struct sockaddr *)&sender_addr, &addr_len);
        if (recv_len > 0) {
            buffer[recv_len] = '\0';
            // In tin nhắn nhận được và in lại prompt "You: " để giao diện không bị nát
            printf("\r[Remote]: %s", buffer);
            printf("You: ");
            fflush(stdout);
        }
    }
    return NULL;
}

int main(int argc, char *argv[]) {
    // 1. Kiểm tra tham số đầu vào
    if (argc != 4) {
        printf("Usage: %s <port> <remote_ip> <remote_port>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    int local_port = atoi(argv[1]);
    char *remote_ip = argv[2];
    int remote_port = atoi(argv[3]);

    // 2. Khởi tạo socket UDP
    if ((sock = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    // 3. Cấu hình địa chỉ local để bind (lắng nghe)
    struct sockaddr_in local_addr;
    memset(&local_addr, 0, sizeof(local_addr));
    local_addr.sin_family = AF_INET;
    local_addr.sin_addr.s_addr = INADDR_ANY;
    local_addr.sin_port = htons(local_port);

    if (bind(sock, (const struct sockaddr *)&local_addr, sizeof(local_addr)) < 0) {
        perror("Bind failed");
        close(sock);
        exit(EXIT_FAILURE);
    }

    // 4. Cấu hình địa chỉ remote đích để gửi tin
    memset(&remote_addr, 0, sizeof(remote_addr));
    remote_addr.sin_family = AF_INET;
    remote_addr.sin_port = htons(remote_port);
    if (inet_pton(AF_INET, remote_ip, &remote_addr.sin_addr) <= 0) {
        perror("Invalid remote IP address");
        close(sock);
        exit(EXIT_FAILURE);
    }

    printf("UDP Chat Started\n");
    printf("- Local port : %d\n", local_port);
  

    // 5. Tạo luồng (thread) để lắng nghe tin nhắn đến
    pthread_t tid;
    if (pthread_create(&tid, NULL, receive_thread, NULL) != 0) {
        perror("Failed to create receive thread");
        close(sock);
        exit(EXIT_FAILURE);
    }

    // 6. Luồng chính: Đọc từ bàn phím và gửi đi
    char message[BUF_SIZE];
    while (1) {
        printf("You: ");
        if (fgets(message, BUF_SIZE, stdin) == NULL) {
            break; // Thoát nếu gặp EOF (Ctrl+D)
        }

        // Bỏ ký tự xuống dòng ở cuối (nếu có) để in ra cho đẹp
        // message[strcspn(message, "\n")] = 0; 
        
        sendto(sock, message, strlen(message), 0, 
               (const struct sockaddr *)&remote_addr, sizeof(remote_addr));
    }

    // Dọn dẹp
    close(sock);
    return 0;
}
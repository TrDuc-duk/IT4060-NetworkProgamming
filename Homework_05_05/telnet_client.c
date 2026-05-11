#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <poll.h>

#define BUFFER_SIZE 1024

int main() {
    // 1. Tạo socket
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        perror("Socket creation failed");
        return 1;
    }

    // 2. Cấu hình địa chỉ server cần kết nối tới (IP localhost, Port 8000)
    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(8000);
    server_addr.sin_addr.s_addr = inet_addr("127.0.0.1");

    // 3. Kết nối tới server
    if (connect(sockfd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("Connection to server failed");
        close(sockfd);
        return 1;
    }

    printf("Connected to Telnet Server (127.0.0.1:8000).\n");
    printf("Type 'quit' or 'exit' to close the client.\n\n");

    // 4. Thiết lập mảng pollfd để giám sát 2 luồng I/O
    struct pollfd fds[2];
    
    fds[0].fd = STDIN_FILENO; // Bàn phím (Standard Input)
    fds[0].events = POLLIN;   // Chờ sự kiện có người gõ phím
    
    fds[1].fd = sockfd;       // Socket kết nối với Server
    fds[1].events = POLLIN;   // Chờ sự kiện Server gửi data về

    char buffer[BUFFER_SIZE];

    // 5. Vòng lặp chính
    while (1) {
        int ret = poll(fds, 2, -1); // Chờ vô thời hạn cho đến khi có sự kiện
        if (ret < 0) {
            perror("Poll failed");
            break;
        }

        // Sự kiện 1: Bạn gõ gì đó từ bàn phím
        if (fds[0].revents & POLLIN) {
            if (fgets(buffer, sizeof(buffer), stdin) != NULL) {
                // Tiện ích: Gõ quit hoặc exit để tự thoát client
                if (strncmp(buffer, "quit", 4) == 0 || strncmp(buffer, "exit", 4) == 0) {
                    printf("Exiting client...\n");
                    break;
                }
                // Gửi lệnh vừa gõ lên Server
                send(sockfd, buffer, strlen(buffer), 0);
            }
        }

        // Sự kiện 2: Server trả kết quả về (Thông báo đăng nhập hoặc kết quả lệnh dir/ls)
        if (fds[1].revents & POLLIN) {
            int bytes_received = recv(sockfd, buffer, sizeof(buffer) - 1, 0);
            
            // Nếu bytes_received <= 0 nghĩa là Server đã sập hoặc chủ động ngắt kết nối
            if (bytes_received <= 0) {
                printf("\n[Connection closed by Server]\n");
                break;
            }
            
            // In kết quả ra màn hình
            buffer[bytes_received] = '\0';
            printf("%s", buffer);
            fflush(stdout); // Đẩy dữ liệu ra màn hình ngay lập tức
        }
    }

    // Đóng kết nối khi kết thúc
    close(sockfd);
    return 0;
}
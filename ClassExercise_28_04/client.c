#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>

int main() {
    int sock;
    struct sockaddr_in server;
    char message[2048], server_reply[2048];

    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock == -1) {
        perror("Không thể tạo socket");
        return 1;
    }

    server.sin_family = AF_INET;
    server.sin_addr.s_addr = inet_addr("127.0.0.1"); // Kết nối tới localhost
    server.sin_port = htons(8888);

    if (connect(sock, (struct sockaddr *)&server, sizeof(server)) < 0) {
        perror("Lỗi kết nối tới server");
        return 1;
    }

    // Nhận xâu chào từ server
    int read_size = recv(sock, server_reply, sizeof(server_reply) - 1, 0);
    if (read_size > 0) {
        server_reply[read_size] = '\0';
        printf("Server: %s", server_reply);
    }

    // Vòng lặp gửi nhận tin nhắn
    while (1) {
        printf("Nhập xâu (gõ 'exit' để thoát): ");
        fgets(message, sizeof(message), stdin);
        
        // Xóa ký tự newline do fgets tạo ra
        message[strcspn(message, "\n")] = 0;

        // Gửi dữ liệu tới server
        if (send(sock, message, strlen(message), 0) < 0) {
            perror("Gửi thất bại");
            break;
        }

        // Nhận phản hồi từ server
        memset(server_reply, 0, sizeof(server_reply)); // Xóa bộ đệm cũ
        read_size = recv(sock, server_reply, sizeof(server_reply) - 1, 0);
        
        if (read_size > 0) {
            server_reply[read_size] = '\0';
            printf("%s", server_reply);
            
            // Nếu server gửi xâu tạm biệt thì thoát vòng lặp
            if (strstr(server_reply, "Tạm biệt") != NULL) {
                break;
            }
        } else if (read_size == 0) {
            printf("Server đã ngắt kết nối.\n");
            break;
        } else {
            perror("Lỗi nhận dữ liệu");
            break;
        }
    }

    close(sock);
    return 0;
}
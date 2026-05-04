#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/socket.h>
#include <arpa/inet.h>

int client_count = 0;
pthread_mutex_t count_mutex = PTHREAD_MUTEX_INITIALIZER;

// Hàm mã hóa xâu ký tự theo quy tắc
void encrypt_string(char *str) {
    for (int i = 0; str[i] != '\0'; i++) {
        if (str[i] >= 'a' && str[i] <= 'y') {
            str[i]++;
        } else if (str[i] == 'z') {
            str[i] = 'a';
        } else if (str[i] >= 'A' && str[i] <= 'Y') {
            str[i]++;
        } else if (str[i] == 'Z') {
            str[i] = 'A';
        } else if (str[i] >= '0' && str[i] <= '9') {
            str[i] = '9' - (str[i] - '0'); // Ký tự số thay bằng số bù 9
        }
        // Ký tự khác giữ nguyên
    }
}

// Hàm xử lý giao tiếp với từng client
void *connection_handler(void *socket_desc) {
    int sock = *(int*)socket_desc;
    int read_size;
    char client_message[2048];
    char server_message[2048];

    // Cập nhật số lượng client an toàn với mutex
    pthread_mutex_lock(&count_mutex);
    client_count++;
    int current_count = client_count;
    pthread_mutex_unlock(&count_mutex);

    // Gửi xâu chào
    snprintf(server_message, sizeof(server_message), "Xin chào. Hiện có %d clients đang kết nối.\n", current_count);
    write(sock, server_message, strlen(server_message));

    // Nhận thông điệp từ client
    while ((read_size = recv(sock, client_message, sizeof(client_message) - 1, 0)) > 0) {
        client_message[read_size] = '\0'; // Chốt chuỗi
        
        // Xóa ký tự xuống dòng (\n hoặc \r) nếu có
        client_message[strcspn(client_message, "\r\n")] = 0;

        // Kiểm tra lệnh exit
        if (strcmp(client_message, "exit") == 0) {
            char *bye_msg = "Tạm biệt!\n";
            write(sock, bye_msg, strlen(bye_msg));
            break;
        }

        // Mã hóa và gửi lại
        encrypt_string(client_message);
        snprintf(server_message, sizeof(server_message), "Kết quả: %s\n", client_message);
        write(sock, server_message, strlen(server_message));
    }

    // Giảm số lượng client khi đóng kết nối
    pthread_mutex_lock(&count_mutex);
    client_count--;
    pthread_mutex_unlock(&count_mutex);

    close(sock);
    free(socket_desc);
    return 0;
}

int main() {
    int server_sock, client_sock, *new_sock;
    struct sockaddr_in server, client;
    socklen_t c = sizeof(struct sockaddr_in);

    server_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (server_sock == -1) {
        perror("Không thể tạo socket");
        return 1;
    }

    // Cho phép tái sử dụng port ngay lập tức (tránh lỗi Address already in use)
    int opt = 1;
    setsockopt(server_sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    server.sin_family = AF_INET;
    server.sin_addr.s_addr = INADDR_ANY;
    server.sin_port = htons(8888);

    if (bind(server_sock, (struct sockaddr *)&server, sizeof(server)) < 0) {
        perror("Lỗi bind");
        return 1;
    }

    listen(server_sock, 5);
    printf("Server đang chạy tại cổng 8888...\n");

    while ((client_sock = accept(server_sock, (struct sockaddr *)&client, &c))) {
        pthread_t client_thread;
        new_sock = malloc(sizeof(int));
        *new_sock = client_sock;

        if (pthread_create(&client_thread, NULL, connection_handler, (void*) new_sock) < 0) {
            perror("Không thể tạo thread");
            return 1;
        }
        pthread_detach(client_thread); // Giải phóng tài nguyên thread tự động
    }

    close(server_sock);
    return 0;
}
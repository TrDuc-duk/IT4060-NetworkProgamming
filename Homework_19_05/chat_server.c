#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <time.h>

#define MAX_CLIENTS 100
#define BUFFER_SIZE 2048

// Cấu trúc lưu trữ thông tin của một Client
typedef struct {
    int socket;
    char client_id[50];
    char client_name[50];
    int active;
} ClientInfo;

ClientInfo clients[MAX_CLIENTS];
pthread_mutex_t clients_mutex = PTHREAD_MUTEX_INITIALIZER;

// Hàm gửi tin nhắn đến tất cả các client (trừ người gửi)
void broadcast_message(char *message, int sender_socket) {
    pthread_mutex_lock(&clients_mutex);
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (clients[i].active && clients[i].socket != sender_socket) {
            send(clients[i].socket, message, strlen(message), 0);
        }
    }
    pthread_mutex_unlock(&clients_mutex);
}

// Luồng xử lý cho từng Client
void *handle_client(void *arg) {
    int client_index = *(int *)arg;
    int sock = clients[client_index].socket;
    char buffer[BUFFER_SIZE];
    char id[50], name[50];

    // 1. Vòng lặp xác thực cú pháp "client_id client_name"
    while (1) {
        char *prompt = "Vui long nhap ten (Cu phap: client_id client_name): ";
        send(sock, prompt, strlen(prompt), 0);
        
        memset(buffer, 0, sizeof(buffer));
        int bytes = recv(sock, buffer, sizeof(buffer) - 1, 0);
        if (bytes <= 0) {
            close(sock);
            clients[client_index].active = 0;
            return NULL;
        }

        // Kiểm tra xem chuỗi nhận được có đúng 2 thành phần không
        if (sscanf(buffer, "%49s %49s", id, name) == 2) {
            strcpy(clients[client_index].client_id, id);
            strcpy(clients[client_index].client_name, name);
            char *success = "Dang nhap thanh cong! Bat dau chat...\n";
            send(sock, success, strlen(success), 0);
            printf("Client joined: ID=%s, Name=%s\n", id, name);
            break;
        } else {
            char *err = "Sai cu phap. Vui long thu lai!\n";
            send(sock, err, strlen(err), 0);
        }
    }

    // 2. Vòng lặp nhận và broadcast tin nhắn
    while (1) {
        memset(buffer, 0, sizeof(buffer));
        int bytes = recv(sock, buffer, sizeof(buffer) - 1, 0);
        if (bytes <= 0) {
            printf("Client %s disconnected.\n", clients[client_index].client_id);
            break;
        }

        buffer[strcspn(buffer, "\n")] = 0; // Xóa ký tự xuống dòng
        if (strlen(buffer) == 0) continue;

        // Lấy thời gian hiện tại
        time_t rawtime;
        struct tm *timeinfo;
        char time_str[80];
        time(&rawtime);
        timeinfo = localtime(&rawtime);
        // Format: YYYY/MM/DD HH:MMPM
        strftime(time_str, sizeof(time_str), "%Y/%m/%d %I:%M%p", timeinfo);

        // Chuẩn bị chuỗi broadcast: "2023/05/6 11:00PM abc: xin chào"
        char broadcast_str[BUFFER_SIZE * 2];
        snprintf(broadcast_str, sizeof(broadcast_str), "%s %s: %s\n", time_str, clients[client_index].client_id, buffer);

        // Phân phát cho mọi người
        broadcast_message(broadcast_str, sock);
    }

    // Dọn dẹp khi Client ngắt kết nối
    close(sock);
    pthread_mutex_lock(&clients_mutex);
    clients[client_index].active = 0;
    pthread_mutex_unlock(&clients_mutex);
    return NULL;
}

int main() {
    int server_sock = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in addr = {AF_INET, htons(8888), INADDR_ANY};

    int opt = 1;
    setsockopt(server_sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    bind(server_sock, (struct sockaddr *)&addr, sizeof(addr));
    listen(server_sock, 10);

    // Khởi tạo mảng client
    for (int i = 0; i < MAX_CLIENTS; i++) clients[i].active = 0;

    printf("Chat Server dang chay tren port 8888...\n");

    while (1) {
        int client_sock = accept(server_sock, NULL, NULL);
        if (client_sock < 0) continue;

        pthread_mutex_lock(&clients_mutex);
        int index = -1;
        // Tìm vị trí trống trong mảng
        for (int i = 0; i < MAX_CLIENTS; i++) {
            if (!clients[i].active) {
                clients[i].socket = client_sock;
                clients[i].active = 1;
                index = i;
                break;
            }
        }
        pthread_mutex_unlock(&clients_mutex);

        if (index != -1) {
            // Cấp phát động để truyền index an toàn vào luồng
            int *arg = malloc(sizeof(int));
            *arg = index;
            pthread_t tid;
            pthread_create(&tid, NULL, handle_client, arg);
            pthread_detach(tid);
        } else {
            char *full = "Server da day!\n";
            send(client_sock, full, strlen(full), 0);
            close(client_sock);
        }
    }

    return 0;
}
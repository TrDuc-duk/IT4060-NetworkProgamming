#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>

#define PORT 9000
#define MAX_CLIENTS 100
#define MAX_TOPICS_PER_CLIENT 10
#define TOPIC_LEN 50
#define BUFFER_SIZE 1024

// Cấu trúc lưu trữ thông tin của một Client
typedef struct {
    int socket;
    char topics[MAX_TOPICS_PER_CLIENT][TOPIC_LEN];
    int topic_count;
} ClientInfo;

ClientInfo clients[MAX_CLIENTS];
pthread_mutex_t clients_mutex = PTHREAD_MUTEX_INITIALIZER;

// Hàm xử lý luồng cho từng client
void *client_handler(void *arg) {
    int client_index = *((int *)arg);
    free(arg); 
    
    int sock = clients[client_index].socket;
    char buffer[BUFFER_SIZE];
    
    printf("Client [%d] connected.\n", sock);

    while (1) {
        memset(buffer, 0, BUFFER_SIZE);
        int bytes_received = recv(sock, buffer, BUFFER_SIZE - 1, 0);
        
        // Nếu nhận <= 0 byte nghĩa là client đã ngắt kết nối
        if (bytes_received <= 0) {
            break;
        }

        char cmd[10], topic[TOPIC_LEN], msg[BUFFER_SIZE];
        memset(cmd, 0, sizeof(cmd));
        memset(topic, 0, sizeof(topic));
        memset(msg, 0, sizeof(msg));

        // Phân tích chuỗi nhận được: Lệnh, Topic, và phần Message
        int parsed = sscanf(buffer, "%s %s %[^\r\n]", cmd, topic, msg);

        // --- Xử lý lệnh SUB ---
        if (strcmp(cmd, "SUB") == 0 && parsed >= 2) {
            pthread_mutex_lock(&clients_mutex);
            // Kiểm tra xem client đã đăng ký topic này chưa để tránh trùng lặp
            int already_subbed = 0;
            for (int i = 0; i < clients[client_index].topic_count; i++) {
                if (strcmp(clients[client_index].topics[i], topic) == 0) {
                    already_subbed = 1;
                    break;
                }
            }

            if (already_subbed) {
                char *err = "-> Error: You are already subscribed to this topic\n";
                send(sock, err, strlen(err), 0);
            } else if (clients[client_index].topic_count < MAX_TOPICS_PER_CLIENT) {
                int t_count = clients[client_index].topic_count;
                strcpy(clients[client_index].topics[t_count], topic);
                clients[client_index].topic_count++;
                
                char ack[100];
                sprintf(ack, "-> Subscribed to topic: %s\n", topic);
                send(sock, ack, strlen(ack), 0);
                printf("Client [%d] subscribed to '%s'\n", sock, topic);
            } else {
                char *err = "-> Error: Max topics reached\n";
                send(sock, err, strlen(err), 0);
            }
            pthread_mutex_unlock(&clients_mutex);
        } 
        // --- Xử lý lệnh UNSUB ---
        else if (strcmp(cmd, "UNSUB") == 0 && parsed >= 2) {
            pthread_mutex_lock(&clients_mutex);
            int found = 0;
            for (int i = 0; i < clients[client_index].topic_count; i++) {
                if (strcmp(clients[client_index].topics[i], topic) == 0) {
                    // Xóa topic bằng cách copy topic cuối cùng đè lên vị trí hiện tại
                    int last_index = clients[client_index].topic_count - 1;
                    if (i != last_index) {
                        strcpy(clients[client_index].topics[i], clients[client_index].topics[last_index]);
                    }
                    clients[client_index].topic_count--;
                    found = 1;
                    break;
                }
            }
            pthread_mutex_unlock(&clients_mutex);

            if (found) {
                char ack[100];
                sprintf(ack, "-> Unsubscribed from topic: %s\n", topic);
                send(sock, ack, strlen(ack), 0);
                printf("Client [%d] unsubscribed from '%s'\n", sock, topic);
            } else {
                char *err = "-> Error: You are not subscribed to this topic\n";
                send(sock, err, strlen(err), 0);
            }
        }
        // --- Xử lý lệnh PUB ---
        else if (strcmp(cmd, "PUB") == 0 && parsed == 3) {
            pthread_mutex_lock(&clients_mutex);
            char forward_msg[BUFFER_SIZE + 100];
            sprintf(forward_msg, "[New message in %s]: %s\n", topic, msg);
            
            // Duyệt qua toàn bộ client đang hoạt động
            for (int i = 0; i < MAX_CLIENTS; i++) {
                if (clients[i].socket != 0 && clients[i].socket != sock) {
                    for (int j = 0; j < clients[i].topic_count; j++) {
                        if (strcmp(clients[i].topics[j], topic) == 0) {
                            send(clients[i].socket, forward_msg, strlen(forward_msg), 0);
                            break; 
                        }
                    }
                }
            }
            pthread_mutex_unlock(&clients_mutex);
            
            char *ack = "-> Message published successfully\n";
            send(sock, ack, strlen(ack), 0);
            printf("Client [%d] published to '%s'\n", sock, topic);
        } else {
            char *err = "-> Invalid command. Use 'SUB <topic>', 'UNSUB <topic>', or 'PUB <topic> <msg>'\n";
            send(sock, err, strlen(err), 0);
        }
    }

    // Dọn dẹp khi client ngắt kết nối
    pthread_mutex_lock(&clients_mutex);
    close(clients[client_index].socket);
    clients[client_index].socket = 0;
    clients[client_index].topic_count = 0;
    pthread_mutex_unlock(&clients_mutex);
    
    printf("Client [%d] disconnected.\n", sock);
    return NULL;
}

int main() {
    int server_sock, client_sock;
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_len = sizeof(client_addr);

    for (int i = 0; i < MAX_CLIENTS; i++) {
        clients[i].socket = 0;
        clients[i].topic_count = 0;
    }

    if ((server_sock = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    int opt = 1;
    setsockopt(server_sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);

    if (bind(server_sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1) {
        perror("Bind failed");
        close(server_sock);
        exit(EXIT_FAILURE);
    }

    if (listen(server_sock, 10) == -1) {
        perror("Listen failed");
        close(server_sock);
        exit(EXIT_FAILURE);
    }

    printf("=== Pub/Sub Server started on port %d ===\n", PORT);

    while (1) {
        client_sock = accept(server_sock, (struct sockaddr *)&client_addr, &client_len);
        if (client_sock < 0) {
            perror("Accept failed");
            continue;
        }

        pthread_mutex_lock(&clients_mutex);
        int index = -1;
        for (int i = 0; i < MAX_CLIENTS; i++) {
            if (clients[i].socket == 0) {
                clients[i].socket = client_sock;
                clients[i].topic_count = 0;
                index = i;
                break;
            }
        }
        pthread_mutex_unlock(&clients_mutex);

        if (index != -1) {
            pthread_t tid;
            int *arg = malloc(sizeof(int));
            *arg = index;
            pthread_create(&tid, NULL, client_handler, arg);
            pthread_detach(tid); 
        } else {
            printf("Server full. Rejecting connection.\n");
            close(client_sock);
        }
    }

    close(server_sock);
    return 0;
}
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#define MAX_WAITING 100
#define BUFFER_SIZE 2048

int waiting_queue[MAX_WAITING];
int queue_count = 0;
pthread_mutex_t queue_mutex = PTHREAD_MUTEX_INITIALIZER;

typedef struct {
    int client1;
    int client2;
} ClientPair;

// Tạo struct riêng để truyền tham số an toàn vào luồng
typedef struct {
    int src;
    int dst;
} ForwardArgs;

// Luồng chuyển tiếp dữ liệu từ nguồn sang đích
void *forward_message(void *arg) {
    ForwardArgs *args = (ForwardArgs *)arg;
    int src = args->src;
    int dst = args->dst;
    char buffer[BUFFER_SIZE];

    while (1) {
        int bytes_received = recv(src, buffer, sizeof(buffer), 0);
        
        if (bytes_received <= 0) {
            // Dùng shutdown để ép ngắt toàn bộ hoạt động đọc/ghi ngay lập tức.
            // Điều này sẽ đánh thức hàm recv() đang bị treo ở luồng đối diện.
            printf("Socket %d disconnected. Forcing close on socket %d.\n", src, dst);
            shutdown(src, SHUT_RDWR);
            shutdown(dst, SHUT_RDWR);
            close(src);
            close(dst);
            break;
        }
        
        // Nếu gửi lỗi (bên kia đã ngắt) thì cũng thoát vòng lặp
        if (send(dst, buffer, bytes_received, 0) <= 0) {
            break;
        }
    }
    
    free(args); // Giải phóng bộ nhớ đã cấp phát cho tham số luồng
    return NULL;
}

// Hàm quản lý một cặp chat
void *handle_pair(void *arg) {
    ClientPair *pair = (ClientPair *)arg;
    pthread_t t1, t2;

    // Cấp phát động tham số cho 2 chiều chuyển tiếp để tránh xung đột vùng nhớ
    ForwardArgs *args_1_to_2 = malloc(sizeof(ForwardArgs));
    args_1_to_2->src = pair->client1;
    args_1_to_2->dst = pair->client2;

    ForwardArgs *args_2_to_1 = malloc(sizeof(ForwardArgs));
    args_2_to_1->src = pair->client2;
    args_2_to_1->dst = pair->client1;

    printf("Pair established between socket %d and %d\n", pair->client1, pair->client2);

    // Tạo 2 luồng chuyển tiếp song song cho 2 chiều
    pthread_create(&t1, NULL, forward_message, (void *)args_1_to_2);
    pthread_create(&t2, NULL, forward_message, (void *)args_2_to_1);

    // Đợi 2 luồng này kết thúc
    pthread_join(t1, NULL);
    pthread_join(t2, NULL);

    printf("Pair session %d-%d ended.\n", pair->client1, pair->client2);
    free(pair);
    return NULL;
}

int main() {
    int server_sock = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in addr = {AF_INET, htons(8888), INADDR_ANY};

    // Tùy chọn này giúp bạn có thể chạy lại server ngay lập tức mà không bị lỗi "Address already in use"
    int opt = 1;
    setsockopt(server_sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    if (bind(server_sock, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        perror("Bind failed");
        exit(1);
    }
    
    listen(server_sock, 10);

    printf("Chat Server started on port 8888. Waiting for clients...\n");

    while (1) {
        int client_sock = accept(server_sock, NULL, NULL);
        if (client_sock < 0) continue;

        printf("Client %d connected. Searching for partner...\n", client_sock);

        pthread_mutex_lock(&queue_mutex);
        waiting_queue[queue_count++] = client_sock;

        if (queue_count >= 2) {
            // Lấy 2 client ra khỏi hàng đợi để ghép cặp
            ClientPair *pair = malloc(sizeof(ClientPair));
            pair->client1 = waiting_queue[queue_count - 2];
            pair->client2 = waiting_queue[queue_count - 1];
            queue_count -= 2;

            pthread_t thread_id;
            pthread_create(&thread_id, NULL, handle_pair, (void *)pair);
            pthread_detach(thread_id); // Tách luồng quản lý cặp để main loop tiếp tục accept người mới
        } else {
            char *wait_msg = "Waiting for a partner...\n";
            send(client_sock, wait_msg, strlen(wait_msg), 0);
        }
        pthread_mutex_unlock(&queue_mutex);
    }

    close(server_sock);
    return 0;
}
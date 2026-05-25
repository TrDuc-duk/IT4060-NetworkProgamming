#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <time.h>

#define BUFFER_SIZE 1024

// Luồng xử lý cho từng Client
void *handle_client(void *arg) {
    int sock = *(int *)arg;
    free(arg); // Giải phóng bộ nhớ động
    char buffer[BUFFER_SIZE];
    char format[50];

    while (1) {
        memset(buffer, 0, sizeof(buffer));
        int bytes_received = recv(sock, buffer, sizeof(buffer) - 1, 0);
        
        if (bytes_received <= 0) {
            printf("Socket %d disconnected.\n", sock);
            break;
        }

        // Xóa ký tự xuống dòng (enter) từ Client
        buffer[strcspn(buffer, "\r\n")] = '\0';
        if (strlen(buffer) == 0) continue;

        // Kiểm tra đúng cú pháp "GET_TIME "
        if (strncmp(buffer, "GET_TIME ", 9) == 0) {
            strcpy(format, buffer + 9);
            
            // Lấy thời gian thực của hệ thống
            time_t t = time(NULL);
            struct tm *tm = localtime(&t);
            char response[256];

            // So sánh và format thời gian theo yêu cầu
            if (strcmp(format, "dd/mm/yyyy") == 0) {
                strftime(response, sizeof(response), "%d/%m/%Y\n", tm);
            } else if (strcmp(format, "dd/mm/yy") == 0) {
                strftime(response, sizeof(response), "%d/%m/%y\n", tm);
            } else if (strcmp(format, "mm/dd/yyyy") == 0) {
                strftime(response, sizeof(response), "%m/%d/%Y\n", tm);
            } else if (strcmp(format, "mm/dd/yy") == 0) {
                strftime(response, sizeof(response), "%m/%d/%y\n", tm);
            } else {
                strcpy(response, "Loi: Format khong duoc ho tro. Cac format hop le: dd/mm/yyyy, dd/mm/yy, mm/dd/yyyy, mm/dd/yy\n");
            }
            
            // Trả kết quả về cho Client
            send(sock, response, strlen(response), 0);
        } else {
            // Lỗi gõ sai lệnh hoàn toàn
            char *err = "Loi: Sai cu phap. Hay dung 'GET_TIME [format]'\n";
            send(sock, err, strlen(err), 0);
        }
    }
    
    close(sock);
    return NULL;
}

int main() {
    int server_sock = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in addr = {AF_INET, htons(8888), INADDR_ANY};

    // Tái sử dụng port tránh lỗi "Address already in use"
    int opt = 1;
    setsockopt(server_sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    if (bind(server_sock, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        perror("Bind failed");
        exit(1);
    }
    
    listen(server_sock, 10);
    printf("Time Server (Multithreaded) dang chay tren port 8888...\n");

    while (1) {
        int client_sock = accept(server_sock, NULL, NULL);
        if (client_sock < 0) continue;

        printf("New connection: Socket %d\n", client_sock);

        // Cấp phát động tham số cho luồng để tránh đụng độ bộ nhớ giữa các client
        int *arg = malloc(sizeof(int));
        *arg = client_sock;

        pthread_t tid;
        pthread_create(&tid, NULL, handle_client, arg);
        // Tách luồng để nó tự dọn dẹp tài nguyên khi kết thúc
        pthread_detach(tid);
    }

    close(server_sock);
    return 0;
}
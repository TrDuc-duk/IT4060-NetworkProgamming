#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <time.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>

#define MAX_BUFFER_SIZE 1024
#define SERVER_PORT 8888

void handle_client(int client_sock) {
    char buffer[MAX_BUFFER_SIZE];
    char formatted_time[MAX_BUFFER_SIZE];

    memset(buffer, 0, sizeof(buffer));
    int bytes_received = recv(client_sock, buffer, sizeof(buffer) - 1, 0);
    if (bytes_received <= 0) {
        close(client_sock);
        exit(EXIT_SUCCESS);
    }

    // Xử lý tách chuỗi lệnh: GET_TIME [format]
    char command[32] = "";
    char format[32] = "";
    
    // Sử dụng sscanf để kiểm tra định dạng lệnh gửi lên
    int num_args = sscanf(buffer, "%s %s", command, format);

    if (num_args == 2 && strcmp(command, "GET_TIME") == 0) {
        time_t current_time = time(NULL);
        struct tm* time_info = localtime(&current_time);

        if (strcmp(format, "dd/mm/yyyy") == 0) {
            strftime(formatted_time, sizeof(formatted_time), "%d/%m/%Y", time_info);
        } else if (strcmp(format, "dd/mm/yy") == 0) {
            strftime(formatted_time, sizeof(formatted_time), "%d/%m/%y", time_info);
        } else if (strcmp(format, "mm/dd/yyyy") == 0) {
            strftime(formatted_time, sizeof(formatted_time), "%m/%d/%Y", time_info);
        } else if (strcmp(format, "mm/dd/yy") == 0) {
            strftime(formatted_time, sizeof(formatted_time), "%m/%d/%y", time_info);
        } else {
            strcpy(formatted_time, "ERROR: Invalid format");
        }
    } else {
        strcpy(formatted_time, "ERROR: Invalid command syntax. Use: GET_TIME [format]");
    }

    send(client_sock, formatted_time, strlen(formatted_time), 0);
    
    printf("Handled request from client.\n");
    close(client_sock);
    exit(EXIT_SUCCESS);
}

int main() {
    int sockfd, client_sock;
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_len = sizeof(client_addr);

    // Tránh zombie process: Tự động giải phóng tiến trình con khi kết thúc
    signal(SIGCHLD, SIG_IGN);

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) { perror("Socket error"); exit(1); }

    int opt = 1;
    setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(SERVER_PORT);
    server_addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(sockfd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("Bind error"); exit(1);
    }

    listen(sockfd, 10);
    printf("Time Server is running on port %d...\n", SERVER_PORT);

    while (1) {
        client_sock = accept(sockfd, (struct sockaddr*)&client_addr, &client_len);
        if (client_sock < 0) continue;

        if (fork() == 0) { // Process con
            close(sockfd); 
            handle_client(client_sock);
        } else { // Process cha
            close(client_sock);
        }
    }
    return 0;
}
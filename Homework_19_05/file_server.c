#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <dirent.h>
#include <sys/stat.h>
#include <signal.h>

#define BUFFER_SIZE 1024
#define FILE_PATH "./files/"

void handle_client(int client_socket) {
    DIR *d;
    struct dirent *dir;
    char file_list[BUFFER_SIZE * 4] = ""; 
    char temp_list[BUFFER_SIZE * 4] = "";
    int file_count = 0;

    // 1. Gửi danh sách file
    d = opendir(FILE_PATH);
    if (d) {
        while ((dir = readdir(d)) != NULL) {
            // Chỉ lấy file thường, bỏ qua thư mục (. và ..)
            if (dir->d_type == DT_REG) {
                file_count++;
                strcat(temp_list, dir->d_name);
                strcat(temp_list, "\r\n");
            }
        }
        closedir(d);
        
        if (file_count > 0) {
            sprintf(file_list, "OK %d\r\n%s\r\n", file_count, temp_list);
            send(client_socket, file_list, strlen(file_list), 0);
        } else {
            char *err = "ERROR No files to download\r\n";
            send(client_socket, err, strlen(err), 0);
            close(client_socket);
            exit(0);
        }
    } else {
        char *err = "ERROR No files to download\r\n";
        send(client_socket, err, strlen(err), 0);
        close(client_socket);
        exit(0);
    }

    // 2. Vòng lặp chờ nhận tên file từ client (để bắt lỗi nếu gửi sai tên)
    while (1) {
        char file_name[256];
        memset(file_name, 0, sizeof(file_name));
        
        int bytes_received = recv(client_socket, file_name, sizeof(file_name) - 1, 0);
        if (bytes_received <= 0) break; // Client ngắt kết nối

        // Xóa ký tự xuống dòng nếu có
        file_name[strcspn(file_name, "\r\n")] = '\0';

        char full_path[512];
        snprintf(full_path, sizeof(full_path), "%s%s", FILE_PATH, file_name);

        FILE *fp = fopen(full_path, "rb");
        if (fp == NULL) {
            // Nếu file không tồn tại, báo lỗi để client gửi lại
            char *err = "ERROR File not found\r\n";
            send(client_socket, err, strlen(err), 0);
        } else {
            // Lấy kích thước file
            fseek(fp, 0, SEEK_END);
            long file_size = ftell(fp);
            fseek(fp, 0, SEEK_SET);

            // Gửi header báo OK và kích thước
            char response[BUFFER_SIZE];
            snprintf(response, sizeof(response), "OK %ld\r\n", file_size);
            send(client_socket, response, strlen(response), 0);

            // Gửi nội dung file
            char buffer[BUFFER_SIZE];
            size_t read_bytes;
            while ((read_bytes = fread(buffer, 1, sizeof(buffer), fp)) > 0) {
                send(client_socket, buffer, read_bytes, 0);
            }
            fclose(fp);
            break; // Đóng kết nối sau khi gửi xong file thành công
        }
    }
    close(client_socket);
    exit(0);
}

int main() {
    int server_socket, client_socket;
    struct sockaddr_in server_address, client_address;
    socklen_t client_address_length = sizeof(client_address);

    // Ngăn chặn tiến trình zombie khi sử dụng đa tiến trình (fork)
    signal(SIGCHLD, SIG_IGN);

    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket == -1) {
        perror("Error: socket creation failed");
        exit(EXIT_FAILURE);
    }

    int opt = 1;
    setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    server_address.sin_family = AF_INET;
    server_address.sin_addr.s_addr = INADDR_ANY;
    server_address.sin_port = htons(8888);

    if (bind(server_socket, (struct sockaddr *)&server_address, sizeof(server_address)) < 0) {
        perror("Error: bind failed");
        exit(EXIT_FAILURE);
    }

    if (listen(server_socket, 5) < 0) {
        perror("Error: listen failed");
        exit(EXIT_FAILURE);
    }

    printf("Server is listening on port 8888...\n");

    while (1) {
        client_socket = accept(server_socket, (struct sockaddr *)&client_address, &client_address_length);
        if (client_socket < 0) {
            perror("Error: accept failed");
            continue;
        }

        printf("New client connected: %s:%d\n", inet_ntoa(client_address.sin_addr), ntohs(client_address.sin_port));

        // Kỹ thuật Đa tiến trình
        pid_t pid = fork();
        if (pid == 0) {
            // Tiến trình con xử lý client
            close(server_socket); 
            handle_client(client_socket);
        } else if (pid > 0) {
            // Tiến trình cha đóng socket của client (vì con đã giữ) và tiếp tục lắng nghe
            close(client_socket);
        } else {
            perror("Error: fork failed");
        }
    }
    
    close(server_socket);
    return 0;
}
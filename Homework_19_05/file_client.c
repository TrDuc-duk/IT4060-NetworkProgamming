#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#define BUFFER_SIZE 1024

int main() {
    int client_socket;
    struct sockaddr_in server_address;

    client_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (client_socket == -1) {
        perror("Error: socket creation failed");
        exit(EXIT_FAILURE);
    }

    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(8888);

    if (inet_pton(AF_INET, "127.0.0.1", &(server_address.sin_addr)) <= 0) {
        perror("Error: invalid server address");
        exit(EXIT_FAILURE);
    }

    if (connect(client_socket, (struct sockaddr *)&server_address, sizeof(server_address)) < 0) {
        perror("Error: connection failed");
        exit(EXIT_FAILURE);
    }

    // 1. Nhận danh sách file từ server
    char file_list[BUFFER_SIZE * 4];
    memset(file_list, 0, sizeof(file_list));
    
    // Đọc thông báo danh sách
    ssize_t read_bytes = recv(client_socket, file_list, sizeof(file_list) - 1, 0);
    if (read_bytes <= 0) {
        perror("Error: receive failed");
        close(client_socket);
        return 1;
    }
    file_list[read_bytes] = '\0';

    if (strncmp(file_list, "ERROR", 5) == 0) {
        printf("Server response: %s", file_list);
        close(client_socket);
        return 0; // Nghỉ nếu không có file
    } else {
        printf("Server response:\n%s\n", file_list);
    }

    // 2. Vòng lặp yêu cầu file
    while (1) {
        char file_name[100];
        printf("Enter file name to download: ");
        fgets(file_name, sizeof(file_name), stdin);
        file_name[strcspn(file_name, "\n")] = '\0'; // Xóa '\n'

        // Gửi tên file đi
        send(client_socket, file_name, strlen(file_name), 0);

        // Đọc phần Header phản hồi từ server (đọc từng byte cho đến khi thấy \n để không lẹm data)
        char header[256];
        memset(header, 0, sizeof(header));
        int header_len = 0;
        char c;
        while (recv(client_socket, &c, 1, 0) > 0) {
            header[header_len++] = c;
            if (c == '\n') break;
        }

        if (strncmp(header, "ERROR", 5) == 0) {
            // Nếu lỗi báo file không tồn tại, in ra và cho phép nhập lại
            printf("Server response: %s", header);
            continue; 
        } else if (strncmp(header, "OK", 2) == 0) {
            long file_size = 0;
            sscanf(header, "OK %ld", &file_size);
            
            FILE *fp = fopen(file_name, "wb");
            if (fp != NULL) {
                long remaining_bytes = file_size;
                char data[BUFFER_SIZE];
                
                // Nhận đủ số byte đúng với file_size
                while (remaining_bytes > 0) {
                    int to_read = (remaining_bytes < BUFFER_SIZE) ? remaining_bytes : BUFFER_SIZE;
                    ssize_t bytes_recv = recv(client_socket, data, to_read, 0);
                    
                    if (bytes_recv <= 0) break;
                    fwrite(data, 1, bytes_recv, fp);
                    remaining_bytes -= bytes_recv;
                }
                fclose(fp);
                printf("File \"%s\" downloaded successfully.\n", file_name);
            } else {
                perror("Error: file creation failed");
            }
            break; // Thoát vòng lặp khi đã nhận xong file hợp lệ
        }
    }

    close(client_socket);
    return 0;
}
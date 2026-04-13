#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/select.h>

#define MAX_CLIENTS 10
#define BUFFER_SIZE 2048

int main() {
    int server_socket, client_socket[MAX_CLIENTS];
    int is_logged_in[MAX_CLIENTS]; // Mảng đánh dấu trạng thái đăng nhập
    struct sockaddr_in server_address;
    int max_clients = MAX_CLIENTS;
    int activity, i, valread, sd, max_sd;
    char buffer[BUFFER_SIZE];
    fd_set readfds;

    for (i = 0; i < max_clients; i++) {
        client_socket[i] = 0;
        is_logged_in[i] = 0;
    }

    if ((server_socket = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("Lỗi tạo socket");
        exit(EXIT_FAILURE);
    }

    server_address.sin_family = AF_INET;
    server_address.sin_addr.s_addr = INADDR_ANY;
    server_address.sin_port = htons(8080);

    if (bind(server_socket, (struct sockaddr *)&server_address, sizeof(server_address)) < 0) {
        perror("Lỗi bind");
        exit(EXIT_FAILURE);
    }

    if (listen(server_socket, 5) < 0) {
        perror("Lỗi listen");
        exit(EXIT_FAILURE);
    }

    printf("Telnet server đang chạy trên port 8080...\n");

    while (1) {
        FD_ZERO(&readfds);
        FD_SET(server_socket, &readfds);
        max_sd = server_socket;

        for (i = 0; i < max_clients; i++) {
            sd = client_socket[i];
            if (sd > 0) FD_SET(sd, &readfds);
            if (sd > max_sd) max_sd = sd;
        }

        activity = select(max_sd + 1, &readfds, NULL, NULL, NULL);

        if (FD_ISSET(server_socket, &readfds)) {
            struct sockaddr_in client_addr;
            int addrlen = sizeof(client_addr);
            int new_socket = accept(server_socket, (struct sockaddr *)&client_addr, (socklen_t *)&addrlen);
            
            printf("Kết nối mới: %s:%d\n", inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));

            for (i = 0; i < max_clients; i++) {
                if (client_socket[i] == 0) {
                    client_socket[i] = new_socket;
                    is_logged_in[i] = 0;
                    break;
                }
            }
        }

        for (i = 0; i < max_clients; i++) {
            sd = client_socket[i];
            if (FD_ISSET(sd, &readfds)) {
                memset(buffer, 0, BUFFER_SIZE);
                valread = read(sd, buffer, BUFFER_SIZE);

                if (valread == 0) {
                    printf("Client ngắt kết nối.\n");
                    close(sd);
                    client_socket[i] = 0;
                    is_logged_in[i] = 0;
                } else {
                    buffer[strcspn(buffer, "\n\r")] = 0; // Xóa ký tự xuống dòng

                    if (is_logged_in[i] == 0) {
                        // Xử lý đăng nhập
                        char user[50], pass[50];
                        if (sscanf(buffer, "%s %s", user, pass) == 2) {
                            FILE *f = fopen("database.txt", "r");
                            char db_user[50], db_pass[50];
                            int found = 0;
                            while (fscanf(f, "%s %s", db_user, db_pass) != EOF) {
                                if (strcmp(user, db_user) == 0 && strcmp(pass, db_pass) == 0) {
                                    found = 1; break;
                                }
                            }
                            fclose(f);

                            if (found) {
                                is_logged_in[i] = 1;
                                send(sd, "OK\n", 3, 0);
                            } else {
                                send(sd, "FAILED\n", 7, 0);
                            }
                        } else {
                            send(sd, "Dinh dang: [user] [pass]\n", 25, 0);
                        }
                    } else {
                        // Đã đăng nhập -> Thực thi lệnh
                        char cmd[BUFFER_SIZE + 20];
                        sprintf(cmd, "%s > out.txt 2>&1", buffer); // Chạy lệnh và lưu vào out.txt
                        system(cmd);

                        // Đọc file out.txt gửi lại cho client
                        FILE *f = fopen("out.txt", "r");
                        if (f) {
                            char file_buf[BUFFER_SIZE];
                            size_t n = fread(file_buf, 1, sizeof(file_buf), f);
                            if (n > 0) send(sd, file_buf, n, 0);
                            else send(sd, "Lenh da thuc thi (khong co ket qua).\n", 37, 0);
                            fclose(f);
                        }
                    }
                }
            }
        }
    }
    return 0;
}
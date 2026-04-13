#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <sys/select.h>
#include <time.h> // Sử dụng để định dạng thời gian

#define PORT 8888 
#define BACKLOG 5 

int main(int argc, char *argv[]) {
    fd_set masterfds, readfds;
    int maxfds;
    int listener, newfd;
    struct sockaddr_in serveraddr, clientaddr;
    socklen_t addrlen;
    char buffer[1024];
    int nbytes;

    // Cấu trúc mảng quản lý trạng thái của từng socket
    int is_registered[FD_SETSIZE];
    char client_ids[FD_SETSIZE][50];
    char client_names[FD_SETSIZE][50];

    // Khởi tạo trạng thái chưa đăng ký
    memset(is_registered, 0, sizeof(is_registered));

    // Khởi tạo socket
    listener = socket(AF_INET, SOCK_STREAM, 0);
    
    // Khử lỗi "Address already in use" khi khởi động lại server
    int opt = 1;
    setsockopt(listener, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    // Thiết lập địa chỉ và cổng
    serveraddr.sin_family = AF_INET;
    serveraddr.sin_addr.s_addr = INADDR_ANY;
    serveraddr.sin_port = htons(PORT);

    // Gán địa chỉ và cổng cho socket
    if (bind(listener, (struct sockaddr *)&serveraddr, sizeof(serveraddr)) < 0) {
        perror("bind");
        exit(EXIT_FAILURE);
    }

    // Chờ đợi kết nối
    if (listen(listener, BACKLOG) < 0) {
        perror("listen");
        exit(EXIT_FAILURE);
    }

    FD_ZERO(&masterfds);
    FD_SET(listener, &masterfds);
    maxfds = listener;

    printf("Chat Server dang chay tren cong %d...\n", PORT);

    while(1) {
        readfds = masterfds;
        // Kiểm tra tập file descriptors
        if (select(maxfds + 1, &readfds, NULL, NULL, NULL) == -1) {
            perror("select");
            exit(EXIT_FAILURE);
        }

        // Duyệt qua tất cả các kết nối hiện tại để tìm nơi có dữ liệu
        for (int i = 0; i <= maxfds; i++) {
            if (FD_ISSET(i, &readfds)) {
                if (i == listener) {
                    // CÓ KẾT NỐI MỚI ĐẾN SERVER
                    addrlen = sizeof(clientaddr);
                    newfd = accept(listener, (struct sockaddr *)&clientaddr, &addrlen);

                    if (newfd < 0) {
                        perror("accept");
                    } else {
                        printf("Co ket noi moi tu %s tren socket %d\n", inet_ntoa(clientaddr.sin_addr), newfd);
                        FD_SET(newfd, &masterfds); // Thêm vào tập master
                        if (newfd > maxfds) {
                            maxfds = newfd;
                        }

                        // Reset trạng thái của socket này
                        is_registered[newfd] = 0;

                        // Gửi yêu cầu nhập tên chuẩn
                        char *greeting = "Vui long nhap ten theo cu phap: 'client_id: client_name'\n";
                        send(newfd, greeting, strlen(greeting), 0);
                    }
                } else {
                    // CÓ DỮ LIỆU TỪ CLIENT HIỆN CÓ
                    memset(buffer, 0, sizeof(buffer));
                    nbytes = recv(i, buffer, sizeof(buffer) - 1, 0);

                    if (nbytes <= 0) {
                        // Nhận lỗi hoặc client đóng kết nối (nbytes == 0)
                        if (nbytes == 0) {
                            printf("Socket %d da ngat ket noi.\n", i);
                        } else {
                            perror("recv");
                        }
                        close(i);
                        FD_CLR(i, &masterfds); // Xóa khỏi tập master
                        is_registered[i] = 0;  // Xóa trạng thái đăng ký
                    } else {
                        // Xóa các ký tự xuống dòng ở cuối thông điệp
                        buffer[strcspn(buffer, "\r\n")] = '\0';

                        if (!is_registered[i]) {
                            // BƯỚC 1: Xử lý định dạng tên
                            char temp_id[50], temp_name[50];
                            // Cắt chuỗi theo định dạng "%[^:]: %s" 
                            // Lấy ID tới khi gặp ':', và Tên là chuỗi liền mạch (không dấu cách)
                            if (sscanf(buffer, "%[^:]: %s", temp_id, temp_name) == 2) {
                                strcpy(client_ids[i], temp_id);
                                strcpy(client_names[i], temp_name);
                                is_registered[i] = 1;
                                
                                char success_msg[100];
                                sprintf(success_msg, "Dang ky thanh cong. Chao mung %s!\n", temp_name);
                                send(i, success_msg, strlen(success_msg), 0);
                                printf("Client socket %d xac nhan: ID = %s, Name = %s\n", i, temp_id, temp_name);
                            } else {
                                char *err_msg = "Sai cu phap! Vui long nhap 'client_id: client_name':\n";
                                send(i, err_msg, strlen(err_msg), 0);
                            }
                        } else {
                            // BƯỚC 2: Phát sóng (Broadcast) tin nhắn
                            char out_buffer[2048];
                            time_t t = time(NULL);
                            struct tm *tm = localtime(&t);
                            char time_str[64];
                            
                            // Định dạng: YYYY/MM/DD hh:mm:ssPM
                            strftime(time_str, sizeof(time_str), "%Y/%m/%d %I:%M:%S%p", tm);

                            // Format nội dung xuất ra cho các client khác
                            snprintf(out_buffer, sizeof(out_buffer), "%s %s: %s\n", time_str, client_ids[i], buffer);
                            printf("Phat song tu %s: %s\n", client_ids[i], buffer); // In ra log ở server

                            // Gửi tới tất cả các client khác đã đăng ký
                            for (int j = 0; j <= maxfds; j++) {
                                if (FD_ISSET(j, &masterfds)) {
                                    // Không gửi lại cho listener (server), cho chính người gửi (i), và người chưa gửi ID
                                    if (j != listener && j != i && is_registered[j]) {
                                        send(j, out_buffer, strlen(out_buffer), 0);
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }
    return 0;
}
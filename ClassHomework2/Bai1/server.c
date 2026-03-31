#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <ctype.h>

#define PORT 8080
#define MAX_CLIENTS 10
#define BUFFER_SIZE 256

// Trạng thái của client
typedef struct {
    int socket;
    int state; // 0: chờ nhập tên, 1: chờ nhập MSSV
    char name[BUFFER_SIZE];
    char mssv[BUFFER_SIZE];
} ClientState;

// Hàm tạo email theo chuẩn HUST: Tên.HọĐệm+6_số_cuối_MSSV@sis.hust.edu.vn
void generate_hust_email(char* name, char* mssv, char* email) {
    // Xóa ký tự xuống dòng
    name[strcspn(name, "\r\n")] = 0;
    mssv[strcspn(mssv, "\r\n")] = 0;

    char *words[20];
    int word_count = 0;
    
    // Tách chuỗi họ tên bằng khoảng trắng
    char *token = strtok(name, " ");
    while (token != NULL && word_count < 20) {
        words[word_count++] = token;
        token = strtok(NULL, " ");
    }

    if (word_count == 0) {
        strcpy(email, "Email cua ban: khong_hop_le@sis.hust.edu.vn\n");
        return;
    }

    // 1. Xử lý Tên (Từ cuối cùng)
    char first_name[50] = {0};
    strcpy(first_name, words[word_count - 1]);
    if (strlen(first_name) > 0) {
        first_name[0] = toupper(first_name[0]); // Viết hoa chữ đầu
        for (int i = 1; first_name[i]; i++) {
            first_name[i] = tolower(first_name[i]); // Viết thường các chữ sau
        }
    }

    // 2. Xử lý Họ và Đệm (Lấy chữ cái đầu, viết hoa)
    char initials[20] = {0};
    for (int i = 0; i < word_count - 1; i++) {
        initials[i] = toupper(words[i][0]);
    }
    initials[word_count - 1] = '\0';

    // 3. Xử lý MSSV (Lấy 6 số cuối)
    char short_mssv[20] = {0};
    int mssv_len = strlen(mssv);
    if (mssv_len >= 6) {
        strcpy(short_mssv, mssv + mssv_len - 6);
    } else {
        strcpy(short_mssv, mssv); // Nếu MSSV nhập vào ngắn hơn 6 số, lấy nguyên
    }

    // 4. Ghép lại thành email hoàn chỉnh
    sprintf(email, "\n=> Email cua ban: %s.%s%s@sis.hust.edu.vn\n\n", first_name, initials, short_mssv);
}

int main() {
    int master_socket, addrlen, new_socket, activity, valread;
    struct sockaddr_in address;
    ClientState clients[MAX_CLIENTS];
    fd_set readfds;
    char buffer[BUFFER_SIZE];

    // Khởi tạo mảng client
    for (int i = 0; i < MAX_CLIENTS; i++) {
        clients[i].socket = 0;
    }

    if ((master_socket = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("Socket failed");
        exit(EXIT_FAILURE);
    }

    // Thiết lập tùy chọn cho socket để có thể sử dụng lại port ngay lập tức (tránh lỗi "Address already in use")
    int opt = 1;
    if (setsockopt(master_socket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt))) {
        perror("setsockopt");
        exit(EXIT_FAILURE);
    }

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    if (bind(master_socket, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("Bind failed");
        exit(EXIT_FAILURE);
    }

    if (listen(master_socket, 5) < 0) {
        perror("Listen failed");
        exit(EXIT_FAILURE);
    }

    printf("Server dang lang nghe tren port %d...\n", PORT);
    addrlen = sizeof(address);

    while (1) {
        FD_ZERO(&readfds);
        FD_SET(master_socket, &readfds);
        int max_sd = master_socket;

        for (int i = 0; i < MAX_CLIENTS; i++) {
            int sd = clients[i].socket;
            if (sd > 0) FD_SET(sd, &readfds);
            if (sd > max_sd) max_sd = sd;
        }

        // Chờ sự kiện
        activity = select(max_sd + 1, &readfds, NULL, NULL, NULL);
        if (activity < 0) printf("Loi select\n");

        // Xử lý kết nối mới
        if (FD_ISSET(master_socket, &readfds)) {
            if ((new_socket = accept(master_socket, (struct sockaddr *)&address, (socklen_t*)&addrlen)) < 0) {
                perror("Accept failed");
                exit(EXIT_FAILURE);
            }
            
            printf("Client moi ket noi: FD %d\n", new_socket);
            
            char *msg = "Xin chao! Vui long nhap Ho Ten cua ban (Khong dau): ";
            send(new_socket, msg, strlen(msg), 0);

            for (int i = 0; i < MAX_CLIENTS; i++) {
                if (clients[i].socket == 0) {
                    clients[i].socket = new_socket;
                    clients[i].state = 0; 
                    break;
                }
            }
        }

        // Xử lý dữ liệu từ các client cũ
        for (int i = 0; i < MAX_CLIENTS; i++) {
            int sd = clients[i].socket;
            if (FD_ISSET(sd, &readfds)) {
                valread = read(sd, buffer, BUFFER_SIZE - 1);
                
                if (valread == 0) {
                    printf("Client FD %d ngat ket noi.\n", sd);
                    close(sd);
                    clients[i].socket = 0;
                } else {
                    buffer[valread] = '\0';
                    
                    if (clients[i].state == 0) {
                        strncpy(clients[i].name, buffer, BUFFER_SIZE);
                        clients[i].state = 1; // Chuyển sang chờ MSSV
                        char *msg = "Vui long nhap MSSV cua ban: ";
                        send(sd, msg, strlen(msg), 0);
                        
                    } else if (clients[i].state == 1) {
                        strncpy(clients[i].mssv, buffer, BUFFER_SIZE);
                        
                        char email_response[BUFFER_SIZE * 2];
                        // Gọi hàm chuẩn hóa theo format HUST
                        generate_hust_email(clients[i].name, clients[i].mssv, email_response);
                        
                        send(sd, email_response, strlen(email_response), 0);
                        
                        close(sd);
                        clients[i].socket = 0;
                        printf("Da phuc vu xong Client FD %d.\n", sd);
                    }
                }
            }
        }
    }
    return 0;
}
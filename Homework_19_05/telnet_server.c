#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#define BUFFER_SIZE 2048
#define DB_FILE "users.txt"

// Hàm kiểm tra thông tin đăng nhập từ file txt
int authenticate(const char *user, const char *pass) {
    FILE *fp = fopen(DB_FILE, "r");
    if (!fp) {
        perror("Loi: Khong the mo file CSDL users.txt");
        return 0; // Trả về 0 nếu không có file
    }

    char f_user[50], f_pass[50];
    // Đọc từng dòng: định dạng "user pass"
    while (fscanf(fp, "%49s %49s", f_user, f_pass) != EOF) {
        if (strcmp(user, f_user) == 0 && strcmp(pass, f_pass) == 0) {
            fclose(fp);
            return 1; // Đúng tài khoản
        }
    }

    fclose(fp);
    return 0; // Sai tài khoản
}

// Luồng xử lý cho mỗi Client
void *handle_client(void *arg) {
    int sock = *(int *)arg;
    free(arg); // Giải phóng con trỏ truyền vào
    char buffer[BUFFER_SIZE];
    char user[50], pass[50];

    // --- GIAI ĐOẠN 1: XÁC THỰC ---
    while (1) {
        char *prompt = "Yeu cau dang nhap (Cu phap: user pass): ";
        send(sock, prompt, strlen(prompt), 0);
        
        memset(buffer, 0, sizeof(buffer));
        int bytes = recv(sock, buffer, sizeof(buffer) - 1, 0);
        if (bytes <= 0) {
            close(sock);
            return NULL;
        }

        // Tách user và pass
        if (sscanf(buffer, "%49s %49s", user, pass) == 2) {
            if (authenticate(user, pass)) {
                char *success = "Dang nhap thanh cong!\n";
                send(sock, success, strlen(success), 0);
                break; // Thoát vòng lặp xác thực, chuyển sang nhận lệnh
            } else {
                char *fail = "Sai tai khoan hoac mat khau. Vui long thu lai!\n";
                send(sock, fail, strlen(fail), 0);
            }
        } else {
            char *err = "Sai cu phap. Nhap lai (Vi du: admin admin)\n";
            send(sock, err, strlen(err), 0);
        }
    }

    // --- GIAI ĐOẠN 2: THỰC THI LỆNH (TELNET) ---
    while (1) {
        char *prompt = "telnet> ";
        send(sock, prompt, strlen(prompt), 0);

        memset(buffer, 0, sizeof(buffer));
        int bytes = recv(sock, buffer, sizeof(buffer) - 1, 0);
        if (bytes <= 0) break; // Client ngắt kết nối

        buffer[strcspn(buffer, "\r\n")] = '\0'; // Xóa dấu xuống dòng
        if (strlen(buffer) == 0) continue;

        if (strcmp(buffer, "exit") == 0 || strcmp(buffer, "quit") == 0) {
            send(sock, "Tam biet!\n", 10, 0);
            break;
        }

        // Tạo tên file output độc lập cho từng luồng dựa trên Socket ID
        char out_file[64];
        snprintf(out_file, sizeof(out_file), "out_%d.txt", sock);

        // Chèn lệnh điều hướng dữ liệu ra file tạm
        // Mẹo: Thêm '2>&1' để bắt cả lỗi gõ sai lệnh vào file (stderr -> stdout)
        char sys_cmd[BUFFER_SIZE + 100];
        snprintf(sys_cmd, sizeof(sys_cmd), "%s > %s 2>&1", buffer, out_file);

        // Gọi hệ điều hành thực thi
        system(sys_cmd);

        // Đọc kết quả từ file out.txt gửi lại cho Client
        FILE *fp = fopen(out_file, "r");
        if (fp) {
            char file_buf[BUFFER_SIZE];
            size_t read_bytes;
            int has_data = 0;
            
            while ((read_bytes = fread(file_buf, 1, sizeof(file_buf), fp)) > 0) {
                send(sock, file_buf, read_bytes, 0);
                has_data = 1;
            }
            fclose(fp);
            remove(out_file); // Xóa file tạm ngay sau khi dùng xong
            
            if (!has_data) {
                // Nếu lệnh chạy đúng nhưng không sinh ra text (ví dụ: mkdir test)
                send(sock, "\n", 1, 0);
            }
        } else {
            char *err_msg = "Loi: Khong the doc ket qua lenh.\n";
            send(sock, err_msg, strlen(err_msg), 0);
        }
    }

    printf("Socket %d disconnected.\n", sock);
    close(sock);
    return NULL;
}

int main() {
    int server_sock = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in addr = {AF_INET, htons(8888), INADDR_ANY};

    int opt = 1;
    setsockopt(server_sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    bind(server_sock, (struct sockaddr *)&addr, sizeof(addr));
    listen(server_sock, 10);

    printf("Telnet Server dang chay tren port 8888...\n");

    while (1) {
        int client_sock = accept(server_sock, NULL, NULL);
        if (client_sock < 0) continue;

        printf("New connection: Socket %d\n", client_sock);

        // Cấp phát vùng nhớ động chứa socket fd truyền vào luồng
        int *arg = malloc(sizeof(int));
        *arg = client_sock;

        pthread_t tid;
        pthread_create(&tid, NULL, handle_client, arg);
        pthread_detach(tid);
    }

    close(server_sock);
    return 0;
}
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <poll.h>

#define MAX_FDS 2048
#define BUFFER_SIZE 1024

// Cấu trúc lưu trữ thông tin tài khoản
typedef struct {
    char username[50];
    char password[50];
} Account;

// Hàm đọc dữ liệu từ file cơ sở dữ liệu
int read_database(Account *database, const char *filename) {
    FILE *file = fopen(filename, "r");
    if (!file) {
        perror("Error opening database file");
        return -1;
    }

    int count = 0;
    while (fscanf(file, "%s %s", database[count].username, database[count].password) != EOF) {
        count++;
    }

    fclose(file);
    return count;
}

// Hàm loại bỏ ký tự xuống dòng
void trim_newline(char *str) {
    int len = strlen(str);
    while (len > 0 && (str[len - 1] == '\n' || str[len - 1] == '\r')) {
        str[len - 1] = '\0';
        len--;
    }
}

int main() {
    Account database[MAX_FDS];
    int num_accounts = read_database(database, "database.txt");
    if (num_accounts == -1) {
        printf("Please create 'database.txt' with format: user pass\\n");
        return 1;
    }

    int listener = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (listener == -1) {
        perror("socket() failed");
        return 1;
    }

    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(8000);

    if (bind(listener, (struct sockaddr *)&addr, sizeof(addr))) {
        perror("bind() failed");
        return 1;
    }

    if (listen(listener, 5)) {
        perror("listen() failed");
        return 1;
    }

    struct pollfd fds[MAX_FDS];
    int nfds = 0;

    fds[0].fd = listener;
    fds[0].events = POLLIN;
    nfds++;

    // Mảng lưu trạng thái đăng nhập: 0 là chưa đăng nhập, 1 là đã đăng nhập
    int client_log[MAX_FDS];
    for(int i = 0; i < MAX_FDS; i++) client_log[i] = 0;
    
    char buf[BUFFER_SIZE];

    printf("Telnet Server is running on port 8000...\n");

    while (1) {
        int ret = poll(fds, nfds, -1);
        if (ret == -1) break;

        for (int i = 0; i < nfds; i++) {
            if (fds[i].revents & POLLIN) {
                if (fds[i].fd == listener) {
                    // KẾT NỐI MỚI
                    int client = accept(listener, NULL, NULL);
                    if (nfds < MAX_FDS) {
                        fds[nfds].fd = client;
                        fds[nfds].events = POLLIN;
                        nfds++;
                        
                        client_log[client] = 0; // Đặt trạng thái chưa đăng nhập

                        printf("New client connected: %d\n", client);
                        char *msg = "Enter username and password (user pass): ";
                        send(client, msg, strlen(msg), 0);
                    } else {
                        close(client);
                    }
                } else {
                    // DỮ LIỆU TỪ CLIENT
                    int client = fds[i].fd;
                    ret = recv(client, buf, sizeof(buf) - 1, 0);
                    
                    if (ret <= 0) {
                        // CLIENT NGẮT KẾT NỐI
                        close(client);
                        fds[i] = fds[nfds - 1];
                        nfds--;
                        i--;
                        client_log[client] = 0;
                        printf("Client %d disconnected.\n", client);
                    } else {
                        buf[ret] = 0;
                        trim_newline(buf); // Xóa ký tự Enter thừa
                        if (strlen(buf) == 0) continue; // Bỏ qua nếu chỉ gõ Enter

                        if (client_log[client] == 0) {
                            // XỬ LÝ ĐĂNG NHẬP
                            char username[50], password[50];
                            if (sscanf(buf, "%s %s", username, password) != 2) {
                                char *error_msg = "Invalid format. Try again: ";
                                send(client, error_msg, strlen(error_msg), 0);
                            } else {
                                int authenticated = 0;
                                for (int j = 0; j < num_accounts; j++) {
                                    if (strcmp(username, database[j].username) == 0 &&
                                        strcmp(password, database[j].password) == 0) {
                                        authenticated = 1;
                                        break;
                                    }
                                }
                                if (!authenticated) {
                                    char *error_msg = "Incorrect username or password. Try again: ";
                                    send(client, error_msg, strlen(error_msg), 0);
                                } else {
                                    client_log[client] = 1;
                                    char *succ_msg = "Login successful! Enter command: \n";
                                    send(client, succ_msg, strlen(succ_msg), 0);
                                }
                            }
                        } else {
                            // XỬ LÝ LỆNH TELNET
                            char cmd[BUFFER_SIZE + 20];
                            snprintf(cmd, sizeof(cmd), "%s > out.txt 2>&1", buf); 
                            // Dùng 2>&1 để lấy cả thông báo lỗi nếu gõ sai lệnh
                            system(cmd);

                            // Đọc và gửi file out.txt
                            FILE *f = fopen("out.txt", "rb");
                            if (f != NULL) {
                                char file_buf[BUFFER_SIZE];
                                int bytes_read;
                                while ((bytes_read = fread(file_buf, 1, sizeof(file_buf), f)) > 0) {
                                    send(client, file_buf, bytes_read, 0);
                                }
                                fclose(f);
                            } else {
                                char *err = "Command executed but cannot read output.\n";
                                send(client, err, strlen(err), 0);
                            }
                            
                            // Gửi dấu nhắc lệnh tiếp theo
                            char *prompt = "\n> ";
                            send(client, prompt, strlen(prompt), 0);
                        }
                    }
                }
            }
        }
    }
    return 0;
}
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <sys/wait.h>
#include <signal.h>

#define BUFFER_SIZE 1024
#define MAX_ACCOUNTS 2048

typedef struct {
    char username[50];
    char password[50];
} Account;

// Đọc dữ liệu từ file cơ sở dữ liệu
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

// Loại bỏ ký tự xuống dòng
void trim_newline(char *str) {
    int len = strlen(str);
    while (len > 0 && (str[len - 1] == '\n' || str[len - 1] == '\r')) {
        str[len - 1] = '\0';
        len--;
    }
}

// Xử lý tín hiệu SIGCHLD để dọn dẹp tiến trình con, tránh lỗi Zombie Process
void sigchld_handler(int s) {
    while(waitpid(-1, NULL, WNOHANG) > 0);
}

int main() {
    Account database[MAX_ACCOUNTS];
    int num_accounts = read_database(database, "database.txt");
    if (num_accounts == -1) {
        printf("Please create 'database.txt' with format: user pass\n");
        return 1;
    }

    int listener = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (listener == -1) {
        perror("socket() failed");
        return 1;
    }

    // Tái sử dụng port tránh lỗi "Address already in use"
    int opt = 1;
    setsockopt(listener, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

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

    // Đăng ký dọn dẹp tiến trình con
    signal(SIGCHLD, sigchld_handler);

    printf("Telnet Server (Multiprocessing) is running on port 8000...\n");

    while (1) {
        int client = accept(listener, NULL, NULL);
        if (client == -1) {
            perror("accept() failed");
            continue;
        }

        printf("New client connected: %d\n", client);

        // Tách tiến trình
        pid_t pid = fork();
        if (pid < 0) {
            perror("fork() failed");
            close(client);
            continue;
        }

        if (pid == 0) {
            // ================= TIẾN TRÌNH CON =================
            close(listener); // Tiến trình con không cần socket lắng nghe
            
            char buf[BUFFER_SIZE];
            int authenticated = 0;

            // Vòng lặp đăng nhập
            while (!authenticated) {
                char *msg = "Enter username and password (user pass): ";
                send(client, msg, strlen(msg), 0);
                
                int ret = recv(client, buf, sizeof(buf) - 1, 0);
                if (ret <= 0) {
                    close(client);
                    exit(0); // Client ngắt kết nối lúc đang đăng nhập
                }
                
                buf[ret] = 0;
                trim_newline(buf);
                if (strlen(buf) == 0) continue;

                char username[50], password[50];
                if (sscanf(buf, "%s %s", username, password) == 2) {
                    for (int j = 0; j < num_accounts; j++) {
                        if (strcmp(username, database[j].username) == 0 &&
                            strcmp(password, database[j].password) == 0) {
                            authenticated = 1;
                            break;
                        }
                    }
                }

                if (!authenticated) {
                    char *error_msg = "Incorrect username or password. Try again.\n";
                    send(client, error_msg, strlen(error_msg), 0);
                } else {
                    char *succ_msg = "Login successful! Enter command:\n> ";
                    send(client, succ_msg, strlen(succ_msg), 0);
                }
            }

            // Vòng lặp nhận và thực thi lệnh
            char out_filename[32];
            sprintf(out_filename, "out_%d.txt", getpid()); // File output riêng biệt cho mỗi process

            while (1) {
                int ret = recv(client, buf, sizeof(buf) - 1, 0);
                if (ret <= 0) {
                    printf("Client %d disconnected.\n", client);
                    break;
                }
                
                buf[ret] = 0;
                trim_newline(buf);
                if (strlen(buf) == 0) continue;
                if (strcmp(buf, "exit") == 0) break;

                // Thực thi lệnh và ghi output vào file
                char cmd[BUFFER_SIZE + 50];
                snprintf(cmd, sizeof(cmd), "%s > %s 2>&1", buf, out_filename);
                system(cmd);

                // Đọc file và gửi về client
                FILE *f = fopen(out_filename, "rb");
                if (f != NULL) {
                    char file_buf[BUFFER_SIZE];
                    int bytes_read;
                    while ((bytes_read = fread(file_buf, 1, sizeof(file_buf), f)) > 0) {
                        send(client, file_buf, bytes_read, 0);
                    }
                    fclose(f);
                    remove(out_filename); // Dọn file rác ngay sau khi đọc
                } else {
                    char *err = "Command executed but cannot read output.\n";
                    send(client, err, strlen(err), 0);
                }

                char *prompt = "\n> ";
                send(client, prompt, strlen(prompt), 0);
            }

            close(client);
            exit(0); // Kết thúc tiến trình con
            // ==================================================
        }

        // ================= TIẾN TRÌNH CHA =================
        close(client); // Cha giao việc cho con rồi nên đóng socket giao tiếp này lại
    }

    close(listener);
    return 0;
}
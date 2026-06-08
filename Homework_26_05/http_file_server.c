#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <string.h>
#include <pthread.h>
#include <signal.h>
#include <sys/wait.h>
#include <dirent.h>
#include <sys/stat.h>

void *client_thread(void *);
const char *get_content_type(const char *file_path);

void signal_handler(int signo) {
    wait(NULL);
}

// Hàm giải mã URL (vd: đổi "%20" thành dấu cách để đọc đúng tên file)
void urldecode(const char *src, char *dest) {
    while (*src) {
        if (*src == '%' && *(src + 1) && *(src + 2)) {
            char hex[3] = {src[1], src[2], 0};
            *dest++ = (char)strtol(hex, NULL, 16);
            src += 3;
        } else if (*src == '+') {
            *dest++ = ' ';
            src++;
        } else {
            *dest++ = *src++;
        }
    }
    *dest = '\0';
}

void send_folder(int client, const char *folder_path) {
    DIR *dir = opendir(folder_path);
    if (dir == NULL) {
        char *response = "HTTP/1.1 404 Not Found\r\nContent-Type: text/html\r\n\r\n<html><body><h1>Folder Not Found</h1></body></html>";
        send(client, response, strlen(response), 0);
        return;
    }

    char *response_header = "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\nConnection: close\r\n\r\n<html><body><h2>Directory Listing</h2>";
    send(client, response_header, strlen(response_header), 0);

    struct dirent *entry;
    struct stat file_stat;
    char buffer[2048];
    char subfolder_path[1024];

    // Nút quay lại (nếu cần)
    sprintf(buffer, "<p><b><a href=\"../\">../ (Parent Directory)</a></b></p>");
    send(client, buffer, strlen(buffer), 0);

    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) continue;

        // Tạo đường dẫn đầy đủ để dùng hàm stat() kiểm tra loại file
        sprintf(subfolder_path, "%s/%s", folder_path, entry->d_name);
        stat(subfolder_path, &file_stat);

        if (S_ISDIR(file_stat.st_mode)) {
            // Thư mục: In đậm <b>
            sprintf(buffer, "<p><b><a href=\"%s/\">%s/</a></b></p>", entry->d_name, entry->d_name);
        } else {
            // File: In nghiêng <i>
            sprintf(buffer, "<p><i><a href=\"%s\">%s</a></i></p>", entry->d_name, entry->d_name);
        }
        send(client, buffer, strlen(buffer), 0);
    }
    closedir(dir);

    char *response_footer = "</body></html>";
    send(client, response_footer, strlen(response_footer), 0);
}

void send_file(int client, const char *file_path) {
    FILE *file = fopen(file_path, "rb");
    if (file == NULL) {
        char *response = "HTTP/1.1 404 Not Found\r\nContent-Type: text/html\r\n\r\n<html><body><h1>File Not Found</h1></body></html>";
        send(client, response, strlen(response), 0);
        return;
    }

    char response_header[2048];
    sprintf(response_header, "HTTP/1.1 200 OK\r\nContent-Type: %s\r\nConnection: close\r\n\r\n", get_content_type(file_path));
    send(client, response_header, strlen(response_header), 0);

    char buffer[4096];
    size_t bytesRead;
    while ((bytesRead = fread(buffer, 1, sizeof(buffer), file)) > 0) {
        send(client, buffer, bytesRead, 0);
    }
    fclose(file);
}

const char *get_content_type(const char *file_path) {
    const char *extension = strrchr(file_path, '.');
    if (extension != NULL) {
        if (strcmp(extension, ".txt") == 0 || strcmp(extension, ".c") == 0 || strcmp(extension, ".cpp") == 0) return "text/plain";
        else if (strcmp(extension, ".html") == 0) return "text/html";
        else if (strcmp(extension, ".jpg") == 0 || strcmp(extension, ".jpeg") == 0) return "image/jpeg";
        else if (strcmp(extension, ".png") == 0) return "image/png";
        else if (strcmp(extension, ".mp3") == 0) return "audio/mpeg";
        else if (strcmp(extension, ".wav") == 0) return "audio/wav";
        else if (strcmp(extension, ".mp4") == 0) return "video/mp4";
    }
    return "application/octet-stream"; // Tải file xuống nếu không nhận diện được định dạng
}

void *client_thread(void *param) {
    int client = *(int *)param;
    char buf[2048];

    int ret = recv(client, buf, sizeof(buf) - 1, 0);
    if (ret <= 0) {
        close(client);
        return NULL;
    }

    buf[ret] = 0;
    
    char method[16], path[256], decoded_path[256];
    sscanf(buf, "%s %s", method, path);
    urldecode(path, decoded_path); // Giải mã %20 thành khoảng trắng

    char current_dir[256];
    getcwd(current_dir, sizeof(current_dir));
    char file_path[1024];
    
    // Kết hợp đường dẫn hiện tại và URL request
    sprintf(file_path, "%s%s", current_dir, decoded_path);

    struct stat file_stat;
    if (stat(file_path, &file_stat) == 0) {
        if (S_ISDIR(file_stat.st_mode)) {
            send_folder(client, file_path);
        } else if (S_ISREG(file_stat.st_mode)) {
            send_file(client, file_path);
        }
    } else {
        char *response = "HTTP/1.1 404 Not Found\r\nContent-Type: text/html\r\n\r\n<html><body><h1>404 Not Found</h1></body></html>";
        send(client, response, strlen(response), 0);
    }

    close(client);
    return NULL;
}

int main() {
    int listener = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    
    int opt = 1;
    setsockopt(listener, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)); // Chống kẹt port

    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(9000);

    if (bind(listener, (struct sockaddr *)&addr, sizeof(addr))) {
        perror("bind() failed");
        return 1;
    }

    if (listen(listener, 5)) {
        perror("listen() failed");
        return 1;
    }

    signal(SIGPIPE, SIG_IGN); // Bỏ qua lỗi kết nối ngắt đột ngột (rất hay xảy ra khi dùng trình duyệt)

    printf("File Server đang chạy tại cổng 9000...\n");

    while (1) {
        int client = accept(listener, NULL, NULL);
        if (client == -1) continue;

        pthread_t thread_id;
        // Cấp phát động để tránh lỗi race-condition trên biến client
        int *client_ptr = malloc(sizeof(int));
        *client_ptr = client;
        
        pthread_create(&thread_id, NULL, client_thread, client_ptr);
        pthread_detach(thread_id);
    }

    close(listener);
    return 0;
}
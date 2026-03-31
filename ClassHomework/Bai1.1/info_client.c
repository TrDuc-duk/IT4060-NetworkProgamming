#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <string.h> 
#include <arpa/inet.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/stat.h>

int main() {
    int client = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    addr.sin_port = htons(9000);

    if (connect(client, (struct sockaddr *)&addr, sizeof(addr)) == -1) {
        perror("connect() failed");
        return 1;
    }

    char buf[2048]; 
    char path[256];
    int pos = 0;

    getcwd(path, sizeof(path));
    printf("Current path: %s\n", path);

    // 1. Đóng gói tên thư mục (kết thúc bằng \n)
    strcpy(buf + pos, path);
    pos += strlen(path);
    buf[pos] = '\n';
    pos++;

    DIR *d = opendir(path);
    struct dirent *dir;
    struct stat st;

    if (d != NULL) {
        while ((dir = readdir(d)) != NULL) {
            // Chỉ lấy các tập tin thông thường (regular files)
            if (dir->d_type == DT_REG) {
                if (stat(dir->d_name, &st) == 0) {
                    printf("%s - %ld bytes\n", dir->d_name, st.st_size);

                    // 2. Đóng gói tên file (kết thúc bằng \0 để server dễ đọc)
                    int len = strlen(dir->d_name);
                    strcpy(buf + pos, dir->d_name);
                    pos += len;
                    buf[pos] = '\0'; // Dùng null-terminator để phân tách tên
                    pos++;

                    // 3. Đóng gói kích thước (dạng nhị phân - nhỏ nhất)
                    long size = (long)st.st_size;
                    memcpy(buf + pos, &size, sizeof(size));
                    pos += sizeof(size);
                }
            }
        }
        closedir(d);
    }

    // Gửi toàn bộ gói tin
    send(client, buf, pos, 0);
    printf("Total bytes sent: %d\n", pos);

    close(client);
    return 0;
}
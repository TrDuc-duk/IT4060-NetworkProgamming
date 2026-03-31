#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netdb.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <dirent.h>
#include <sys/stat.h>

int main(int argc, char *argv[]) {
    if (argc < 3) {
        printf("Cach dung: %s <IP_Server> <Port>\n", argv[0]);
        return 1;
    }

    int client = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    
    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = inet_addr(argv[1]);
    addr.sin_port = htons(atoi(argv[2]));

    int res = connect(client, (struct sockaddr *)&addr, sizeof(addr));
    if (res == -1) {
        printf("Khong ket noi duoc den server!\n");
        return 1;
    }

    // 1. Lấy đường dẫn thư mục hiện tại
    char cwd[1024];
    if (getcwd(cwd, sizeof(cwd)) == NULL) {
        perror("Loi getcwd");
        return 1;
    }

    char buf[8192] = ""; 
    char temp[512];

    // 2. Nối đường dẫn thư mục vào buffer trước tiên, kèm theo dấu "|" để ngăn cách
    sprintf(buf, "%s|", cwd);

    // 3. Đọc thư mục và nối tiếp danh sách file vào buffer
    DIR *d = opendir(".");
    struct dirent *dir;
    struct stat st;
    
    if (d) {
        while ((dir = readdir(d)) != NULL) {
            if (strcmp(dir->d_name, ".") == 0 || strcmp(dir->d_name, "..") == 0) continue;

            if (stat(dir->d_name, &st) == 0) {
                if (S_ISREG(st.st_mode)) {
                    // Nối tên file và kích thước (ví dụ: bai1.c:12871 )
                    sprintf(temp, "%s:%ld ", dir->d_name, st.st_size);
                    strcat(buf, temp);
                }
            }
        }
        closedir(d);
    }

    // Gửi đi toàn bộ dữ liệu
    send(client, buf, strlen(buf), 0);
    printf("Da gui du lieu thu muc va file len server.\n");

    close(client);
    return 0;
}
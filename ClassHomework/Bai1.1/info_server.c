#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

int main() {
    int listener = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (listener == -1) {
        perror("socket() failed");
        return 1;
    }

    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(9000);
    
    // SỬA LỖI ÉP KIỂU TẠI ĐÂY
    if (bind(listener, (struct sockaddr *)&addr, sizeof(addr)) == -1) {
        perror("bind() failed");
        return 1;
    }

    if (listen(listener, 5) == -1) {
        perror("listen() failed");
        return 1;
    }
    printf("Waiting for connection...\n");

    int client = accept(listener, NULL, NULL);
    if (client == -1) {
        perror("accept() failed");
        return 1;
    }
    printf("Client connected: %d\n", client);

    char buf[2048]; 
    int ret = recv(client, buf, sizeof(buf), 0);
    
    if (ret > 0) {
        int pos = 0;

        // 1. Đọc đường dẫn thư mục (đến ký tự \n)
        printf("Directory: ");
        while (pos < ret && buf[pos] != '\n') {
            putchar(buf[pos]);
            pos++;
        }
        printf("\n------------------------------\n");
        pos++; // Bỏ qua ký tự \n

        // 2. Đọc danh sách file và kích thước
        while (pos < ret) {
            // Đọc tên file (đến khi gặp \0)
            printf("%s - ", buf + pos);
            pos += strlen(buf + pos) + 1; // +1 để nhảy qua ký tự \0

            // Đọc kích thước file 
            if (pos + sizeof(long) <= ret) {
                long filesize;
                memcpy(&filesize, buf + pos, sizeof(long));
                printf("%ld bytes\n", filesize);
                pos += sizeof(long);
            }
        }
    }

    close(client);
    close(listener);
    return 0;
}
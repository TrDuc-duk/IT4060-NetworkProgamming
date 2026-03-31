#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h> 
#include <arpa/inet.h>
#include <unistd.h>

#define BUFFER_SIZE 256 // Dùng buffer 

int main () {
    // 1. Tạo socket
    int client = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (client == -1) {
        perror("socket() failed");
        return 1;
    }

    // 2. Khai báo cấu trúc địa chỉ server
    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    addr.sin_port = htons(9000);

    // 3. Kết nối đến server
    if (connect(client, (struct sockaddr *)&addr, sizeof(addr)) == -1) {
        perror("connect() failed");
        exit(1);
    }

    char buf[BUFFER_SIZE];
    printf("--- San sang gui du lieu ---\n");
    printf("Nhap van ban (Go 'exit' de thoat):\n");

    // 4. Liên tục đọc từ bàn phím và gửi
    while (1) {
        if (fgets(buf, sizeof(buf), stdin) == NULL) {
            break;
        }

        // Loại bỏ ký tự xuống dòng '\n' do phím Enter tạo ra
        // Việc này đảm bảo chuỗi của lần gửi sau nối tiếp chính xác ngay sau lần gửi trước
        int len = strlen(buf);
        if (len > 0 && buf[len - 1] == '\n') {
            buf[len - 1] = '\0';
            len--;
        }

        if (strcmp(buf, "exit") == 0) {
            break;
        }

        if (len > 0) {
            if (send(client, buf, len, 0) == -1) {
                perror("send() failed");
                break;
            }
        }
    }

    printf("Da ngat ket noi!\n");
    close(client);

    return 0;
}
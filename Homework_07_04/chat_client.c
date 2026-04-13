#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <sys/select.h> // Bổ sung thư viện cho client

#define SERVER_ADDRESS "127.0.0.1" 
#define SERVER_PORT 8888 

int main() {
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock == -1) {
        printf("Khong the tao socket.\n");
        exit(1);
    }

    struct sockaddr_in server_address;
    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(SERVER_PORT);
    server_address.sin_addr.s_addr = inet_addr(SERVER_ADDRESS);

    if (connect(sock, (struct sockaddr*)&server_address, sizeof(server_address)) == -1) {
        printf("Khong the ket noi toi may chu chat.\n");
        exit(1);
    }

    printf("Ket noi toi may chu chat thanh cong!\n");

    fd_set readfds;
    char message[1000];

    while (1) {
        FD_ZERO(&readfds);
        FD_SET(STDIN_FILENO, &readfds); // Giám sát bàn phím
        FD_SET(sock, &readfds);         // Giám sát socket từ server

        if (select(sock + 1, &readfds, NULL, NULL, NULL) == -1) {
            perror("select");
            break;
        }

        // CÓ DỮ LIỆU TỪ MÁY CHỦ CHAT (Có người nhắn)
        if (FD_ISSET(sock, &readfds)) {
            memset(message, 0, sizeof(message));
            int bytes_received = recv(sock, message, sizeof(message) - 1, 0);
            
            if (bytes_received <= 0) {
                printf("\nMay chu da dong ket noi.\n");
                break;
            }
            
            // In thông báo từ người khác và khôi phục lại prompt "Bạn: "
            printf("\r\033[K"); // Xóa dòng hiện tại để UI sạch hơn
            printf("%s", message);
            printf("Ban: "); 
            fflush(stdout);
        }

        // NGƯỜI DÙNG GÕ TỪ BÀN PHÍM
        if (FD_ISSET(STDIN_FILENO, &readfds)) {
            memset(message, 0, sizeof(message));
            fgets(message, 1000, stdin);

            if (send(sock, message, strlen(message), 0) == -1) {
                printf("Khong the gui tin nhan den may chu chat.\n");
                break;
            }

            if (strcmp(message, "exit\n") == 0) {
                printf("Da thoat khoi chuong trinh chat.\n");
                break;
            }
            
            printf("Ban: ");
            fflush(stdout);
        }
    }

    close(sock);
    return 0;
}
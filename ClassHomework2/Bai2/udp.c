#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/select.h>

#define BUFFER_SIZE 1024

int main(int argc, char *argv[]) {
    // 1. Kiem tra tham so dong lenh
    if (argc != 4) {
        printf("Cach su dung: %s <port_s> <ip_d> <port_d>\n", argv[0]);
        printf("Vi du: %s 8000 127.0.0.1 8001\n", argv[0]);
        return 1;
    }

    int port_s = atoi(argv[1]);
    char *ip_d = argv[2];
    int port_d = atoi(argv[3]);

    int sockfd;
    struct sockaddr_in my_addr, dest_addr, sender_addr;
    socklen_t sender_len = sizeof(sender_addr);
    char buffer[BUFFER_SIZE];
    fd_set readfds;

    // 2. Tao UDP socket
    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        perror("Loi tao socket");
        exit(EXIT_FAILURE);
    }

    // 3. Cau hinh dia chi cua ung dung hien tai (Lắng nghe trên port_s)
    memset(&my_addr, 0, sizeof(my_addr));
    my_addr.sin_family = AF_INET;
    my_addr.sin_addr.s_addr = INADDR_ANY;
    my_addr.sin_port = htons(port_s);

    if (bind(sockfd, (const struct sockaddr *)&my_addr, sizeof(my_addr)) < 0) {
        perror("Loi bind");
        close(sockfd);
        exit(EXIT_FAILURE);
    }

    // 4. Cau hinh dia chi dich de gui tin nhan den (ip_d, port_d)
    memset(&dest_addr, 0, sizeof(dest_addr));
    dest_addr.sin_family = AF_INET;
    dest_addr.sin_port = htons(port_d);
    if (inet_pton(AF_INET, ip_d, &dest_addr.sin_addr) <= 0) {
        printf("Dia chi IP dich khong hop le hoac khong ho tro.\n");
        return 1;
    }

    printf("--- UDP Chat non-blocking ---\n");
    printf("[+] Dang lang nghe tren cong: %d\n", port_s);
    printf("[+] Tin nhan go tu ban phim se duoc gui den: %s:%d\n", ip_d, port_d);
    printf("Bat dau chat (Go tin nhan va nhan Enter)...\n\n");

    // 5. Vong lap chinh su dung select() de non-blocking
    while (1) {
        FD_ZERO(&readfds);
        FD_SET(STDIN_FILENO, &readfds); // Theo doi ban phim (Standard Input)
        FD_SET(sockfd, &readfds);       // Theo doi socket (Mang)

        // select() se block o day cho den khi 1 trong 2 (ban phim hoac mang) co du lieu
        // Gia tri max_fd la sockfd (vi STDIN_FILENO = 0)
        if (select(sockfd + 1, &readfds, NULL, NULL, NULL) < 0) {
            perror("Loi select");
            break;
        }

        // a) Kiem tra neu co du lieu tu mang gui den
        if (FD_ISSET(sockfd, &readfds)) {
            int n = recvfrom(sockfd, buffer, BUFFER_SIZE - 1, 0, (struct sockaddr *)&sender_addr, &sender_len);
            if (n > 0) {
                buffer[n] = '\0'; // Them ky tu ket thuc chuoi
                // In ra man hinh tin nhan kem IP/Port cua nguoi gui
                printf("\r[Tu %s:%d]: %s", inet_ntoa(sender_addr.sin_addr), ntohs(sender_addr.sin_port), buffer);
            }
        }

        // b) Kiem tra neu nguoi dung go phim va nhan Enter
        if (FD_ISSET(STDIN_FILENO, &readfds)) {
            if (fgets(buffer, BUFFER_SIZE, stdin) != NULL) {
                // Gui du lieu qua UDP den dest_addr
                sendto(sockfd, buffer, strlen(buffer), 0, (const struct sockaddr *)&dest_addr, sizeof(dest_addr));
            }
        }
    }

    close(sockfd);
    return 0;
}
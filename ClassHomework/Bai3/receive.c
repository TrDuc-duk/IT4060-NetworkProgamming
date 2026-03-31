#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <unistd.h>

#define BUFFER_SIZE 1024

int main(int argc, char *argv[]) {
    // Kiem tra tham so truyen vao
    if (argc != 2) {
        printf("Cach dung: %s <port>\n", argv[0]);
        return 1;
    }

    // 1. Tao socket UDP
    int receiver_sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (receiver_sock == -1) {
        perror("Khong the tao socket");
        return 1;
    }

    // 2. Thiet lap dia chi va cong lang nghe
    struct sockaddr_in receiver_addr;
    receiver_addr.sin_family = AF_INET;
    receiver_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    receiver_addr.sin_port = htons(atoi(argv[1]));

    if (bind(receiver_sock, (struct sockaddr *)&receiver_addr, sizeof(receiver_addr)) == -1) {
        perror("bind() that bai");
        close(receiver_sock);
        return 1;
    }

    printf("[Receiver] Dang lang nghe tren cong %s...\n", argv[1]);

    struct sockaddr_in sender_addr;
    socklen_t sender_addr_len = sizeof(sender_addr);
    char buffer[BUFFER_SIZE];

    // 3. Vong lap nhan va phan hoi (echo)
    while (1) {
        memset(buffer, 0, BUFFER_SIZE); // Xoa sach buffer truoc khi nhan
        
        // Nhan du lieu
        int bytes_received = recvfrom(receiver_sock, buffer, BUFFER_SIZE - 1, 0, 
                                      (struct sockaddr *)&sender_addr, &sender_addr_len);
        
        if (bytes_received > 0) {
            printf("[Sender %s:%d]: %s", 
                   inet_ntoa(sender_addr.sin_addr), 
                   ntohs(sender_addr.sin_port), 
                   buffer);

            // Gui tra lai (echo)
            sendto(receiver_sock, buffer, bytes_received, 0, 
                   (struct sockaddr *)&sender_addr, sender_addr_len);
        }
    }

    close(receiver_sock);
    return 0;
}
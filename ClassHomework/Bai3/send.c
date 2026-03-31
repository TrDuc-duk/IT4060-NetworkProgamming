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
    if (argc != 3) {
        printf("Cach dung: %s <receiver_ip> <receiver_port>\n", argv[0]);
        return 1;
    }

    // 1. Tao socket UDP
    int sender_sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (sender_sock == -1) {
        perror("Khong the tao socket");
        return 1;
    }

    // 2. Thiet lap dia chi dich (Receiver)
    struct sockaddr_in receiver_addr;
    receiver_addr.sin_family = AF_INET;
    receiver_addr.sin_addr.s_addr = inet_addr(argv[1]);
    receiver_addr.sin_port = htons(atoi(argv[2]));

    char buffer[BUFFER_SIZE];
    socklen_t receiver_addr_len = sizeof(receiver_addr);

    printf("[Sender] San sang gui tin nhan toi %s:%s\n", argv[1], argv[2]);
    printf("Go 'exit' de thoat.\n");

    // 3. Vong lap gui tin nhan va nhan echo
    while (1) {
        printf("Ban: ");
        if (fgets(buffer, BUFFER_SIZE, stdin) == NULL) break;

        // Kiem tra lenh thoat
        if (strncmp(buffer, "exit", 4) == 0) break;

        // Gui tin nhan (luu y strlen de gui dung so byte cua chuoi)
        sendto(sender_sock, buffer, strlen(buffer), 0, 
               (struct sockaddr *)&receiver_addr, sizeof(receiver_addr));

        memset(buffer, 0, BUFFER_SIZE); // Xoa buffer de chuan bi nhan

        // Cho nhan phan hoi tu Receiver
        int bytes_received = recvfrom(sender_sock, buffer, BUFFER_SIZE - 1, 0, 
                                      (struct sockaddr *)&receiver_addr, &receiver_addr_len);
                                      
        if (bytes_received > 0) {
            printf("Receive tra ve: %s", buffer);
        }
    }

    close(sender_sock);
    return 0;
}
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#define BUFFER_SIZE 2048

// Luồng nhận tin nhắn từ đối phương
void *receive_handler(void *sock_ptr) {
    int sock = *(int *)sock_ptr;
    char buffer[BUFFER_SIZE];
    while (1) {
        int bytes = recv(sock, buffer, sizeof(buffer) - 1, 0);
        if (bytes <= 0) {
            printf("\nPartner disconnected or server down. Exiting...\n");
            exit(0);
        }
        buffer[bytes] = '\0';
        printf("\rPartner: %s", buffer);
        printf("You: ");
        fflush(stdout);
    }
}

int main() {
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in addr = {AF_INET, htons(8888)};
    inet_pton(AF_INET, "127.0.0.1", &addr.sin_addr);

    if (connect(sock, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        perror("Connection failed");
        return 1;
    }

    printf("Connected to server.\n");

    pthread_t recv_thread;
    pthread_create(&recv_thread, NULL, receive_handler, &sock);

    char msg[BUFFER_SIZE];
    while (1) {
        printf("You: ");
        fgets(msg, sizeof(msg), stdin);
        if (send(sock, msg, strlen(msg), 0) <= 0) break;
    }

    close(sock);
    return 0;
}
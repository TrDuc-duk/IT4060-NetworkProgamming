#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <poll.h>

int main() {
    int client = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    addr.sin_port = htons(8000);

    if (connect(client, (struct sockaddr *)&addr, sizeof(addr))) {
        perror("connect() failed");
        return 1;
    }

    struct pollfd fds[2];
    fds[0].fd = STDIN_FILENO; // Đọc từ bàn phím
    fds[0].events = POLLIN;
    fds[1].fd = client;       // Đọc từ server
    fds[1].events = POLLIN;

    char buf[2048];
    while (1) {
        if (poll(fds, 2, -1) == -1) break;

        if (fds[0].revents & POLLIN) { // Phím nhấn
            fgets(buf, sizeof(buf), stdin);
            send(client, buf, strlen(buf), 0);
        }
        if (fds[1].revents & POLLIN) { // Server gửi tin
            int ret = recv(client, buf, sizeof(buf) - 1, 0);
            if (ret <= 0) break;
            buf[ret] = 0;
            printf("%s", buf);
        }
    }
    close(client);
    return 0;
}

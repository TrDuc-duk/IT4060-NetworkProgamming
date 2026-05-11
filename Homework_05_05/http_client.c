#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

int main() {
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(8080);
    inet_pton(AF_INET, "127.0.0.1", &addr.sin_addr);

    if (connect(sock, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        perror("Connect failed");
        return 1;
    }

    char *req = "GET / HTTP/1.1\r\nHost: localhost\r\n\r\n";
    send(sock, req, strlen(req), 0);

    char buf[1024];
    int ret = recv(sock, buf, sizeof(buf) - 1, 0);
    if (ret > 0) {
        buf[ret] = '\0';
        printf("Response from server:\n%s\n", buf);
    }

    close(sock);
    return 0;
}
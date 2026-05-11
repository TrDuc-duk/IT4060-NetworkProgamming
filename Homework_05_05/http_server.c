#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/wait.h>

#define PORT 8080
#define NUM_CHILDREN 5

void handle_request(int client_sock) {
    char buf[1024];
    int ret = recv(client_sock, buf, sizeof(buf) - 1, 0);
    if (ret > 0) {
        buf[ret] = '\0';
        printf("Child [%d] received request:\n%s\n", getpid(), buf);

        char *msg = "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\r\n<html><body><h1>Xin chao tu Preforked Server!</h1></body></html>";
        send(client_sock, msg, strlen(msg), 0);
    }
    close(client_sock);
}

int main() {
    int server_sock = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(PORT);

    bind(server_sock, (struct sockaddr *)&addr, sizeof(addr));
    listen(server_sock, 10);

    printf("Preforking server starting with %d children on port %d...\n", NUM_CHILDREN, PORT);

    for (int i = 0; i < NUM_CHILDREN; i++) {
        pid_t pid = fork();
        if (pid == 0) { // Tiến trình con
            while (1) {
                struct sockaddr_in client_addr;
                socklen_t client_len = sizeof(client_addr);
                int client_sock = accept(server_sock, (struct sockaddr *)&client_addr, &client_len);
                
                if (client_sock < 0) continue;
                
                printf("Child [%d] accepted connection from %s\n", getpid(), inet_ntoa(client_addr.sin_addr));
                handle_request(client_sock);
            }
        }
    }

    // Tiến trình cha đợi các con (hoặc chạy vô hạn)
    while (wait(NULL) > 0);
    close(server_sock);
    return 0;
}
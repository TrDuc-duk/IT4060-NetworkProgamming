#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <poll.h>
#include <time.h>

#define MAX_FDS 2048
#define MAX_MSG 2048

int main()
{
    int listener = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (listener == -1)
    {
        perror("socket() failed");
        return 1;
    }

    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(8000);

    if (bind(listener, (struct sockaddr *)&addr, sizeof(addr)))
    {
        perror("bind() failed");
        return 1;
    }

    if (listen(listener, 5))
    {
        perror("listen() failed");
        return 1;
    }

    struct pollfd fds[MAX_FDS];
    int nfds = 0;

    fds[0].fd = listener;
    fds[0].events = POLLIN;
    nfds++;

    // Mảng lưu tên client theo số mô tả file (file descriptor)
    char client_names[MAX_FDS][50];
    for (int i = 0; i < MAX_FDS; i++)
        strcpy(client_names[i], "");

    char buf[MAX_MSG];

    printf("Server is running on port 8000...\n");

    while (1)
    {
        int ret = poll(fds, nfds, -1);
        if (ret == -1)
        {
            perror("poll() failed");
            break;
        }

        for (int i = 0; i < nfds; i++)
        {
            if (fds[i].revents & POLLIN)
            {
                if (fds[i].fd == listener)
                {
                    int client = accept(listener, NULL, NULL);
                    if (nfds < MAX_FDS)
                    {
                        fds[nfds].fd = client;
                        fds[nfds].events = POLLIN;
                        nfds++;
                        printf("New connection: %d\n", client);
                        char *msg = "Please login (Syntax: client_id: client_name)\n";
                        send(client, msg, strlen(msg), 0);
                    }
                    else
                    {
                        close(client);
                    }
                }
                else
                {
                    int client = fds[i].fd;
                    ret = recv(client, buf, sizeof(buf) - 1, 0);

                    if (ret <= 0)
                    {
                        printf("Client %d disconnected\n", client);
                        close(client);
                        strcpy(client_names[client], "");
                        fds[i] = fds[nfds - 1];
                        nfds--;
                        i--;
                    }
                    else
                    {
                        buf[ret] = 0;
                        // Loại bỏ ký tự xuống dòng nếu có
                        if (buf[ret - 1] == '\n')
                            buf[ret - 1] = 0;
                        if (buf[ret - 2] == '\r')
                            buf[ret - 2] = 0;

                        if (strcmp(client_names[client], "") == 0)
                        {
                            // Kiểm tra cú pháp đăng nhập
                            char name[50];
                            if (strncmp(buf, "client_id: ", 11) == 0)
                            {
                                strcpy(client_names[client], buf + 11);
                                char ok_msg[100];
                                sprintf(ok_msg, "Welcome %s! You can now chat.\n", client_names[client]);
                                send(client, ok_msg, strlen(ok_msg), 0);
                            }
                            else
                            {
                                char *err = "Invalid syntax. Try again: client_id: client_name\n";
                                send(client, err, strlen(err), 0);
                            }
                        }
                        else
                        {
                            // Đã có tên, thực hiện chuyển tiếp tin nhắn
                            time_t rawtime;
                            struct tm *info;
                            char datetime[80];
                            time(&rawtime);
                            info = localtime(&rawtime);
                            strftime(datetime, 80, "%Y/%m/%d %I:%M:%S%p", info);

                            // Tăng kích thước message để chứa đủ tất cả thành phần
                            char message[MAX_MSG + 500];
                            snprintf(message, sizeof(message), "%s %s: %s\n", datetime, client_names[client], buf);

                            for (int j = 0; j < nfds; j++)
                            {
                                // Gửi cho các client khác (không gửi cho listener và chính nó)
                                if (fds[j].fd != listener && fds[j].fd != client && strcmp(client_names[fds[j].fd], "") != 0)
                                {
                                    send(fds[j].fd, message, strlen(message), 0);
                                }
                            }
                        }
                    }
                }
            }
        }
    }
    close(listener);
    return 0;
}
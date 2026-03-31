#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netdb.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

int main(int argc, char *argv[]) {
    if (argc < 2) {
        printf("Cach dung: %s <Port>\n", argv[0]);
        return 1;
    }

    int listener = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (listener == -1) {
        printf("Failed to create socket.\n");
        exit(1);
    }

    int opt = 1;
    setsockopt(listener, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(atoi(argv[1]));

    if (bind(listener, (struct sockaddr *)&addr, sizeof(addr))) {
        perror("bind() failed");
        return 1;
    }
    
    if (listen(listener, 5)) {
        perror("listen() failed");
        return 1;
    }
    
    printf("Waiting for a new client ...\n");

    struct sockaddr_in clientAddr;
    socklen_t clientAddrLen = sizeof(clientAddr);
    int client = accept(listener, (struct sockaddr *)&clientAddr, &clientAddrLen);
    if (client == -1) {
        printf("accept() failed.\n");
        exit(1);
    }
    
    char msg[8192]; 
    memset(msg, 0, sizeof(msg));
    
    int ret = recv(client, msg, sizeof(msg) - 1, 0);
    if (ret <= 0) {
        printf("Nhan du lieu that bai.\n");
        close(client);
        close(listener);
        return 1;
    }

    // 1. Tách và in ra đường dẫn thư mục trước (được ngăn cách bởi dấu |)
    char *dir_path = strtok(msg, "|");
    if (dir_path != NULL) {
        printf("%s\n", dir_path);
    }

    // 2. Tách phần còn lại (danh sách file) để xử lý
    char *files_str = strtok(NULL, ""); 
    
    if (files_str != NULL) {
        // Tách từng file bằng khoảng trắng
        char *token = strtok(files_str, " ");
        while (token != NULL) {
            // Tách tên file và kích thước bằng dấu hai chấm (:)
            char *colon = strchr(token, ':');
            if (colon != NULL) {
                *colon = '\0'; // Biến dấu : thành ký tự kết thúc chuỗi
                char *filename = token;
                char *filesize = colon + 1;
                
                // In ra đúng định dạng yêu cầu
                printf("%s - %s bytes\n", filename, filesize);
            }
            token = strtok(NULL, " ");
        }
    }

    close(client);
    close(listener);
    return 0;
}
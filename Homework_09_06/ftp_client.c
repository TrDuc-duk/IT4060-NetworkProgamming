#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <netdb.h>
#include <string.h>
#include <arpa/inet.h>

// Hàm đảo ngược chuỗi
void reverse_string(char *str) {
    int len = strlen(str);
    // Loại bỏ các ký tự xuống dòng ở cuối file (nếu có) để đảo ngược chính xác
    while (len > 0 && (str[len - 1] == '\r' || str[len - 1] == '\n')) {
        str[len - 1] = '\0';
        len--;
    }
    
    for (int i = 0; i < len / 2; i++) {
        char temp = str[i];
        str[i] = str[len - 1 - i];
        str[len - 1 - i] = temp;
    }
}

int main()
{
    int client = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

    // 1. Phan giai ten mien server lebavui.io.vn
    struct hostent *he = gethostbyname("lebavui.io.vn");
    if (he == NULL) {
        printf("Khong the phan giai ten mien lebavui.io.vn\n");
        return 1;
    }
    struct in_addr **addr_list = (struct in_addr **) he->h_addr_list;

    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_addr = *addr_list[0]; // Lay IP tu ten mien
    addr.sin_port = htons(21);

    if (connect(client, (struct sockaddr *)&addr, sizeof(addr)))
    {
        perror("connect() failed");
        return 1;
    }

    char buf[4096];
    int len;

    // Nhan xau chao tu Server
    len = recv(client, buf, sizeof(buf) - 1, 0);
    buf[len] = 0;
    puts(buf);

    // 2. Dang nhap tu dong (Hardcode theo yeu cau)
    char username[] = "user_20225180";
    char password[] = "518011";

    // Gui lenh USER
    sprintf(buf, "USER %s\r\n", username);
    send(client, buf, strlen(buf), 0);
    len = recv(client, buf, sizeof(buf) - 1, 0);
    buf[len] = 0;
    puts(buf);

    // Gui lenh PASS
    sprintf(buf, "PASS %s\r\n", password);
    send(client, buf, strlen(buf), 0);
    len = recv(client, buf, sizeof(buf) - 1, 0);
    buf[len] = 0;
    puts(buf);

    if (strncmp(buf, "230", 3) == 0) {
        printf("---> DANG NHAP THANH CONG!\n\n");
    } else {
        printf("---> DANG NHAP THAT BAI!\n");
        return 1;
    }

    // Bien luu ten file
    char question_file[256] = {0};
    char answer_file[256] = {0};

    // =========================================================
    // 3. LAY DANH SACH FILE (NLST) DE TIM "question_xxxxxx.txt"
    // =========================================================
    {
        // Gui lenh EPSV
        send(client, "EPSV\r\n", 6, 0);
        len = recv(client, buf, sizeof(buf) - 1, 0);
        buf[len] = 0;

        // Xac dinh cong (port)
        char *pos1 = strstr(buf, "|||") + 3;
        int port = atoi(pos1);

        // Mo ket noi moi den kenh du lieu
        int data_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        struct sockaddr_in data_addr;
        data_addr.sin_family = AF_INET;
        data_addr.sin_addr = *addr_list[0];
        data_addr.sin_port = htons(port);

        if (connect(data_socket, (struct sockaddr *)&data_addr, sizeof(data_addr))) {
            perror("connect() data socket failed"); return 1;
        }

        // Gui lenh NLST (Chi lay ten file, de parse hon LIST)
        send(client, "NLST\r\n", 6, 0);
        len = recv(client, buf, sizeof(buf) - 1, 0); // Nhan 150 File status okay
        
        // Nhan du lieu danh sach file
        char list_data[4096] = {0};
        while ((len = recv(data_socket, buf, sizeof(buf) - 1, 0)) > 0) {
            buf[len] = 0;
            strcat(list_data, buf);
        }
        close(data_socket);

        // Nhan phan hoi 226 Transfer complete
        len = recv(client, buf, sizeof(buf) - 1, 0); 
        
        // Tim ten file question_ trong list_data
        char *q_start = strstr(list_data, "question_");
        if (q_start != NULL) {
            char *q_end = strstr(q_start, ".txt");
            if (q_end != NULL) {
                int name_len = (q_end + 4) - q_start;
                strncpy(question_file, q_start, name_len);
                question_file[name_len] = '\0';
                
                // Tao ten file answer tuong ung
                sprintf(answer_file, "answer_%s", question_file + 9); // bo chu 'question_' (9 ky tu)
                printf("Tim thay file: %s\n", question_file);
                printf("Se tao file: %s\n\n", answer_file);
            }
        } else {
            printf("Khong tim thay file question nao!\n");
            return 1;
        }
    }

    // =========================================================
    // 4. DOWNLOAD FILE VA DAO NGUOC NOI DUNG
    // =========================================================
    char file_content[2048] = {0};
    {
        // Gui lenh EPSV
        send(client, "EPSV\r\n", 6, 0);
        len = recv(client, buf, sizeof(buf) - 1, 0);
        buf[len] = 0;

        char *pos1 = strstr(buf, "|||") + 3;
        int port = atoi(pos1);

        int data_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        struct sockaddr_in data_addr;
        data_addr.sin_family = AF_INET;
        data_addr.sin_addr = *addr_list[0];
        data_addr.sin_port = htons(port);
        connect(data_socket, (struct sockaddr *)&data_addr, sizeof(data_addr));

        // Gui lenh RETR de tai file
        sprintf(buf, "RETR %s\r\n", question_file);
        send(client, buf, strlen(buf), 0);
        len = recv(client, buf, sizeof(buf) - 1, 0); // Nhận 150

        // Doc noi dung file tu data_socket truc tiep vao RAM
        int total_len = 0;
        while ((len = recv(data_socket, buf, sizeof(buf) - 1, 0)) > 0) {
            buf[len] = 0;
            strcat(file_content, buf);
        }
        close(data_socket);
        
        // Nhan 226 Transfer complete
        len = recv(client, buf, sizeof(buf) - 1, 0); 

        printf("Noi dung goc:\n%s\n", file_content);
        
        // Dao nguoc noi dung
        reverse_string(file_content);
        printf("Noi dung sau khi dao nguoc:\n%s\n\n", file_content);
    }

    // =========================================================
    // 5. UPLOAD FILE ANSWER LEN SERVER (STOR)
    // =========================================================
    {
        // Gui lenh EPSV
        send(client, "EPSV\r\n", 6, 0);
        len = recv(client, buf, sizeof(buf) - 1, 0);
        buf[len] = 0;

        char *pos1 = strstr(buf, "|||") + 3;
        int port = atoi(pos1);

        int data_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        struct sockaddr_in data_addr;
        data_addr.sin_family = AF_INET;
        data_addr.sin_addr = *addr_list[0];
        data_addr.sin_port = htons(port);
        connect(data_socket, (struct sockaddr *)&data_addr, sizeof(data_addr));

        // Gui lenh STOR de upload
        sprintf(buf, "STOR %s\r\n", answer_file);
        send(client, buf, strlen(buf), 0);
        len = recv(client, buf, sizeof(buf) - 1, 0); // Nhận 150

        // Gui noi dung da dao nguoc qua data_socket
        send(data_socket, file_content, strlen(file_content), 0);
        
        close(data_socket);

        // Nhan 226 Transfer complete
        len = recv(client, buf, sizeof(buf) - 1, 0); 
        buf[len] = 0;
        puts(buf);
        printf("---> UPLOAD THANH CONG!\n");
    }

    // Gui lenh QUIT
    send(client, "QUIT\r\n", 6, 0);

    // Ket thuc, dong socket
    close(client);
    return 0;
}
#include <iostream>
#include <cstring>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>

#define BUFFER_SIZE 1024

using namespace std;

int main(int argc, char *argv[])
{
    // Kiểm tra tham số dòng lệnh
    if(argc != 3)
    {
        cerr << "Usage: " << argv[0] << " <IP_address> <port>\n";
        return 1;
    }

    // 1. Tạo socket
    int client_sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (client_sock == -1)
    {
        perror("socket() failed");
        return 1;
    }

    // 2. Khai báo và thiết lập địa chỉ server
    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr)); // Khởi tạo vùng nhớ bằng 0 cho an toàn
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(stoi(argv[2])); // Chuyển port sang network byte order

    // Sử dụng inet_pton an toàn hơn inet_addr
    if (inet_pton(AF_INET, argv[1], &server_addr.sin_addr) <= 0) 
    {
        cerr << "Lỗi: Địa chỉ IP không hợp lệ.\n";
        close(client_sock);
        return 1;
    }
    
    // 3. Kết nối đến server
    if(connect(client_sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1)
    {
        perror("connect() failed");
        close(client_sock);
        return 1;
    }

    cout << "Da ket noi den server! Nhap 'exit' de thoat.\n";

    // Khai báo buffer dùng chung cho cả việc nhận và gửi
    char buffer[BUFFER_SIZE];

    // --- NHẬN LỜI CHÀO TỪ SERVER ---
    int bytes_received = recv(client_sock, buffer, sizeof(buffer) - 1, 0);
    if (bytes_received > 0) {
        buffer[bytes_received] = '\0'; // Đảm bảo kết thúc chuỗi
        cout << "--- Tin nhan tu Server: " << buffer << "\n";
    }

    // 4. Nhập dữ liệu từ bàn phím và gửi đi
    while(true)
    {
        cout << ">> ";
        if (fgets(buffer, sizeof(buffer), stdin) == NULL) {
            break;
        }

        // Thoát nếu người dùng nhập "exit" (fgets lấy cả ký tự '\n')
        if(strncmp(buffer, "exit\n", 5) == 0)
        {
            cout << "Dong ket noi...\n";
            break;
        }

        // Gửi dữ liệu đi
        ssize_t bytes_sent = send(client_sock, buffer, strlen(buffer), 0);
        if (bytes_sent == -1) {
            perror("send() failed");
            break;
        }
    }

    // 5. Đóng socket
    close(client_sock);

    return 0;
}
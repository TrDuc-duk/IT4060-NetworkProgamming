#include <iostream>
#include <fstream>
#include <cstring>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>

#define BUFFER_SIZE 1024

using namespace std;

int main(int argc, char *argv[])
{
    // 1. Kiểm tra tham số dòng lệnh
    if(argc != 4)
    {
        cerr << "Usage: " << argv[0] << " <port> <greeting_file> <client_data_file>\n";
        return 1;
    }

    // 2. Tạo socket
    int server_sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (server_sock == -1)
    {
        perror("socket() failed");
        return 1;
    }

    // Khai báo địa chỉ server
    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY); // Lắng nghe trên mọi IP của máy
    server_addr.sin_port = htons(stoi(argv[1]));     // Cổng lấy từ tham số dòng lệnh

    // 3. Bind socket với địa chỉ và cổng
    if (bind(server_sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1) {
        perror("bind() failed");
        close(server_sock);
        return 1;
    }

    // 4. Lắng nghe kết nối
    if (listen(server_sock, 5) == -1) 
    {
        perror("listen() failed");
        close(server_sock);
        return 1;
    }
    cout << "Dang lang nghe ket noi o cong " << argv[1] << "...\n";

    // 5. Chấp nhận kết nối từ Client
    int client_sock = accept(server_sock, NULL, NULL);
    if (client_sock == -1)
    {
        perror("accept() failed");
        close(server_sock);
        return 1;
    }
    cout << "Co mot client moi vua ket noi (Socket ID: " << client_sock << ")\n";

    // ==========================================
    // YÊU CẦU 1: Đọc file lời chào và gửi cho Client
    // ==========================================
    ifstream greeting_file(argv[2]);
    if (!greeting_file) {
        cerr << "Loi: Khong the mo file loi chao '" << argv[2] << "'\n";
    } else {
        // Đọc toàn bộ nội dung file lời chào vào một string
        string greeting_msg((istreambuf_iterator<char>(greeting_file)), istreambuf_iterator<char>());
        greeting_file.close();
        
        // Gửi qua mạng
        send(client_sock, greeting_msg.c_str(), greeting_msg.length(), 0);
    }

    // ==========================================
    // YÊU CẦU 2: Nhận dữ liệu và ghi vào file
    // ==========================================
    ofstream data_file(argv[3], ios::app); // Mở file để ghi (ios::app là ghi nối tiếp vào cuối file)
    if (!data_file) {
        cerr << "Loi: Khong the tao/mo file du lieu '" << argv[3] << "'\n";
        close(client_sock);
        close(server_sock);
        return 1;
    }

    char buffer[BUFFER_SIZE];
    while(true)
    {
        int bytes_received = recv(client_sock, buffer, sizeof(buffer) - 1, 0);
        
        // Nếu client ngắt kết nối (bytes_received == 0) hoặc có lỗi (< 0)
        if(bytes_received <= 0) 
        {
            cout << "Client da ngat ket noi.\n";
            break;
        }

        // Đảm bảo chuỗi kết thúc đúng cách để in ra màn hình không bị rác
        buffer[bytes_received] = '\0'; 
        
        // Ghi dữ liệu vào file
        data_file << buffer;
        data_file.flush(); // Bắt buộc ghi ngay lập tức xuống ổ cứng
        
        // In ra màn hình server để dễ theo dõi
        cout << "Nhan tu client: " << buffer;
    }

    // 6. Đóng file và ngắt kết nối
    data_file.close();
    close(client_sock);
    close(server_sock);

    return 0;
}
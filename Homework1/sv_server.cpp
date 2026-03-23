#include <iostream>
#include <fstream>
#include <cstring>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <ctime>
#include <iomanip>

using namespace std;

int main(int argc, char* argv[]) {
    // 1. Kiểm tra tham số dòng lệnh (cần 3 tham số: file_chay, cong, ten_file_log)
    if(argc != 3)
    {
        cerr << "Usage: " << argv[0] << " <port> <log_file>\n";
        return 1;
    }

    // 2. Tạo socket và thiết lập địa chỉ
    int server_sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (server_sock == -1)
    {
        perror("socket() failed");
        return 1;
    }

    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    server_addr.sin_port = htons(stoi(argv[1]));

    if (bind(server_sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1) {
        perror("bind() failed");
        close(server_sock);
        return 1;
    }

    if (listen(server_sock, 5) == -1) 
    {
        perror("listen() failed");
        close(server_sock);
        return 1;
    }
    cout << "Server dang lang nghe o cong " << argv[1] << "...\n";

    // Mở file log để ghi nối tiếp
    ofstream log_file(argv[2], ios::app);
    if (!log_file) {
        cerr << "Loi: Khong the mo file log '" << argv[2] << "'\n";
        close(server_sock);
        return 1;
    }

    // 3. Vòng lặp phục vụ các Client
    while (true) {
        // Cấu trúc để lưu thông tin địa chỉ của Client kết nối đến
        struct sockaddr_in client_addr;
        socklen_t client_len = sizeof(client_addr);
        
        int client_sock = accept(server_sock, (struct sockaddr *)&client_addr, &client_len);
        if (client_sock == -1)
        {
            perror("accept() failed");
            continue; // Bỏ qua client bị lỗi, tiếp tục chờ client khác
        }

        // --- BƯỚC A: LẤY IP VÀ THỜI GIAN ---
        // Lấy IP
        char ip_str[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &client_addr.sin_addr, ip_str, INET_ADDRSTRLEN);

        // Lấy thời gian hiện tại
        time_t now = time(0);
        tm *ltm = localtime(&now);
        char time_str[20];
        strftime(time_str, sizeof(time_str), "%Y-%m-%d %H:%M:%S", ltm);

        // --- BƯỚC B: NHẬN DỮ LIỆU ---
        char buf[256];
        int bytes_received = recv(client_sock, buf, sizeof(buf), 0);
        
        if (bytes_received <= 0) {
            close(client_sock);
            continue;
        }

        // --- BƯỚC C: BÓC TÁCH DỮ LIỆU (Unpacking) ---
        int pos = 0;
        int mssv;
        unsigned char ngay, thang;
        unsigned short nam;
        float diemtb;
        char hoten[64];

        memcpy(&mssv, buf + pos, sizeof(mssv)); pos += sizeof(mssv);
        ngay = buf[pos++];
        thang = buf[pos++];
        memcpy(&nam, buf + pos, sizeof(nam)); pos += sizeof(nam);
        memcpy(&diemtb, buf + pos, sizeof(diemtb)); pos += sizeof(diemtb);
        
        int len = bytes_received - pos;
        memcpy(hoten, buf + pos, len);
        // Đảm bảo chuỗi kết thúc đúng phòng trường hợp client gửi thiếu
        hoten[len > 0 && hoten[len-1] == '\0' ? len-1 : len] = '\0'; 

        // --- BƯỚC D: ĐỊNH DẠNG VÀ GHI LOG ---
        char log_line[512];
        // Định dạng ngày sinh theo chuẩn YYYY-MM-DD
        snprintf(log_line, sizeof(log_line), "%s %s %d %s %04d-%02d-%02d %.2f\n",
                 ip_str, time_str, mssv, hoten, nam, thang, ngay, diemtb);

        // In ra màn hình Server
        cout << log_line;
        
        // Ghi vào file log
        log_file << log_line;
        log_file.flush(); // Đảm bảo ghi xuống đĩa cứng ngay lập tức

        close(client_sock); // Xử lý xong thì ngắt kết nối với client này
    }
    
    log_file.close();
    close(server_sock);
    return 0;
}
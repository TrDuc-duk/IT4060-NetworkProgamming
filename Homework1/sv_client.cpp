#include <iostream>
#include <cstring>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>

using namespace std;

int main(int argc, char* argv[]) 
{
    // 1. Kiểm tra tham số dòng lệnh
    if(argc != 3)
    {
        cerr << "Usage: " << argv[0] << " <IP_address> <port>\n";
        return 1;
    }

    // 2. Tạo socket
    int client_sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (client_sock == -1)
    {
        perror("socket() failed");
        return 1;
    }

    // 3. Khai báo địa chỉ server và dùng inet_pton cho an toàn
    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(stoi(argv[2]));

    if (inet_pton(AF_INET, argv[1], &server_addr.sin_addr) <= 0) 
    {
        cerr << "Loi: Dia chi IP khong hop le.\n";
        close(client_sock);
        return 1;
    }

    // 4. KẾT NỐI TRƯỚC KHI NHẬP DỮ LIỆU
    if(connect(client_sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1)
    {
        perror("connect() failed: Server hien khong mo");
        close(client_sock);
        return 1;
    }
    cout << "--- DA KET NOI DEN SERVER ---\n\n";

    // 5. Nhập dữ liệu sinh viên
    int mssv;
    char hoten[64];
    unsigned short nam;
    unsigned char ngay, thang; 
    int in_ngay, in_thang; // Biến tạm để nhập số cho đúng
    float diemtb;

    cout << "Nhap MSSV: ";
    cin >> mssv;
    cin.ignore(); // Dọn dẹp ký tự Enter thừa trong bộ đệm

    cout << "Nhap Ho ten: ";
    cin.getline(hoten, sizeof(hoten));

    cout << "Nhap ngay, thang, nam sinh (Cach nhau boi dau cach. VD: 15 8 2002): ";
    cin >> in_ngay >> in_thang >> nam;
    ngay = (unsigned char)in_ngay;
    thang = (unsigned char)in_thang;

    cout << "Nhap Diem trung binh: ";
    cin >> diemtb;

    // 6. TIẾN HÀNH ĐÓNG GÓI DỮ LIỆU (Data Packing)
    char buffer[256];
    int pos = 0;

    // Đóng gói MSSV (4 bytes)
    memcpy(buffer + pos, &mssv, sizeof(mssv));
    pos += sizeof(mssv);

    // Đóng gói Ngày, Tháng (1 byte mỗi loại, gán thẳng không cần memcpy)
    buffer[pos++] = ngay;
    buffer[pos++] = thang;

    // Đóng gói Năm (2 bytes)
    memcpy(buffer + pos, &nam, sizeof(nam));
    pos += sizeof(nam);

    // Đóng gói Điểm TB (4 bytes)
    memcpy(buffer + pos, &diemtb, sizeof(diemtb));
    pos += sizeof(diemtb);

    // Đóng gói Họ tên (Chuỗi ký tự - QUAN TRỌNG: Phải cộng thêm 1 để lấy ký tự '\0')
    int name_len = strlen(hoten) + 1; 
    memcpy(buffer + pos, hoten, name_len);
    pos += name_len;

    // 7. Gửi toàn bộ gói tin sang server
    ssize_t sent_bytes = send(client_sock, buffer, pos, 0);
    if (sent_bytes == -1) {
        perror("send() failed");
    } else {
        cout << "\n=> Da gui thanh cong " << sent_bytes << " bytes du lieu sang Server!\n";
    }

    close(client_sock);
    return 0;
}
/*
Sinh viên lập trình tạo trang web thực hiện các phép tính đơn giản:
+ Nhận tham số từ lệnh GET
+ Nhận tham số từ lệnh POST
Các tham số bao gồm:
+ x, y: 2 tham số là 2 toán hạng kiểu số thực (hoặc nguyên)
+ cmd: tham số phép tính cần thực hiện, gồm các giá trị add, sub, mul, div.
Server trả lại kết quả là trang web hiển thị phép tính và kết quả.
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <unistd.h>

// Hàm để gửi phản hồi HTTP
void send_response(int client, const char *message) {
    char response[4096];
    snprintf(response, sizeof(response),
        "HTTP/1.1 200 OK\r\n"
        "Content-Type: text/html\r\n"
        "\r\n"
        "%s", message);
    send(client, response, strlen(response), 0);
}

// Hàm để tách và tìm giá trị tham số từ chuỗi query (Đã sửa lỗi an toàn)
void get_param_value(const char* query, const char* param, char* output) {
    output[0] = '\0'; // Khởi tạo chuỗi rỗng mặc định
    
    char search_str[256];
    snprintf(search_str, sizeof(search_str), "%s=", param); // Tìm chính xác "tên_tham_số="
    
    char *p = strstr(query, search_str);
    if (p) {
        p += strlen(search_str); // Bỏ qua phần tên tham số
        int i = 0;
        // Copy giá trị cho đến khi gặp '&', dấu cách, xuống dòng hoặc hết chuỗi
        while (p[i] != '&' && p[i] != ' ' && p[i] != '\r' && p[i] != '\n' && p[i] != '\0') {
            output[i] = p[i];
            i++;
        }
        output[i] = '\0'; // Đóng chuỗi kết quả
    }
}

// Hàm xử lý yêu cầu từ client
void handle_client(int client) {
    char buffer[4096];
    int bytes_received = recv(client, buffer, sizeof(buffer) - 1, 0);
    if (bytes_received <= 0) {
        close(client);
        return;
    }

    buffer[bytes_received] = '\0';
    printf("Received request:\n%s\n", buffer);

    char *query = NULL;

    // Phân tích HTTP Request để lấy phần chứa tham số
    if (strncmp(buffer, "GET", 3) == 0) {
        char *get_query = strstr(buffer, "GET /?");
        if (get_query) {
            query = get_query + strlen("GET /?");
        }
    } 
    else if (strncmp(buffer, "POST", 4) == 0) {
        char *post_query = strstr(buffer, "\r\n\r\n"); // Body của POST nằm sau 2 lần xuống dòng
        if (post_query) {
            query = post_query + 4;
        }
    }

    // Nếu tìm thấy chuỗi tham số hợp lệ
    if (query != NULL && strlen(query) > 0) {
        char x_str[256], y_str[256], cmd[256];
        
        // Trích xuất các tham số
        get_param_value(query, "x", x_str);
        get_param_value(query, "y", y_str);
        get_param_value(query, "cmd", cmd);
      
        printf("Parsed params -> cmd: '%s', x: '%s', y: '%s'\n", cmd, x_str, y_str);

        // Kiểm tra xem đã lấy đủ 3 tham số chưa
        if (strlen(cmd) > 0 && strlen(x_str) > 0 && strlen(y_str) > 0) {
            int x = atoi(x_str);
            int y = atoi(y_str);
            int result;
            char *op_symbol = "";

            if (strcmp(cmd, "add") == 0) {
                result = x + y;
                op_symbol = "+";
            } else if (strcmp(cmd, "sub") == 0) {
                result = x - y;
                op_symbol = "-";
            } else if (strcmp(cmd, "mul") == 0) {
                result = x * y;
                op_symbol = "*";
            } else if (strcmp(cmd, "div") == 0) {
                if (y == 0) {
                    send_response(client, "<html><body><h1>Error: Division by zero</h1></body></html>");
                    close(client);
                    return;
                }
                result = x / y;
                op_symbol = "/";
            } else {
                send_response(client, "<html><body><h1>Error: Invalid operation</h1></body></html>");
                close(client);
                return;
            }

            // Trả lại kết quả phép tính
            char response_message[256];
            snprintf(response_message, sizeof(response_message),
                "<html><body><h1>%d %s %d = %d</h1></body></html>", x, op_symbol, y, result);
            send_response(client, response_message);
        } else {
            send_response(client, "<html><body><h1>Error: Missing parameters</h1></body></html>");
        }
    } else {
        // Mặc định trả về nếu không có tham số
        send_response(client, "<html><body><h1>Vui long gui request GET hoac POST chua tham so x, y, cmd.</h1></body></html>");
    }

    close(client);
}

int main() {
    // Tạo socket
    int listener = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (listener == -1) {
        perror("socket() failed");
        return 1;
    }

    // Cấu hình socket để tái sử dụng port ngay lập tức (tránh lỗi Address already in use)
    int opt = 1;
    setsockopt(listener, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    // Khai báo địa chỉ server
    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(9000);

    // Gắn socket với cấu trúc địa chỉ
    if (bind(listener, (struct sockaddr *)&addr, sizeof(addr))) {
        perror("bind() failed");
        return 1;
    }

    // Chuyển socket sang trạng thái chờ kết nối
    if (listen(listener, 5)) {
        perror("listen() failed");
        return 1;
    }

    printf("Server is listening on port 9000...\n");

    while (1) {
        // Chấp nhận kết nối từ client
        int client = accept(listener, NULL, NULL);
        if (client == -1) {
            perror("accept() failed");
            continue;
        }

        printf("\n--- New client connected: %d ---\n", client);

        if (fork() == 0) {
            // Tiến trình con, xử lý yêu cầu từ client
            close(listener);
            handle_client(client);
            exit(0);
        }

        // Đóng socket client ở tiến trình cha
        close(client);
    }

    return 0;
}
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

#define BUFFER_SIZE 15 
#define PATTERN "0123456789"
#define PATTERN_LEN 10

int count_pattern(const char *data, int data_len, char *cache, int *total_count) {
    // Tạo vùng đệm tạm: cache (tối đa 9 ký tự) + data mới nhận
    char temp[BUFFER_SIZE + PATTERN_LEN]; 
    int cache_len = strlen(cache);
    
    memcpy(temp, cache, cache_len);
    memcpy(temp + cache_len, data, data_len);
    int temp_len = cache_len + data_len;
    temp[temp_len] = '\0'; // Đảm bảo là một xâu hợp lệ để dùng strstr

    char *ptr = temp;
    while ((ptr = strstr(ptr, PATTERN)) != NULL) {
        (*total_count)++;
        ptr += PATTERN_LEN; // Nhảy qua pattern đã tìm thấy
    }

    // Cập nhật cache cho lần nhận kế tiếp: 
    // Giữ lại 9 ký tự cuối cùng của temp (vì pattern dài 10)
    if (temp_len >= PATTERN_LEN - 1) {
        int copy_start = temp_len - (PATTERN_LEN - 1);
        memcpy(cache, temp + copy_start, PATTERN_LEN - 1);
        cache[PATTERN_LEN - 1] = '\0';
    } else {
        strcpy(cache, temp);
    }
    return 0;
}

int main() {
    int count = 0;
    int listener = socket(AF_INET, SOCK_STREAM, 0);
    if (listener == -1) {
        perror("socket() failed");
        return 1;
    }

    // Tùy chọn: Cho phép dùng lại port ngay lập tức sau khi tắt server để dễ test
    int opt = 1;
    setsockopt(listener, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(9000);

    if (bind(listener, (struct sockaddr *)&addr, sizeof(addr)) == -1) {
        perror("bind() failed");
        return 1;
    }

    if (listen(listener, 5) == -1) {
        perror("listen() failed");
        return 1;
    }

    printf("Waiting for connection on port 9000...\n");

    int client = accept(listener, NULL, NULL);
    if (client == -1) {
        perror("accept() failed");
        return 1;
    }
    printf("Client connected.\n");

    char buf[BUFFER_SIZE];
    char cache[PATTERN_LEN]; 
    memset(cache, 0, sizeof(cache)); // Quan trọng: Xóa trắng cache ban đầu

    int ret;
    // Liên tục nhận dữ liệu
    while ((ret = recv(client, buf, sizeof(buf), 0)) > 0) {
        // Xử lý đếm và cập nhật cache
        count_pattern(buf, ret, cache, &count);
        
        // YÊU CẦU ĐỀ BÀI: Cập nhật và in ra màn hình với dữ liệu ĐÃ NHẬN ĐƯỢC
        printf("-> Cap nhat tong so lan xuat hien: %d\n", count);
    }

    printf("Client ngat ket noi. Tong ket so lan xuat hien: %d\n", count);

    close(client);
    close(listener);
    return 0;
}
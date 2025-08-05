#include <stdio.h>
#include <unistd.h>
#include <string.h>   // thêm cho strncmp
#include <stdlib.h>   // thêm cho atoi
#include "api_module/api_module.h"

int main() {
    const char *config_path = "config.txt";

    while (1) {
        printf("=== Bắt đầu gọi API và cập nhật checktime ===\n");
        if (call_api_and_update_config(config_path) != 0) {
            fprintf(stderr, "Lỗi khi gọi API hoặc cập nhật checktime\n");
        }

        // Đọc lại config để lấy delay mới
        FILE *f = fopen(config_path, "r");
        int delay = 18000; // mặc định 5 tiếng
        char line[256];
        if (f) {
            while (fgets(line, sizeof(line), f)) {
                if (strncmp(line, "delay=", 6) == 0) {
                    delay = atoi(line + 6);
                    break;
                }
            }
            fclose(f);
        }

        printf("Chờ %d giây...\n", delay);
        sleep(delay);
    }

    return 0;
}

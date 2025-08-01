#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <curl/curl.h>
#include <time.h>
#include "threat_config.h"

// Callback ghi dữ liệu nhận được từ API vào file
size_t write_data(void *ptr, size_t size, size_t nmemb, void *stream) {
    return fwrite(ptr, size, nmemb, (FILE *)stream);
}

// Hàm tải dữ liệu từ API và lưu vào file
void fetch_threat_list() {
    CURL *curl;
    FILE *fp;
    CURLcode res;

    printf("[%ld] Đang tải dữ liệu từ API: %s\n", time(NULL), API_URL);

    fp = fopen(OUTPUT_FILE, "wb");
    if (!fp) {
        perror("Không thể mở file để ghi");
        return;
    }

    curl_global_init(CURL_GLOBAL_DEFAULT);
    curl = curl_easy_init();
    if (curl) {
        curl_easy_setopt(curl, CURLOPT_URL, API_URL);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_data);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, fp);
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);

        res = curl_easy_perform(curl);

        if (res != CURLE_OK) {
            fprintf(stderr, "Lỗi khi tải API: %s\n", curl_easy_strerror(res));
        } else {
            printf("[%ld] Dữ liệu đã lưu vào file %s\n", time(NULL), OUTPUT_FILE);
        }

        curl_easy_cleanup(curl);
    } else {
        fprintf(stderr, "Không thể khởi tạo CURL\n");
    }

    curl_global_cleanup();
    fclose(fp);
}

int main() {
    while (1) {
        fetch_threat_list();

        printf("Chờ %d phút để tải lại...\n", FETCH_INTERVAL_MINUTES);
        sleep(FETCH_INTERVAL_MINUTES * 60);
    }
    return 0;
}

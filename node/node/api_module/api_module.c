#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <curl/curl.h>
#include <cjson/cJSON.h>


#define MAX_URL_LEN 256
#define BUFFER_SIZE 4096

typedef struct {
    char url[MAX_URL_LEN];
    time_t checktime;
    int delay;
} ConfigData;

//  Hàm chạy lệnh firewall 
static void run_cmd(const char *cmd) {
    printf("[CMD] %s\n", cmd);
    int ret = system(cmd);
    if (ret != 0) {
        fprintf(stderr, "Lỗi khi chạy: %s\n", cmd);
    }
}

//  Xử lý JSON để chặn IP 
static void process_firewall_rules_json(const char *json_str) {
    cJSON *root = cJSON_Parse(json_str);
    if (!root) {
        fprintf(stderr, "Không parse được JSON\n");
        return;
    }

    //  Xử lý ti[] (block vĩnh viễn) 
    cJSON *ti_arr = cJSON_GetObjectItem(root, "ti");
    if (cJSON_IsArray(ti_arr)) {
        cJSON *item;
        cJSON_ArrayForEach(item, ti_arr) {
            cJSON *ip_item = cJSON_GetObjectItem(item, "ip");
            if (cJSON_IsString(ip_item)) {
                char cmd[256];
                snprintf(cmd, sizeof(cmd), "sudo xdpfw-add -m 2 --ip %s", ip_item->valuestring);
                run_cmd(cmd);
            }
        }
    }

    //  Xử lý block_ip[] (block theo thời gian) 
    cJSON *block_arr = cJSON_GetObjectItem(root, "block_ip");
    if (cJSON_IsArray(block_arr)) {
        cJSON *item;
        cJSON_ArrayForEach(item, block_arr) {
            cJSON *ip_item = cJSON_GetObjectItem(item, "ip");
            cJSON *dur_item = cJSON_GetObjectItem(item, "duration");

            if (cJSON_IsString(ip_item) && cJSON_IsNumber(dur_item)) {
                char cmd[256];
                if (dur_item->valueint > 0) {
                    snprintf(cmd, sizeof(cmd),
                             "sudo xdpfw-add -m 2 --ip %s --expires %d",
                             ip_item->valuestring, dur_item->valueint);
                } else {
                    snprintf(cmd, sizeof(cmd),
                             "sudo xdpfw-add -m 2 --ip %s",
                             ip_item->valuestring);
                }
                run_cmd(cmd);
            }
        }
    }

    cJSON_Delete(root);
}

//  Đọc file config 
static int read_config(const char *config_path, ConfigData *cfg) {
    FILE *f = fopen(config_path, "r");
    if (!f) {
        perror("Không mở được file config");
        return -1;
    }
    char line[256];
    while (fgets(line, sizeof(line), f)) {
        if (strncmp(line, "url=", 4) == 0) {
            strncpy(cfg->url, line + 4, sizeof(cfg->url));
            cfg->url[strcspn(cfg->url, "\r\n")] = 0;
        } else if (strncmp(line, "checktime=", 10) == 0) {
            cfg->checktime = atol(line + 10);
        } else if (strncmp(line, "delay=", 6) == 0) {
            cfg->delay = atoi(line + 6);
        }
    }
    fclose(f);
    return 0;
}

//  Ghi file config 
static int write_config(const char *config_path, const ConfigData *cfg) {
    FILE *f = fopen(config_path, "w");
    if (!f) {
        perror("Không mở được file config để ghi");
        return -1;
    }
    fprintf(f, "url=%s\n", cfg->url);
    fprintf(f, "checktime=%ld\n", cfg->checktime);
    fprintf(f, "delay=%d\n", cfg->delay);
    fclose(f);
    return 0;
}

//  Callback CURL 
struct MemoryStruct {
    char *memory;
    size_t size;
};

static size_t write_callback(void *contents, size_t size, size_t nmemb, void *userp) {
    size_t realsize = size * nmemb;
    struct MemoryStruct *mem = (struct MemoryStruct *)userp;

    char *ptr = realloc(mem->memory, mem->size + realsize + 1);
    if (!ptr) {
        fprintf(stderr, "Không đủ bộ nhớ\n");
        return 0;
    }
    mem->memory = ptr;
    memcpy(&(mem->memory[mem->size]), contents, realsize);
    mem->size += realsize;
    mem->memory[mem->size] = 0;
    return realsize;
}

//  Gọi API 
static int call_api(const char *url, time_t checktime, char **response) {
    CURL *curl;
    CURLcode res;
    struct MemoryStruct chunk;
    chunk.memory = malloc(1);
    chunk.size = 0;

    curl = curl_easy_init();
    if (!curl) {
        fprintf(stderr, "Không khởi tạo được CURL\n");
        free(chunk.memory);
        return -1;
    }

    char full_url[MAX_URL_LEN];
    snprintf(full_url, sizeof(full_url), "%s/query?checktime=%ld", url, checktime);

    curl_easy_setopt(curl, CURLOPT_URL, full_url);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&chunk);

    res = curl_easy_perform(curl);
    if (res != CURLE_OK) {
        fprintf(stderr, "CURL lỗi: %s\n", curl_easy_strerror(res));
        free(chunk.memory);
        curl_easy_cleanup(curl);
        return -1;
    }

    *response = chunk.memory;
    curl_easy_cleanup(curl);
    return 0;
}

// Hàm chính
int call_api_and_update_config(const char *config_path) {
    ConfigData cfg;
    if (read_config(config_path, &cfg) != 0) {
        return -1;
    }

    printf("URL: %s\n", cfg.url);
    printf("Checktime hiện tại: %ld\n", cfg.checktime);
    printf("Delay: %d giây\n", cfg.delay);

    char *response = NULL;
    if (call_api(cfg.url, cfg.checktime, &response) != 0) {
        return -1;
    }

    printf("Response:\n%s\n", response);

    // Xử lý JSON & firewall
    process_firewall_rules_json(response);

    // Cập nhật checktime mới
    cfg.checktime = time(NULL);
    if (write_config(config_path, &cfg) != 0) {
        free(response);
        return -1;
    }

    printf("Cập nhật checktime thành: %ld\n", cfg.checktime);

    free(response);
    return 0;
}

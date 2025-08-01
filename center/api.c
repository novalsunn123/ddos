#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#define PORT 8080
#define FILE_PATH "ip_store.txt"
#define THREAT_FILE "threat_list.txt"
#define DEFAULT_TIME 3000
#define MAX_LINE 256

// Đọc toàn bộ file vào buffer
int read_file_content(const char *path, char *buffer, size_t size) {
    FILE *fp = fopen(path, "r");
    if (!fp) return 0;
    size_t len = fread(buffer, 1, size-1, fp);
    buffer[len] = '\0';
    fclose(fp);
    return len;
}

// Xóa file
void clear_file(const char *path) {
    remove(path);
}

// Cập nhật hoặc thêm mới IP vào file
void add_or_update_ip(const char *ip, int time_val) {
    int updated = 0;
    FILE *fp = fopen(FILE_PATH, "r");
    FILE *temp = fopen("temp.txt", "w");

    if (fp) {
        char line[MAX_LINE];
        char existing_ip[64];
        int existing_time;

        while (fgets(line, sizeof(line), fp)) {
            if (sscanf(line, "%63s %d", existing_ip, &existing_time) == 2) {
                if (strcmp(existing_ip, ip) == 0) {
                    fprintf(temp, "%s %d\n", ip, time_val);
                    updated = 1;
                } else {
                    fputs(line, temp);
                }
            }
        }
        fclose(fp);
    }

    if (!updated) {
        fprintf(temp, "%s %d\n", ip, time_val);
    }

    fclose(temp);
    rename("temp.txt", FILE_PATH);
}

void handle_get_store(int client_fd) {
    char buffer[8192] = {0};
    int len = read_file_content(FILE_PATH, buffer, sizeof(buffer));

    char header[256];
    snprintf(header, sizeof(header),
             "HTTP/1.1 200 OK\r\nContent-Type: text/plain\r\nContent-Length: %d\r\n\r\n",
             len);
    write(client_fd, header, strlen(header));
    if (len > 0) write(client_fd, buffer, len);

    clear_file(FILE_PATH); // Xóa sau khi gửi
}

void handle_get_threat(int client_fd) {
    char buffer[8192] = {0};
    int len = read_file_content(THREAT_FILE, buffer, sizeof(buffer));

    if (len == 0) {
        const char *resp = "HTTP/1.1 404 Not Found\r\n\r\nFile not found or empty\n";
        write(client_fd, resp, strlen(resp));
        return;
    }

    char header[256];
    snprintf(header, sizeof(header),
             "HTTP/1.1 200 OK\r\nContent-Type: text/plain\r\nContent-Length: %d\r\n\r\n",
             len);
    write(client_fd, header, strlen(header));
    write(client_fd, buffer, len);
}

void handle_post_store(const char *req, int client_fd) {
    const char *body = strstr(req, "\r\n\r\n");
    if (!body) {
        const char *resp = "HTTP/1.1 400 Bad Request\r\n\r\nNo body\n";
        write(client_fd, resp, strlen(resp));
        return;
    }
    body += 4;

    char ip[64] = {0};
    int time_val = DEFAULT_TIME;

    if (strstr(req, "application/json")) {
        sscanf(body, "{\"ip\":\"%63[^\"]\",\"time\":%d}", ip, &time_val);
        if (strlen(ip) == 0) {
            sscanf(body, "{\"ip\":\"%63[^\"]\"}", ip);
            time_val = DEFAULT_TIME;
        }
    } else {
        strncpy(ip, body, sizeof(ip)-1);
        ip[strcspn(ip, "\r\n")] = 0;
        time_val = DEFAULT_TIME;
    }

    if (strlen(ip) > 0) {
        add_or_update_ip(ip, time_val);
        const char *resp = "HTTP/1.1 200 OK\r\n\r\nPOST OK\n";
        write(client_fd, resp, strlen(resp));
    } else {
        const char *resp = "HTTP/1.1 400 Bad Request\r\n\r\nInvalid IP\n";
        write(client_fd, resp, strlen(resp));
    }
}

int main() {
    int server_fd, client_fd;
    struct sockaddr_in address;
    socklen_t addrlen = sizeof(address);
    char buffer[8192];

    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);
    bind(server_fd, (struct sockaddr *)&address, sizeof(address));

    listen(server_fd, 10);
    printf("API Server running on port %d\n", PORT);

    while (1) {
        client_fd = accept(server_fd, (struct sockaddr *)&address, &addrlen);
        memset(buffer, 0, sizeof(buffer));
        read(client_fd, buffer, sizeof(buffer)-1);

        if (strncmp(buffer, "GET / ", 6) == 0) {
            handle_get_store(client_fd);
        } else if (strncmp(buffer, "GET /threat-list", 16) == 0) {
            handle_get_threat(client_fd);
        } else if (strncmp(buffer, "POST / ", 7) == 0) {
            handle_post_store(buffer, client_fd);
        } else {
            const char *resp = "HTTP/1.1 405 Method Not Allowed\r\n\r\n";
            write(client_fd, resp, strlen(resp));
        }
        close(client_fd);
    }
    return 0;
}

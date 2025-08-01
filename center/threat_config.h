#ifndef THREAT_CONFIG_H
#define THREAT_CONFIG_H

// Link API trả về file txt dạng IP list
#define API_URL "https://lists.blocklist.de/lists/all.txt"

// File lưu danh sách IP
#define OUTPUT_FILE "threat_list.txt"

// Thời gian gọi lại API (phút)
#define FETCH_INTERVAL_MINUTES 5

#endif

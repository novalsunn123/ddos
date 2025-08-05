package main

import (
	"fmt"
	"os"

	"OTX-Go-SDK/src/otxapi"
)

func main() {
	// file lock
	lockPath := `C:\mydata\SourceCode\XDP-Firewall\center\center_DB\tmp\ip_ti.lock`

	// Kiểm tra nếu file lock tồn tại thì bỏ qua
	if _, err := os.Stat(lockPath); err == nil {
		fmt.Println("Job khác đang chạy → bỏ qua")
		return
	}
	// Tạo file lock để đánh dấu job đang chạy
	os.WriteFile(lockPath, []byte("locked"), 0644)
	// Khi job kết thúc thì xóa file lock
	defer os.Remove(lockPath)

	// Đặt API Key
	os.Setenv("X_OTX_API_KEY", "7b2aa486320c39267dcf96cfbf5cb5f5459a2e11fdf496b4cbfc2ca9110988b6")

	client := otxapi.NewClient(nil)

	// Mở file ở chế độ ghi mới, ghi đè dữ liệu cũ
	outputPath := `C:\mydata\SourceCode\XDP-Firewall\center\center_DB\tmp\ip_ti.txt`
	file, err := os.Create(outputPath)
	if err != nil {
		fmt.Println("Không thể tạo file:", err)
		return
	}
	defer file.Close()

	// Lặp qua các trang
	page := 1
	for {
		opt := &otxapi.ListOptions{Page: page, PerPage: 50}
		pulseList, _, err := client.ThreatIntel.List(opt)
		if err != nil {
			fmt.Println("Lỗi khi gọi API:", err)
			break
		}
		if len(pulseList.Pulses) == 0 {
			break // Hết dữ liệu
		}

		// Lọc và in các IP độc hại
		for _, pulse := range pulseList.Pulses {
			for _, ind := range pulse.Indicators {
				if ind.Type != nil && *ind.Type == "IPv4" {
					file.WriteString(*ind.Indicator + "\n")
				}
			}
		}

		page++
	}
	fmt.Println("Đã lưu danh sách IP vào:", outputPath)
}

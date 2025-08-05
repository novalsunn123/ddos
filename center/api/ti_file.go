package api

import (
	"fmt"
	"os"

	"center/OTX-Go-SDK/src/otxapi"
)

func FetchIPThreatIntel() {

	// Đặt API Key
	os.Setenv("X_OTX_API_KEY", "df58d03f5fba56f3b105e349f1b3b019f6031be3b32b41117e007abdc880c40e")

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

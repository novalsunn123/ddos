package main

import (
	"bufio"
	"database/sql"
	"fmt"
	"log"
	"os"
	"path/filepath"
	"time"

	_ "modernc.org/sqlite"

	// Đổi "sqlite_admin/db" thành module path thật của bạn trong go.mod
	"sqlite-admin/db"
)

func main() {
	// Đường dẫn DB và file IP
	dbPath := `C:\mydata\SourceCode\XDP-Firewall\center\center_DB\data.db`
	ipFile := `C:\mydata\SourceCode\XDP-Firewall\center\center_DB\tmp\ip_ti.txt`

	// Mở kết nối DB
	database, err := sql.Open("sqlite", dbPath)
	if err != nil {
		log.Fatalf("Không thể mở DB: %v", err)
	}
	defer database.Close()

	// Đọc file IP và insert/update vào bảng TI
	if err := insertIPsFromFile(database, ipFile); err != nil {
		log.Fatalf("Lỗi khi đọc file IP: %v", err)
	}

	// Thời gian để check query
	checkTime := time.Now().Unix() + 10

	// In dữ liệu
	fmt.Println("=== Dữ liệu bảng TI ===")
	db.QueryTI(database, checkTime)
}

// insert/update dữ liệu từ file IP vào bảng TI
func insertIPsFromFile(database *sql.DB, filePath string) error {
	absPath, err := filepath.Abs(filePath)
	if err != nil {
		return err
	}

	file, err := os.Open(absPath)
	if err != nil {
		return err
	}
	defer file.Close()

	scanner := bufio.NewScanner(file)
	count := 0
	for scanner.Scan() {
		ip := scanner.Text()
		if ip == "" {
			continue
		}
		db.UpsertTI(database, ip)
		count++
	}

	if err := scanner.Err(); err != nil {
		return err
	}

	fmt.Printf("Đã insert/update %d IP vào bảng TI.\n", count)
	return nil
}

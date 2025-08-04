package db

import (
	"database/sql"
	"fmt"
	"log"
	"time"
)

func UpsertBlockIP(db *sql.DB, ip string, duration int) {
	now := time.Now().Unix()
	if duration == 0 {
		duration = 10000 // mặc định
	}
	_, err := db.Exec(`
	INSERT INTO block_ip (ip, duration, updated_at)
	VALUES (?, ?, ?)
	ON CONFLICT(ip) DO UPDATE SET
		duration = excluded.duration,
		updated_at = excluded.updated_at
	`, ip, duration, now)
	if err != nil {
		log.Fatal(err)
	}
}

func QueryBlockIP(db *sql.DB, checkTime int64) {
	rows, err := db.Query(`SELECT ip, duration, updated_at FROM block_ip WHERE updated_at < ?`, checkTime)
	if err != nil {
		log.Fatal(err)
	}
	defer rows.Close()

	for rows.Next() {
		var ip string
		var duration int
		var updatedAt int64
		rows.Scan(&ip, &duration, &updatedAt)
		fmt.Printf("IP: %-15s  Duration: %-6d  UpdatedAt: %s\n",
			ip, duration, time.Unix(updatedAt, 0).Format("2006-01-02 15:04:05"))
	}
}

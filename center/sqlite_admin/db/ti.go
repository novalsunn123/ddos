package db

import (
	"database/sql"
	"fmt"
	"log"
	"time"
)

func UpsertTI(db *sql.DB, ip string) {
	now := time.Now().Unix()
	_, err := db.Exec(`
	INSERT INTO TI (ip, updated_at)
	VALUES (?, ?)
	ON CONFLICT(ip) DO UPDATE SET updated_at = excluded.updated_at
	`, ip, now)
	if err != nil {
		log.Fatal(err)
	}
}

func QueryTI(db *sql.DB, checkTime int64) {
	rows, err := db.Query(`SELECT ip, updated_at FROM TI WHERE updated_at < ?`, checkTime)
	if err != nil {
		log.Fatal(err)
	}
	defer rows.Close()

	for rows.Next() {
		var ip string
		var updatedAt int64
		rows.Scan(&ip, &updatedAt)
		fmt.Printf("IP: %-15s  UpdatedAt: %s\n", ip, time.Unix(updatedAt, 0).Format("2006-01-02 15:04:05"))
	}
}

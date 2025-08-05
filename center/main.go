package main

import (
	"fmt"
	"sync"
	"time"

	"center/api"
)

func main() {
	var mu sync.Mutex // Dùng để tránh chạy song song

	// Chạy API song song
	go func() {
		api.Api()
	}()

	// Vòng lặp vô hạn
	for {
		mu.Lock() // Chặn không cho job chạy song song

		fmt.Println("=== Bắt đầu FetchIPThreatIntel ===")
		api.FetchIPThreatIntel()

		fmt.Println("=== Bắt đầu InsertDB ===")
		api.InsertDB()

		mu.Unlock()

		fmt.Println("=== Hoàn thành job, đợi 5 tiếng tiếp theo ===")
		time.Sleep(5 * time.Hour)
	}
}

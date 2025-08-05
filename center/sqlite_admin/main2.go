package main

import (
	"database/sql"
	"encoding/json"
	"fmt"
	"log"
	"net/http"
	"strconv"

	"sqlite-admin/db"

	_ "modernc.org/sqlite"
)

var database *sql.DB

// Struct nhận dữ liệu khi POST /blockip
type BlockIPRequest struct {
	IP       string `json:"ip"`
	Duration int    `json:"duration"` // giây
}

// Struct trả về khi GET /query
type QueryResponse struct {
	TI      []map[string]interface{} `json:"ti"`
	BlockIP []map[string]interface{} `json:"block_ip"`
}

func main() {
	var err error
	database, err = sql.Open("sqlite", `C:\mydata\SourceCode\XDP-Firewall\center\center_DB\data.db`)
	if err != nil {
		log.Fatalf("Không mở được DB: %v", err)
	}
	defer database.Close()

	http.HandleFunc("/blockip", handleBlockIP)
	http.HandleFunc("/query", handleQuery)

	fmt.Println("🚀 API Server đang chạy tại :8080")
	log.Fatal(http.ListenAndServe(":8080", nil))
}

// POST /blockip
func handleBlockIP(w http.ResponseWriter, r *http.Request) {
	if r.Method != http.MethodPost {
		http.Error(w, "Chỉ hỗ trợ POST", http.StatusMethodNotAllowed)
		return
	}

	var req BlockIPRequest
	if err := json.NewDecoder(r.Body).Decode(&req); err != nil {
		http.Error(w, "Dữ liệu JSON không hợp lệ", http.StatusBadRequest)
		return
	}

	if req.IP == "" {
		http.Error(w, "Thiếu IP", http.StatusBadRequest)
		return
	}

	db.UpsertBlockIP(database, req.IP, req.Duration)

	w.Header().Set("Content-Type", "application/json")
	json.NewEncoder(w).Encode(map[string]string{
		"status": "ok",
	})
}

// GET /query?checktime=...
func handleQuery(w http.ResponseWriter, r *http.Request) {
	checkTimeStr := r.URL.Query().Get("checktime")
	if checkTimeStr == "" {
		http.Error(w, "Thiếu tham số checktime", http.StatusBadRequest)
		return
	}

	checkTime, err := strconv.ParseInt(checkTimeStr, 10, 64)
	if err != nil {
		http.Error(w, "checktime không hợp lệ", http.StatusBadRequest)
		return
	}

	// Query TI
	tiRows, err := database.Query(`SELECT ip, updated_at FROM TI WHERE updated_at > ?`, checkTime)
	if err != nil {
		http.Error(w, "Lỗi query TI", http.StatusInternalServerError)
		return
	}
	var tiResults []map[string]interface{}
	for tiRows.Next() {
		var ip string
		var updatedAt int64
		tiRows.Scan(&ip, &updatedAt)
		tiResults = append(tiResults, map[string]interface{}{
			"ip":         ip,
			"updated_at": updatedAt,
		})
	}
	tiRows.Close()

	// Query block_ip
	blockRows, err := database.Query(`SELECT ip, duration, updated_at FROM block_ip WHERE updated_at > ?`, checkTime)
	if err != nil {
		http.Error(w, "Lỗi query block_ip", http.StatusInternalServerError)
		return
	}
	var blockResults []map[string]interface{}
	for blockRows.Next() {
		var ip string
		var duration int
		var updatedAt int64
		blockRows.Scan(&ip, &duration, &updatedAt)
		blockResults = append(blockResults, map[string]interface{}{
			"ip":         ip,
			"duration":   duration,
			"updated_at": updatedAt,
		})
	}
	blockRows.Close()

	resp := QueryResponse{
		TI:      tiResults,
		BlockIP: blockResults,
	}

	w.Header().Set("Content-Type", "application/json")
	json.NewEncoder(w).Encode(resp)
}

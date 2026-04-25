package main

import (
	"bufio"
	"encoding/json"
	"fmt"
	"io"
	"net/http"
	"os"
	"os/exec"
	"path/filepath"
	"runtime"
	"strings"

	"github.com/google/uuid"
)

var authToken string

func main() {
	authToken = uuid.New().String()
	fmt.Println("=== YTMP3 Companion ===")
	fmt.Printf("Token: %s\n", authToken)
	fmt.Println("Listening on http://localhost:8765")
	fmt.Println("Press Ctrl+C to stop")

	http.HandleFunc("/", cors(handleStatus))
	http.HandleFunc("/download", cors(handleDownload))
	http.HandleFunc("/progress", cors(handleProgress))

	if err := http.ListenAndServe(":8765", nil); err != nil {
		fmt.Fprintf(os.Stderr, "Server error: %v\n", err)
		os.Exit(1)
	}
}

func cors(next http.HandlerFunc) http.HandlerFunc {
	return func(w http.ResponseWriter, r *http.Request) {
		w.Header().Set("Access-Control-Allow-Origin", "*")
		w.Header().Set("Access-Control-Allow-Headers", "Content-Type, Authorization")
		if r.Method == http.MethodOptions {
			w.WriteHeader(http.StatusNoContent)
			return
		}
		next(w, r)
	}
}

func handleStatus(w http.ResponseWriter, r *http.Request) {
	json.NewEncoder(w).Encode(map[string]any{
		"status": "ok",
		"token":  authToken,
	})
}

type downloadRequest struct {
	URL    string `json:"url"`
	Format string `json:"format"`
}

func handleDownload(w http.ResponseWriter, r *http.Request) {
	if r.Method != http.MethodPost {
		http.Error(w, "Method not allowed", http.StatusMethodNotAllowed)
		return
	}

	var req downloadRequest
	if err := json.NewDecoder(r.Body).Decode(&req); err != nil {
		http.Error(w, "Invalid JSON", http.StatusBadRequest)
		return
	}

	if req.URL == "" {
		http.Error(w, "URL required", http.StatusBadRequest)
		return
	}

	format := req.Format
	if format == "" {
		format = "mp3"
	}

	// Start download in background
	go runDownload(req.URL, format)

	w.WriteHeader(http.StatusAccepted)
	json.NewEncoder(w).Encode(map[string]string{
		"message": "Download started",
	})
}

var progressClients = make(map[string]chan string)

func handleProgress(w http.ResponseWriter, r *http.Request) {
	w.Header().Set("Content-Type", "text/event-stream")
	w.Header().Set("Cache-Control", "no-cache")
	w.Header().Set("Connection", "keep-alive")

	flusher, ok := w.(http.Flusher)
	if !ok {
		http.Error(w, "Streaming not supported", http.StatusInternalServerError)
		return
	}

	clientID := uuid.New().String()
	ch := make(chan string, 10)
	progressClients[clientID] = ch
	defer delete(progressClients, clientID)

	fmt.Fprintf(w, "event: connected\ndata: %s\n\n", clientID)
	flusher.Flush()

	for msg := range ch {
		fmt.Fprintf(w, "data: %s\n\n", msg)
		flusher.Flush()
	}
}

func broadcastProgress(msg string) {
	for _, ch := range progressClients {
		select {
		case ch <- msg:
		default:
		}
	}
}

func runDownload(url, format string) {
	downloadsDir, err := getDownloadsDir()
	if err != nil {
		broadcastProgress(`{"type":"error","message":"` + err.Error() + `"}`)
		return
	}

	outputTemplate := filepath.Join(downloadsDir, "%(title)s.%(ext)s")

	cmd := exec.Command("yt-dlp",
		"-x",
		"--audio-format", format,
		"--audio-quality", "0",
		"--no-playlist",
		"--newline",
		"--progress",
		"-o", outputTemplate,
		url,
	)

	stderr, err := cmd.StderrPipe()
	if err != nil {
		broadcastProgress(`{"type":"error","message":"` + err.Error() + `"}`)
		return
	}

	stdout, err := cmd.StdoutPipe()
	if err != nil {
		broadcastProgress(`{"type":"error","message":"` + err.Error() + `"}`)
		return
	}

	if err := cmd.Start(); err != nil {
		broadcastProgress(`{"type":"error","message":"` + err.Error() + `"}`)
		return
	}

	go readOutput(stdout)
	go readOutput(stderr)

	if err := cmd.Wait(); err != nil {
		broadcastProgress(`{"type":"error","message":"Download failed: ` + err.Error() + `"}`)
		return
	}

	broadcastProgress(`{"type":"complete","message":"Download complete"}`)
}

func readOutput(r io.Reader) {
	scanner := bufio.NewScanner(r)
	for scanner.Scan() {
		line := scanner.Text()
		if strings.Contains(line, "[download]") {
			// Parse progress line
			broadcastProgress(`{"type":"progress","message":"` + strings.TrimSpace(line) + `"}`)
		}
	}
}

func getDownloadsDir() (string, error) {
	home, err := os.UserHomeDir()
	if err != nil {
		return "", err
	}

	switch runtime.GOOS {
	case "windows":
		return filepath.Join(home, "Downloads"), nil
	case "darwin":
		return filepath.Join(home, "Downloads"), nil
	default:
		return filepath.Join(home, "Downloads"), nil
	}
}

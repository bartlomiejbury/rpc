package main

import (
	"fmt"
	"log"
	"os"
	"os/signal"
	"syscall"
	"time"
)

func main() {
	server := NewRPCServer("test")

	server.Register("add", func(params []interface{}) (interface{}, error) {
		a := int(params[0].(float64))
		b := int(params[1].(float64))
		return a + b, nil
	})

	server.Register("multiply", func(params []interface{}) (interface{}, error) {
		a := int(params[0].(float64))
		b := int(params[1].(float64))
		return a * b, nil
	})

	if err := server.Start(); err != nil {
		log.Fatal(err)
	}

	fmt.Println("Server started on ipc:///tmp/rpc.sock and ipc:///tmp/pub.sock")

	// Broadcast co 2 sekundy w goroutine
	done := make(chan bool)
	go func() {
		ticker := time.NewTicker(2 * time.Second)
		defer ticker.Stop()
		i := 0
		for {
			select {
			case <-ticker.C:
				i++
				server.Publish("time", map[string]interface{}{
					"value": time.Now().Format(time.RFC3339),
				})
				fmt.Printf("Published time update %d\n", i)
			case <-done:
				return
			}
		}
	}()

	// Czekaj na Ctrl+C
	sigChan := make(chan os.Signal, 1)
	signal.Notify(sigChan, os.Interrupt, syscall.SIGTERM)
	<-sigChan

	fmt.Println("\nStopping server...")
	close(done)
	server.Stop()
	fmt.Println("Server stopped.")
}
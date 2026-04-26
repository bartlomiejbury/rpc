package main

import (
	"fmt"
	"log"
	"os"
	"os/signal"
	"syscall"
	"time"
)

func onTime(topic, payload string) {
	fmt.Printf("[%s] %s\n", topic, payload)
}

func onWeather(topic, payload string) {
	fmt.Printf("[%s] %s\n", topic, payload)
}

func main() {
	client, err := NewRPCPubSubClient("test")
	if err != nil {
		log.Fatal(err)
	}
	defer client.Stop()

	// Subscribe to topics
	if err := client.Subscribe("time", onTime); err != nil {
		log.Fatal(err)
	}
	if err := client.Subscribe("weather", onWeather); err != nil {
		log.Fatal(err)
	}

	// Make RPC calls
	result, err := client.Call(3000, "add", 5, 3)
	if err != nil {
		log.Printf("RPC error: %v", err)
	} else {
		fmt.Printf("add(5,3) = %v\n", result)
	}

	result, err = client.Call(3000, "multiply", 4, 2)
	if err != nil {
		log.Printf("RPC error: %v", err)
	} else {
		fmt.Printf("multiply(4,2) = %v\n", result)
	}

	// Unsubscribe from 'weather' after 10 seconds
	go func() {
		time.Sleep(10 * time.Second)
		fmt.Println("Unsubscribing from 'weather'")
		if err := client.Unsubscribe("weather"); err != nil {
			log.Printf("Unsubscribe error: %v", err)
		}
	}()

	// Wait for Ctrl+C
	sigChan := make(chan os.Signal, 1)
	signal.Notify(sigChan, os.Interrupt, syscall.SIGTERM)
	<-sigChan

	fmt.Println("\nStopping client...")
}

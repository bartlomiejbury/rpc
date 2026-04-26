package main

import (
	"encoding/json"
	"fmt"
	"sync"
	"time"

	"github.com/pebbe/zmq4"
)

type RPCPubSubClient struct {
	rpcAddr string
	subAddr string

	req *zmq4.Socket
	sub *zmq4.Socket

	callbacks map[string]func(string, string)
	mu        sync.Mutex

	running  bool
	stopChan chan struct{}
	wg       sync.WaitGroup
}

func NewRPCPubSubClient(name string) (*RPCPubSubClient, error) {
	client := &RPCPubSubClient{
		rpcAddr:   "ipc:///tmp/" + name + "_rpc.sock",
		subAddr:   "ipc:///tmp/" + name + "_pub.sock",
		callbacks: make(map[string]func(string, string)),
		running:   true,
		stopChan:  make(chan struct{}),
	}

	var err error

	client.req, err = zmq4.NewSocket(zmq4.REQ)
	if err != nil {
		return nil, fmt.Errorf("failed to create REQ socket: %w", err)
	}
	if err = client.req.Connect(client.rpcAddr); err != nil {
		client.req.Close()
		return nil, fmt.Errorf("failed to connect REQ to %s: %w", client.rpcAddr, err)
	}

	client.sub, err = zmq4.NewSocket(zmq4.SUB)
	if err != nil {
		client.req.Close()
		return nil, fmt.Errorf("failed to create SUB socket: %w", err)
	}

	if err = client.sub.Connect(client.subAddr); err != nil {
		client.req.Close()
		client.sub.Close()
		return nil, fmt.Errorf("failed to connect SUB to %s: %w", client.subAddr, err)
	}

	client.wg.Add(1)
	go client.listenSub()

	return client, nil
}

func (c *RPCPubSubClient) Call(timeoutMs int, method string, params ...interface{}) (interface{}, error) {
	// Set timeout
	c.req.SetRcvtimeo(time.Duration(timeoutMs) * time.Millisecond)

	request := map[string]interface{}{
		"method": method,
		"params": params,
	}

	reqData, err := json.Marshal(request)
	if err != nil {
		return nil, fmt.Errorf("failed to marshal request: %w", err)
	}

	if _, err := c.req.SendBytes(reqData, 0); err != nil {
		return nil, fmt.Errorf("failed to send request: %w", err)
	}

	respData, err := c.req.RecvBytes(0)
	if err != nil {
		return nil, fmt.Errorf("RPC call '%s' timed out: %w", method, err)
	}

	var response map[string]interface{}
	if err := json.Unmarshal(respData, &response); err != nil {
		return nil, fmt.Errorf("failed to unmarshal response: %w", err)
	}

	if response["error"] != nil {
        if errMsg, ok := response["error"].(string); ok {
            if errMsg == "" {
                return nil, fmt.Errorf("RPC error: empty error message")
            }
            return nil, fmt.Errorf("RPC error: %s", errMsg)
        }
        return nil, fmt.Errorf("RPC error: invalid error type")
	}

	return response["result"], nil
}

func (c *RPCPubSubClient) Subscribe(topic string, callback func(string, string)) error {
	c.mu.Lock()
	defer c.mu.Unlock()

	if err := c.sub.SetSubscribe(topic); err != nil {
		return fmt.Errorf("failed to subscribe to %s: %w", topic, err)
	}

	c.callbacks[topic] = callback
	return nil
}

func (c *RPCPubSubClient) Unsubscribe(topic string) error {
	c.mu.Lock()
	defer c.mu.Unlock()

	if err := c.sub.SetUnsubscribe(topic); err != nil {
		return fmt.Errorf("failed to unsubscribe from %s: %w", topic, err)
	}

	delete(c.callbacks, topic)
	return nil
}

func (c *RPCPubSubClient) listenSub() {
	defer c.wg.Done()

	for c.running {
		select {
		case <-c.stopChan:
			return
		default:
			msg, err := c.sub.RecvMessage(zmq4.DONTWAIT)
			if err != nil {
				time.Sleep(10 * time.Millisecond)
				continue
			}

			if len(msg) < 2 {
				continue
			}

			topic := msg[0]
			payload := msg[1]

			c.mu.Lock()
			callback, exists := c.callbacks[topic]
			c.mu.Unlock()

			if exists && callback != nil {
				callback(topic, payload)
			}
		}
	}
}

func (c *RPCPubSubClient) Stop() {
	c.running = false
	close(c.stopChan)
	c.wg.Wait()

	if c.req != nil {
		c.req.Close()
	}
	if c.sub != nil {
		c.sub.Close()
	}
}

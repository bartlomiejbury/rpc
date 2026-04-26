package main

import (
	"encoding/json"
	"sync"
	"time"

	"github.com/pebbe/zmq4"
)

type RPCServer struct {
	RPCAddr string
	PubAddr string
	funcs   map[string]func([]interface{}) (interface{}, error)

	rep *zmq4.Socket
	pub *zmq4.Socket

	stopChan chan struct{}
	wg       sync.WaitGroup
}

func NewRPCServer(name string) *RPCServer {
	return &RPCServer{
		RPCAddr:   "ipc:///tmp/" + name + "_rpc.sock",
		PubAddr:   "ipc:///tmp/" + name + "_pub.sock",
		funcs:    make(map[string]func([]interface{}) (interface{}, error)),
		stopChan: make(chan struct{}),
	}
}

func (s *RPCServer) Register(name string, f func([]interface{}) (interface{}, error)) {
	s.funcs[name] = f
}

func (s *RPCServer) Start() error {
	var err error

	s.rep, err = zmq4.NewSocket(zmq4.REP)
	if err != nil {
		return err
	}
	if err = s.rep.Bind(s.RPCAddr); err != nil {
		return err
	}

	s.pub, err = zmq4.NewSocket(zmq4.PUB)
	if err != nil {
		return err
	}
	if err = s.pub.Bind(s.PubAddr); err != nil {
		return err
	}

	s.wg.Add(1)
	go s.rpcLoop()

	return nil
}

func (s *RPCServer) rpcLoop() {
	defer s.wg.Done()
	for {
		select {
		case <-s.stopChan:
			return
		default:
			msg, err := s.rep.Recv(zmq4.DONTWAIT)
			if err != nil {
				time.Sleep(10 * time.Millisecond)
				continue
			}

			var req map[string]interface{}
			if err := json.Unmarshal([]byte(msg), &req); err != nil {
				s.rep.Send(`{"result":null,"error":"invalid json"}`, 0)
				continue
			}

			method, ok := req["method"].(string)
			if !ok {
				s.rep.Send(`{"result":null,"error":"missing method"}`, 0)
				continue
			}

			params, _ := req["params"].([]interface{})
			fn, exists := s.funcs[method]
			if !exists {
				s.rep.Send(`{"result":null,"error":"unknown method"}`, 0)
				continue
			}

			result, err := fn(params)
			resp := map[string]interface{}{"result": result}
			if err != nil {
				resp["error"] = err.Error()
			} else {
				resp["error"] = nil
			}
			data, _ := json.Marshal(resp)
			s.rep.Send(string(data), 0)
		}
	}
}

func (s *RPCServer) Publish(topic string, msg interface{}) {
	data, _ := json.Marshal(msg)
	s.pub.SendMessage(topic, string(data))
}

func (s *RPCServer) Stop() {
	close(s.stopChan)
	s.wg.Wait()
	if s.rep != nil {
		s.rep.Close()
	}
	if s.pub != nil {
		s.pub.Close()
	}
}
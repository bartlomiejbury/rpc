import zmq
import json
import time
import threading
from typing import Callable, Dict, List, Any, Optional


class RPCServer:
    def __init__(self, name: str):
        self.rpc_addr = "ipc:///tmp/" + name + "_rpc.sock"
        self.pub_addr = "ipc:///tmp/" + name + "_pub.sock"
        self.funcs: Dict[str, Callable] = {}

        self.context = zmq.Context()
        self.rep: Optional[zmq.Socket] = None
        self.pub: Optional[zmq.Socket] = None

        self.stop_flag = threading.Event()
        self.rpc_thread: Optional[threading.Thread] = None

    def register(self, name: str, func: Callable[[List[Any]], Any]):
        """Register an RPC function"""
        self.funcs[name] = func

    def start(self):
        """Start the RPC server"""

        self.rep = self.context.socket(zmq.REP)
        self.rep.bind(self.rpc_addr)

        self.pub = self.context.socket(zmq.PUB)
        self.pub.bind(self.pub_addr)

        self.rpc_thread = threading.Thread(target=self._rpc_loop, daemon=True)
        self.rpc_thread.start()

        print(f"RPC Server started on {self.rpc_addr}")
        print(f"PUB Server started on {self.pub_addr}")

    def _rpc_loop(self):
        """RPC loop running in background thread"""
        while not self.stop_flag.is_set():
            try:
                if self.rep.poll(10, zmq.POLLIN):
                    msg = self.rep.recv_string()

                    try:
                        req = json.loads(msg)
                    except json.JSONDecodeError:
                        self.rep.send_json({"result": None, "errorCode": 2, "errorMsg": "invalid json"})
                        continue

                    method = req.get("method")
                    if not method:
                        self.rep.send_json({"result": None, "errorCode": 1, "errorMsg": "missing method"})
                        continue

                    params = req.get("params", [])

                    if method not in self.funcs:
                        self.rep.send_json({"result": None, "errorCode": 3, "errorMsg": "unknown method"})
                        continue

                    try:
                        result = self.funcs[method](params)
                        self.rep.send_json({"result": result, "errorCode": 0, "errorMsg": None})
                    except Exception as e:
                        self.rep.send_json({"result": None, "errorCode": 1, "errorMsg": str(e)})

            except zmq.ZMQError as e:
                if not self.stop_flag.is_set():
                    print(f"ZMQ Error in RPC loop: {e}")
                break

    def publish(self, topic: str, msg: Any):
        """Publish a message to subscribers"""
        if self.pub:
            data = json.dumps(msg)
            self.pub.send_multipart([topic.encode(), data.encode()])

    def stop(self):
        """Stop the server and close sockets"""
        print("Stopping server...")
        self.stop_flag.set()

        if self.rpc_thread:
            self.rpc_thread.join(timeout=1.0)

        if self.rep:
            self.rep.close()
        if self.pub:
            self.pub.close()

        self.context.term()
        print("Server stopped.")


if __name__ == "__main__":

    # Register the add function
    def add(params: List[Any]) -> int:
        a = int(params[0])
        b = int(params[1])
        return a + b

    # Register the multiply function
    def multiply(params: List[Any]) -> int:
        a = int(params[0])
        b = int(params[1])
        return a * b

    server = RPCServer("test")
    server.register("add", add)
    server.register("multiply", multiply)
    server.start()

    try:
        i = 0
        while True:
            time.sleep(2)
            i += 1
            server.publish("time", {
                "value": time.strftime("%Y-%m-%dT%H:%M:%S%z")
            })
            print(f"Published time update {i}")
    except KeyboardInterrupt:
        print("\nInterrupted by user")
    finally:
        server.stop()

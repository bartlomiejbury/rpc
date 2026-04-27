import zmq
import threading
import json
import time

class RPCPubSubClient:
    def __init__(self, name):
        self.context = zmq.Context()

        rpc_addr = "ipc:///tmp/" + name + "_rpc.sock"
        sub_addr = "ipc:///tmp/" + name + "_pub.sock"

        self.req = self.context.socket(zmq.REQ)
        self.req.connect(rpc_addr)

        self.sub = self.context.socket(zmq.SUB)
        self.sub.connect(sub_addr)

        self._callbacks = {}
        self._lock = threading.Lock()

        self._running = True

        self._sub_thread = threading.Thread(target=self._listen_sub, daemon=True)
        self._sub_thread.start()

    def call(self, method, *params, timeout_ms=3000):
        """Wywołanie RPC synchroniczne z timeoutem w ms"""
        self.req.setsockopt(zmq.RCVTIMEO, timeout_ms)

        req = {"method": method, "params": params}
        self.req.send_json(req)
        try:
            resp = self.req.recv_json()
        except zmq.Again:
            raise TimeoutError(f"RPC call '{method}' timed out after {timeout_ms} ms")

        # Check errorCode (0 = success)
        error_code = resp.get("errorCode", 0)
        if error_code != 0:
            error_msg = resp.get("errorMsg", "unknown error")
            raise Exception(f"RPC error (code {error_code}): {error_msg}")

        return resp.get("result")

    def __getattr__(self, name):
        def method(*args, **kwargs):
            # kwargs mogą zawierać timeout_ms
            return self.call(name, *args, **kwargs)
        return method

    def subscribe(self, topic, callback):
        self.sub.setsockopt_string(zmq.SUBSCRIBE, topic)
        with self._lock:
            self._callbacks[topic] = callback

    def unsubscribe(self, topic):
        self.sub.setsockopt_string(zmq.UNSUBSCRIBE, topic)
        with self._lock:
            if topic in self._callbacks:
                del self._callbacks[topic]

    def stop(self):
        """Zatrzymanie wątku SUB i zamknięcie socketów"""
        self._running = False
        self._sub_thread.join(timeout=1)
        self.sub.close()
        self.req.close()
        self.context.term()

    def _listen_sub(self):
        while self._running:
            try:
                parts = self.sub.recv_multipart(flags=zmq.NOBLOCK)
            except zmq.Again:
                time.sleep(0.01)
                continue

            topic = parts[0].decode()
            payload = parts[1].decode() if len(parts) > 1 else ""
            try:
                payload = json.loads(payload) if payload else {}
            except json.JSONDecodeError:
                payload = {"raw": payload}

            with self._lock:
                cb = self._callbacks.get(topic)
            if cb:
                cb(topic, payload)


if __name__ == "__main__":
    def on_time(topic, data):
        print("[TIME]", data)

    def on_weather(topic, data):
        print("[WEATHER]", data)

    client = RPCPubSubClient("test")
    client.subscribe("time", on_time)
    client.subscribe("weather", on_weather)

    result = client.add(5,3)
    print(f"add(5,3) = {result}")
    result = client.multiply(4,2)
    print(f"multiply(4,2) = {result}")

    time.sleep(10)
    print("Unsubscribing from 'weather'")
    client.unsubscribe("weather")

    try:
        while True:
            time.sleep(1)
    except KeyboardInterrupt:
        print("Stopping client...")
        client.stop()

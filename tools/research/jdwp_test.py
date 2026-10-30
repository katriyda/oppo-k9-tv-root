import socket
s = socket.create_connection(("127.0.0.1", 8700), 3)
s.sendall(b"JDWP-Handshake")
s.settimeout(3)
try:
    print("reply:", repr(s.recv(32)))
except Exception as e:
    print("err:", e)
s.close()

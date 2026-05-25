import socket
import time

s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
s.connect(("127.0.0.1", 8080))

request = "GET / HTTP/1.1\r\nHost: 127.0.0.1\r\n\r\n"

for c in request:
	s.send(c.encode())
	time.sleep(0.2)

print(s.recv(4096).decode(errors="ignore"))
s.close()

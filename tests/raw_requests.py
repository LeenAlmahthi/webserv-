import socket

tests = [
	"GET / HTTP/1.1\r\nHost: localhost\r\n\r\n",
	"DELETE /no-file HTTP/1.1\r\nHost: localhost\r\n\r\n",
	"BADMETHOD / HTTP/1.1\r\nHost: localhost\r\n\r\n",
	"GET /does-not-exist HTTP/1.1\r\nHost: localhost\r\n\r\n",
]

for req in tests:
	s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
	s.connect(("127.0.0.1", 8080))
	s.send(req.encode())
	print(s.recv(4096).decode(errors="ignore").split("\r\n")[0])
	s.close()

*This project has been created as part of the 42 curriculum by lalmahth, naalmasr, & skteifan.*

# webserv

## Description

Webserv is a lightweight HTTP/1.1 web server written in C++98 as part of the 42 curriculum.

The goal of this project is to understand how a real web server works by implementing core networking and web concepts from scratch using low-level system calls and non-blocking I/O.

The server supports:
- Multiple server configurations and ports
- HTTP methods: GET, POST, DELETE
- Static file serving
- Directory listing (autoindex)
- File uploads
- CGI execution (Python, PHP, Shell, etc.)
- HTTP redirections
- Custom error pages
- Chunked transfer decoding
- Non-blocking client handling using `poll()`

The project was developed following the constraints and requirements of the 42 `webserv` subject.

---

## Instructions

### Compilation

```bash
make
```

### Run the server

```bash
./webserv webserv.conf
```

### Example request

```bash
curl -i http://localhost:8080/
```

### CGI Example

```bash
curl -i http://localhost:8080/cgi/test.py
```

### Upload Example

```bash
curl -i -X POST -F "file=@test.txt" http://localhost:8080/upload
```

---

## Features

- Non-blocking server using `poll()`
- Multiple simultaneous clients
- Configurable routes and ports
- CGI execution through configured interpreters
- Chunked request body support
- Upload handling with multipart/form-data
- Custom error pages
- Directory listing
- Request size limiting
- Basic timeout handling for CGI

---

## Resources

### Documentation & References

- RFC 7230 — Hypertext Transfer Protocol (HTTP/1.1)
- RFC 3875 — CGI/1.1 Specification
- Linux man pages:
  - `poll`
  - `socket`
  - `recv`
  - `send`
  - `fork`
  - `execve`
  - `pipe`
- Beej’s Guide to Network Programming:
  - https://beej.us/guide/bgnet/
- MDN HTTP Documentation:
  - https://developer.mozilla.org/en-US/docs/Web/HTTP

### AI Usage

AI tools were used during the development of this project for:
- Debugging assistance
- Understanding HTTP and CGI behavior
- Reviewing architecture decisions
- Suggesting edge-case tests
- Explaining low-level networking concepts
- Improving code organization and readability

All implementation decisions, integration, debugging, and final validation were done manually by the project authors.

---

## Notes

This project was developed under the constraints of:
- C++98 standard
- Non-blocking I/O
- Allowed functions specified by the subject

The server was tested using:
- `curl`
- Web browsers
- `valgrind`
- Manual CGI and upload tests

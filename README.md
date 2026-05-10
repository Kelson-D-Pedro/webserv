_This project has been created as part of the 42 curriculum by kpedro, mebo, jmiguel._

# 🌐 Webserv

An HTTP server implemented in **C++98**, inspired by the **NGINX** architecture.  
This project is part of the **42 Luanda** curriculum, and the objective is to understand how the HTTP protocol works, from **non-blocking sockets** to **CGI** and **file uploads**.

---

## 📖 Table of Contents
- [Description](#-description)
- [Features](#-features)
- [Instructions](#-instructions)
- [Compilation](#-compilation)
- [Execution](#-execution)
- [Configuration](#-configuration)
- [Testing](#-testing)
- [Project Rules](#-project-rules)
- [Bonus](#-bonus)
- [Authors](#-authors)
- [Resources](#-resources)

---

## 📝 Description
**Webserv** is an HTTP server written in **C++98**, capable of serving static pages, processing forms, executing scripts via **CGI**, and managing multiple simultaneous connections in a non-blocking manner.

This project helps you understand deeply:
- How the **HTTP/1.1** protocol works.
- **TCP/IP sockets** and low-level programming.
- Managing multiple clients with **poll()** (or equivalent).
- Configuration structure inspired by **NGINX**.

---

## ⚙️ Features
✔️ Support for **multiple ports** and simultaneous connections.  
✔️ **GET**, **POST**, and **DELETE** HTTP methods.  
✔️ Serve static files (HTML, CSS, images).  
✔️ **File uploads** via POST.  
✔️ **File deletion** with DELETE.  
✔️ **Custom error pages** (404, 403, 500, etc).  
✔️ **Autoindex** (directory listing).  
✔️ Script execution via **CGI** (Python, Bash).  
✔️ Configuration inspired by **nginx.conf**:
   - `listen`, `server`, `location`, `root`, `index`  
   - Request size limit  
   - HTTP redirects  
   - Upload directories  

---
## Instructions 
## 🛠️ Compilation
The project must be compiled with a **Makefile** containing the rules:
```bash
make        # compiles the project
make clean  # removes object files
make fclean # removes object files and final binary
make re     # recompiles from scratch
```

Mandatory compilation flags:
```
-Wall -Wextra -Werror -std=c++98
```

---

## ▶️ Execution
Run the server with:
```bash
./webserv [config_file]
```

Example:
```bash
./webserv config/default.conf
```

If no configuration file is provided, the program can use a **default config**.

---

## 📂 Configuration
The configuration file is inspired by **NGINX**, supporting blocks like:

```nginx
server {
    error_page 404 ./www/errors/404.html;
    listen 8080;
    root ./www;
    index index.html;

    location /uploads {
        allow_methods POST DELETE;
        upload_path ./www/uploads;
    }
}
```

**Supported directives:**
- `listen` - IP address and port to listen on
- `server_name` - Server name (virtual host)
- `root` - Root directory for served files
- `index` - Default index file
- `error_page` - Custom error pages
- `client_max_body_size` - Maximum request body size
- `location` - URL-based routing rules
- `allow_methods` - Allowed HTTP methods per location
- `autoindex` - Enable/disable directory listing
- `upload_path` - Directory for file uploads
- `return` - HTTP redirects
- `cgi_ext` - CGI file extension
- `cgi_path` - Path to CGI interpreter

---

## 🧪 Testing
Recommended tools:
- `curl` → test basic requests
- `telnet` → debug HTTP responses
- `ab`, `wrk`, `hey`, `siege` → stress testing
- Web browser → validate real behavior

Examples:
```bash
curl http://localhost:8080/
curl -X POST -F "file=@test.txt" http://localhost:8080/uploads
curl -X DELETE http://localhost:8080/uploads/test.txt
```

---

## 📏 Project Rules
- Implemented in **C++98**.  
- Only **1 call to epoll()** to manage all I/O.  
- Never block I/O (non-blocking).  
- Only use `fork()` for CGI.  
- No external libraries (Boost, etc).  
- The program **must not crash** under any circumstances.  
- One `epoll()` to monitor all I/O between clients and server.

---

## 🎁 Bonus
- Cookies and session management.  
- Support for multiple CGI types (Python and Bash).  
- Advanced HTTP redirects.  
- **HTTP/1.1 Keep-Alive** support.  

---

## 👥 Authors
- Kelson Pedro (kpedro)
- Melzira Ebo (mebo)
- Joisson Miguel (jmiguel)

42 Luanda – 2026

---

## 📚 Resources

### HTTP Protocol
- [MDN Web Docs - HTTP](https://developer.mozilla.org/en-US/docs/Web/HTTP)
- [RFC 7230 - HTTP/1.1 Message Syntax](https://tools.ietf.org/html/rfc7230)
- [RFC 7231 - HTTP/1.1 Semantics](https://tools.ietf.org/html/rfc7231)

### Networking & Sockets
- [Beej's Guide to Network Programming](https://beej.us/guide/bgnet/)
- [POSIX Socket Programming](https://man7.org/linux/man-pages/man7/socket.7.html)
- [epoll Documentation](https://man7.org/linux/man-pages/man7/epoll.7.html)

### CGI
- [CGI 1.1 Specification](https://tools.ietf.org/html/rfc3875)
- [CGI Environment Variables](https://www.tutorialspoint.com/cgi/cgi_environment.htm)

### NGINX
- [NGINX Documentation](https://nginx.org/en/docs/)
- [NGINX Configuration Guide](https://nginx.org/en/docs/http/ngx_http_core_module.html)

### Tools & Testing
- [curl Documentation](https://curl.se/docs/)
- [Apache Bench (ab)](https://httpd.apache.org/docs/2.4/programs/ab.html)
- [wrk - HTTP Benchmarking Tool](https://github.com/wg/wrk)

### AI Usage
This project was developed with assistance for:
- Code structure and architecture design (see docs/dirStruct.md)
- Debugging non-blocking I/O operations (see file src/core/Multiplexer.cpp)
- HTTP header parsing and validation (with the function parseRequest() located at src/http/Request.cpp)
- CGI environment setup and communication (execute() located at src/cgi/CgiHandler.cpp)
- Configuration file parsing logic (with the function tokenizer() located at src/config/Tokenizer.cpp, and verify_parse_data() located at src/config/VerifingParseData.cpp)
- Error handling and edge cases (status errors responses with functions that handle methods, located at src/http/Method.cpp, and the function Autoindex::generate
 located at src/http/Autoindex.cpp)
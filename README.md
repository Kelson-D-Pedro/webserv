# 🌐 Webserv — C++ HTTP Server (42 Luanda)

_Group project by kpedro, mebo, jmiguel._

## Why this project is relevant for C++ backend roles
This repository demonstrates hands-on backend systems engineering in C++:
- Non-blocking socket programming and I/O multiplexing (`epoll`)
- HTTP/1.1 request parsing and response generation
- Multi-client server behavior with keep-alive handling
- CGI process execution and inter-process communication basics
- Robustness work: protocol correctness, memory leak fixes, status-code handling, and file descriptor flags (`fcntl`)

---

## My contribution summary (Kelson Pedro / `kpedro`)
- **52 commits** between **2025-09-16** and **2026-01-20**
- Core work delivered in:
  - Initial project structure and repository organization
  - `Socket` and multiplexing flow for multiple connections
  - HTTP methods: `GET`, `POST`, `DELETE`, plus upload paths
  - `Request` / `Response` implementation and protocol fixes
  - Stability improvements (keep-alive, parsing corrections, leak fixes, CGI status handling)
  - Early CGI integration

---

## Technical highlights
- HTTP methods: **GET / POST / DELETE**
- Static file serving and directory autoindex
- File upload and file deletion endpoints
- Config-driven routing inspired by NGINX blocks
- Custom error pages (`404`, `403`, `405`, `500`, etc.)
- CGI execution (Python/Bash)
- Multi-port and multi-server configuration support

---

## Build
```bash
make
```

Build flags:
```bash
-Wall -Wextra -Werror -std=c++98
```

---

## Run
```bash
./webserv [config_file]
```

Example:
```bash
./webserv config/default.conf
```

---

## Automated validation
Project includes an end-to-end script:
```bash
./run_tests.sh
```

It validates key flows such as:
- static routes and missing file behavior
- redirects and headers
- CGI GET/POST behavior
- upload limits and `413` handling
- `DELETE` behavior for uploaded files
- autoindex behavior
- method restriction and `Allow` headers

---

## Fit analysis for the Tradeweb C++ Developer opportunity
### Strong match
- **C++ backend engineering mindset:** built and maintained a server in C++ with clear modular classes.
- **Networking fundamentals:** low-level sockets, multiplexed I/O, and protocol-oriented debugging.
- **Problem-solving and ownership:** direct work on stability and protocol correctness issues.
- **Team workflow:** real multi-contributor project with Git history and sustained delivery.
- **Linux development:** practical implementation and testing on Linux.

### Partial match / growth areas
- **Modern C++:** project is C++98 (42 requirement), while role mentions modern C++ patterns/templates.
- **Multithreading:** architecture is event-driven/non-blocking, not thread-based concurrency.
- **Distributed/high-throughput optimization depth:** solid foundation, but limited explicit production-scale benchmarking evidence in the repo.
- **Windows exposure:** current implementation and tooling are Linux-focused.

### Positioning recommendation
Use this project in LinkedIn/CV as **proof of core systems capability**:
1. C++ network programming fundamentals  
2. Protocol-level debugging and reliability work  
3. Ability to build backend services from zero  

Then complement it with 1–2 focused side projects in:
- modern C++ (C++17/20, templates, RAII-heavy design),
- multithreading / lock-free or queue-based design,
- performance benchmarking and profiling.

---

## Authors
- Kelson Pedro (`kpedro`)
- Melzira Ebo (`mebo`)
- Joisson Miguel (`jmiguel`)

42 Luanda – 2026

# Proton Web Server

**High-Performance Event-Driven Web Server, Reverse Proxy, Load Balancer & API Gateway**
![DALL·E 2026-01-08 15 29 27 - High‑quality modern vector logo for an open‑source infrastructure project named “Proton Web Server” with CLEAR, LEGIBLE text  Combine a minimalist abs](https://github.com/user-attachments/assets/f82660d8-4b8c-4119-838a-cd6704c547cb)

A production-grade HTTP server built in C, inspired by NGINX architecture principles but designed from scratch with modern use cases in mind.

[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](LICENSE)
[![Build Status](https://img.shields.io/badge/build-passing-brightgreen.svg)]()

---

## 1. Project Overview

**Name:** Proton Web Server  
**Tagline:** Lightning-fast, event-driven HTTP server and reverse proxy built in pure C

### Goals
- **Performance-first architecture** using event-driven I/O (epoll/kqueue)
- **Master/worker process model** for stability and graceful reloads
- **Minimal memory footprint** with explicit ownership and zero-copy where possible
- **Modular design** allowing custom protocol handlers and filters
- **Production-ready features**: reverse proxy, load balancing, caching, TLS

---

## 2. Architecture Design

### Core Principles
Proton follows the NGINX philosophy but implements it independently:

1. **Master/Worker Process Model**
   - Master process: config parsing, worker lifecycle, graceful reload
   - Worker processes: handle connections via event loop (epoll)
   - Zero downtime reloads via graceful worker replacement

2. **Event-Driven, Non-Blocking I/O**
   - epoll-based event loop on Linux
   - kqueue support for BSD/macOS (future)
   - Single-threaded workers (one per CPU core)
   - Async socket operations (no blocking calls)

3. **Request Lifecycle**
   ```
   Client → Accept → Read → Parse → Route → Handle → Write → Close
              ↓        ↓       ↓       ↓       ↓       ↓       ↓
            epoll   buffer   HTTP   config  module  buffer  epoll
   ```

4. **Memory Management**
   - Custom memory pool allocator (per-request lifecycle)
   - Zero-copy buffer chains for proxying
   - Explicit ownership (no GC overhead)
   - Pre-allocated worker shared memory zones

### Architecture Diagram

```
┌─────────────────────────────────────────────────────────────┐
│                      MASTER PROCESS                         │
│  • Parse config    • Manage workers    • Handle signals     │
└────────┬────────────────────────────────────────────────────┘
         │
         ├──────┬──────────┬──────────┬──────────┐
         ▼      ▼          ▼          ▼          ▼
    ┌────────┐ ┌────────┐ ┌────────┐ ┌────────┐ ...
    │ WORKER │ │ WORKER │ │ WORKER │ │ WORKER │
    │   #1   │ │   #2   │ │   #3   │ │   #4   │
    └────┬───┘ └────┬───┘ └────┬───┘ └────┬───┘
         │          │          │          │
         ▼          ▼          ▼          ▼
    ┌─────────────────────────────────────────┐
    │         epoll EVENT LOOP (each)         │
    │  • Accept connections                   │
    │  • Read/Write sockets                   │
    │  • Parse HTTP                           │
    │  • Execute modules                      │
    │  • Proxy upstream                       │
    └─────────────────────────────────────────┘
```

### Key Differences from NGINX
- **Simplified module API** (registration via function pointers)
- **Built-in API gateway features** (JWT validation, rate limiting)
- **Modern config syntax** (optional JSON/YAML in addition to directive-based)
- **Integrated observability** (Prometheus metrics endpoint)

---

## 3. Repository Structure

```
proton-web/
├── src/
│   ├── core/               # Core server logic
│   │   ├── proton.c        # Main entry point
│   │   ├── master.c        # Master process management
│   │   ├── worker.c        # Worker process event loop
│   │   ├── pool.c          # Memory pool allocator
│   │   ├── buffer.c        # Buffer chain implementation
│   │   └── log.c           # Logging system
│   ├── event/              # Event subsystem
│   │   ├── event.c         # Generic event interface
│   │   ├── epoll.c         # Linux epoll implementation
│   │   └── kqueue.c        # BSD kqueue (future)
│   ├── http/               # HTTP protocol
│   │   ├── http_parser.c   # HTTP/1.1 request parser
│   │   ├── http_handler.c  # Request routing & handling
│   │   ├── http_response.c # Response builder
│   │   └── http_upstream.c # Reverse proxy logic
│   ├── modules/            # Pluggable modules
│   │   ├── mod_static.c    # Static file serving
│   │   ├── mod_proxy.c     # Reverse proxy
│   │   ├── mod_cache.c     # Content caching
│   │   ├── mod_loadbalancer.c
│   │   └── mod_jwt.c       # JWT validation
│   └── config/             # Configuration parser
│       ├── config_parser.c # Directive-based parser
│       └── config_json.c   # JSON config (optional)
├── include/                # Public headers
│   ├── proton.h
│   ├── http.h
│   ├── event.h
│   └── module.h
├── conf/                   # Example configs
│   ├── proton.conf.example
│   └── proton.json.example
├── docs/                   # Documentation
│   ├── architecture.md
│   ├── module-api.md
│   └── configuration.md
├── tests/                  # Test suite
│   ├── unit/               # Unit tests (C)
│   ├── integration/        # Integration tests
│   └── load/               # Load testing scripts
├── tools/                  # Python tooling
│   ├── protonctl           # CLI control utility (Python)
│   ├── config-validate.py  # Config validation
│   └── metrics-exporter.py # Prometheus exporter
├── build/                  # Build artifacts (gitignored)
├── Makefile                # Build system
├── CMakeLists.txt          # Alternative CMake build
├── LICENSE                 # MIT License
└── README.md               # This file
```

### Directory Purposes
- **`src/core/`**: Process management, memory pools, logging
- **`src/event/`**: Platform-specific event loop implementations
- **`src/http/`**: HTTP protocol parsing and handling
- **`src/modules/`**: Feature modules (static files, proxy, cache, etc.)
- **`src/config/`**: Configuration file parsers
- **`tools/`**: Python-based admin/monitoring utilities (NO request handling)

---

## 4. Core Features (MVP v0.1)

### Must-Have Features
✅ **TCP Server**  
- Multi-worker process pool  
- epoll-based accept/read/write  

✅ **HTTP/1.1 Protocol**  
- Request parsing (method, headers, body)  
- Response building (status, headers, chunked encoding)  
- Keep-alive connections  

✅ **Reverse Proxy**  
- Upstream connection pooling  
- Header rewriting  
- Basic load balancing (round-robin)  

✅ **Configuration System**  
- NGINX-style directive syntax  
- Hot reload without downtime  

✅ **Static File Serving**  
- sendfile() optimization  
- MIME type detection  

✅ **Logging**  
- Access logs  
- Error logs with levels (debug, info, warn, error)  

✅ **Graceful Reload**  
- SIGHUP triggers master to spawn new workers  
- Old workers finish requests before exit  

### Planned Features (Post-MVP)
-  **TLS/SSL** (OpenSSL integration)
-  **Compression** (gzip, brotli)
-  **HTTP/2** support
-  **Content Cache** (in-memory + disk)
-  **Load Balancing** (least-conn, IP hash, consistent hashing)
-  **API Gateway** features (JWT auth, rate limiting, request transformation)
-  **Metrics** (Prometheus endpoint)
- **WebSocket** proxy support

---

## 5. Configuration System

### Syntax
NGINX-style directive/block syntax:

```nginx
# proton.conf

worker_processes auto;
error_log /var/log/proton/error.log warn;

events {
    worker_connections 1024;
    use epoll;
}

http {
    access_log /var/log/proton/access.log;
    
    upstream backend {
        server 127.0.0.1:3000;
        server 127.0.0.1:3001;
        server 127.0.0.1:3002;
    }

    server {
        listen 8080;
        server_name example.com;

        location / {
            root /var/www/html;
            index index.html;
        }

        location /api {
            proxy_pass http://backend;
            proxy_set_header Host $host;
            proxy_set_header X-Real-IP $remote_addr;
        }

        location /cached {
            proxy_pass http://backend;
            proxy_cache my_cache;
            proxy_cache_valid 200 5m;
        }
    }
}
```

### Parsing in C
1. **Lexer**: Tokenize config file (directives, blocks, values)
2. **Parser**: Build AST of directives → contexts (http, server, location)
3. **Config Structure**:
   ```c
   typedef struct {
       int worker_processes;
       int worker_connections;
       char *error_log;
       proton_http_config_t *http;
   } proton_config_t;

   typedef struct {
       proton_array_t *servers;  // Array of server blocks
       proton_array_t *upstreams; // Array of upstream blocks
   } proton_http_config_t;

   typedef struct {
       int port;
       char *server_name;
       proton_array_t *locations; // Array of location blocks
   } proton_server_config_t;
   ```
4. **Validation**: Check required directives, valid values
5. **Reload**: Master process reparses on SIGHUP, spawns new workers with new config

---

## 6. Build System

### Makefile (Recommended for MVP)
```makefile
CC = gcc
CFLAGS = -Wall -Wextra -O2 -std=c11 -Iinclude
LDFLAGS = -lpthread

# Debug build
debug: CFLAGS += -g -DDEBUG -O0
debug: all

# Release build
release: CFLAGS += -O3 -DNDEBUG
release: all

SRCS = $(wildcard src/**/*.c src/*.c)
OBJS = $(SRCS:.c=.o)

all: proton

proton: $(OBJS)
	$(CC) $(OBJS) -o build/proton $(LDFLAGS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) build/proton

install:
	install -m 755 build/proton /usr/local/bin/
	install -m 644 conf/proton.conf.example /etc/proton/proton.conf
```

### CMake (Alternative)
```cmake
cmake_minimum_required(VERSION 3.15)
project(proton C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_FLAGS_DEBUG "-g -DDEBUG")
set(CMAKE_C_FLAGS_RELEASE "-O3 -DNDEBUG")

file(GLOB_RECURSE SOURCES "src/*.c")
add_executable(proton ${SOURCES})
target_include_directories(proton PRIVATE include)
target_link_libraries(proton pthread)
```

### Module Compilation
- **Static Modules**: Compiled into main binary (default)
- **Dynamic Modules** (future): `.so` files loaded at runtime via `dlopen()`

---

## 7. Running with Docker

Proton Web Server is fully containerized and can be run with Docker following NGINX best practices.

### Quick Start with Docker

```bash
# Using Docker Compose (recommended for development)
docker-compose up -d

# Or using docker run
docker build -t proton-web-server .
docker run -d -p 8080:8080 --name proton proton-web-server

# Test the server
curl http://localhost:8080/
```

### Docker Features

- **Multi-stage Alpine build** (~10-15MB final image)
- **Non-root user** for security (UID 101)
- **Auto-tuning** worker processes based on CPU cores
- **Health checks** for container orchestration
- **Volume mounts** for live development
- **Production-ready** with NGINX load balancer setup

### Docker Compose Configurations

**Development** (`docker-compose.yml`):
```bash
docker-compose up -d              # Start development server
docker-compose logs -f            # View logs
docker-compose down               # Stop server
```

**Production** (`docker-compose.prod.yml`):
```bash
docker-compose -f docker-compose.prod.yml up -d     # Start with load balancer
docker-compose -f docker-compose.prod.yml ps        # Check status
docker-compose -f docker-compose.prod.yml down      # Stop all services
```

### Environment Variables

Configure the server using environment variables:

| Variable | Description | Default |
|----------|-------------|---------|
| `PROTON_WORKERS` | Number of worker processes | Auto (CPU cores) |
| `PROTON_LOG_LEVEL` | Log level | `info` |
| `TZ` | Timezone | `UTC` |

### Documentation

For comprehensive Docker documentation, see [DOCKER.md](DOCKER.md):
- Building images
- Configuration options
- Production deployment
- Multi-architecture support
- Troubleshooting

---

## 8. Module System

### Module Lifecycle
```c
// include/module.h
typedef struct {
    const char *name;
    int (*init)(proton_config_t *config);
    int (*handler)(proton_request_t *req, proton_response_t *res);
    void (*cleanup)(void);
} proton_module_t;

// Registration (static array in src/core/proton.c)
extern proton_module_t mod_static;
extern proton_module_t mod_proxy;
extern proton_module_t mod_cache;

static proton_module_t *modules[] = {
    &mod_static,
    &mod_proxy,
    &mod_cache,
    NULL
};
```

### Example: Simple HTTP Module
```c
// src/modules/mod_hello.c
#include "proton.h"
#include "http.h"

static int mod_hello_init(proton_config_t *config) {
    proton_log(LOG_INFO, "Hello module initialized");
    return 0;
}

static int mod_hello_handler(proton_request_t *req, proton_response_t *res) {
    if (strcmp(req->uri, "/hello") == 0) {
        res->status = 200;
        proton_response_set_header(res, "Content-Type", "text/plain");
        proton_response_write(res, "Hello, Proton!\n", 15);
        return PROTON_MODULE_HANDLED;
    }
    return PROTON_MODULE_DECLINED;
}

proton_module_t mod_hello = {
    .name = "hello",
    .init = mod_hello_init,
    .handler = mod_hello_handler,
    .cleanup = NULL
};
```

### Module Execution
1. Master process calls `init()` for all modules during startup
2. Worker receives request → calls `handler()` for each module in order
3. First module returning `PROTON_MODULE_HANDLED` stops the chain
4. Modules returning `PROTON_MODULE_DECLINED` pass to next module

---

## 9. Python Tooling (Optional)

Python is **ONLY** used for operational tooling, **NEVER** in the request path.

### Tools
**`protonctl`** - CLI for server management
```bash
protonctl start          # Start server
protonctl stop           # Graceful stop
protonctl reload         # Hot reload config
protonctl status         # Check worker status
protonctl logs --tail 50 # View logs
```

**`config-validate.py`** - Validate config before reload
```bash
python tools/config-validate.py /etc/proton/proton.conf
```

**`metrics-exporter.py`** - Export metrics to Prometheus
```bash
python tools/metrics-exporter.py --port 9090
```

### Interaction with C Core
- **Signals**: Python sends SIGHUP/SIGTERM to master PID
- **Unix Sockets**: Python reads stats from `/var/run/proton/stats.sock`
- **Log Files**: Python tails/parses access/error logs
- **Shared Memory**: Python reads metrics from shared memory zone (mmap)

---

## 10. Testing Strategy

### Unit Tests (C)
- Framework: **Unity** or **Check**
- Test individual functions (parsers, buffers, pools)
```c
// tests/unit/test_http_parser.c
void test_parse_request_line(void) {
    char *raw = "GET /index.html HTTP/1.1\r\n";
    proton_request_t req = {0};
    assert(parse_request_line(raw, &req) == 0);
    assert(strcmp(req.method, "GET") == 0);
    assert(strcmp(req.uri, "/index.html") == 0);
}
```

### Integration Tests
- Python scripts using `requests` library
- Test full HTTP flows (static files, proxy, caching)
```python
# tests/integration/test_proxy.py
def test_proxy_basic():
    resp = requests.get('http://localhost:8080/api/users')
    assert resp.status_code == 200
    assert 'X-Proton-Proxy' in resp.headers
```

### Load Testing
- **wrk**: `wrk -t4 -c100 -d30s http://localhost:8080/`
- **Apache Bench**: `ab -n 10000 -c 100 http://localhost:8080/`
- Target: **50k+ req/s on 4-core machine**

### CI/CD
```yaml
# .github/workflows/ci.yml
name: CI
on: [push, pull_request]
jobs:
  build:
    runs-on: ubuntu-latest
    steps:
      - uses: actions/checkout@v3
      - run: make
      - run: make test
      - run: ./tests/integration/run_tests.sh
```

---

## 11. Getting Started

### Prerequisites
- **OS**: Linux (Ubuntu 20.04+, CentOS 8+)
- **Compiler**: GCC 9+ or Clang 10+
- **Tools**: make, git

### Quick Start
```bash
# 1. Clone repository
git clone https://github.com/MaximKoblyuk/proton-web.git
cd proton-web

# 2. Build
make

# 3. Create minimal config
cat > /tmp/proton.conf << EOF
worker_processes 1;
events {
    worker_connections 1024;
}
http {
    server {
        listen 8080;
        location / {
            return 200 "Proton is running!\n";
        }
    }
}
EOF

# 4. Run
./build/proton -c /tmp/proton.conf

# 5. Test
curl http://localhost:8080/
# Output: Proton is running!
```

### Static File Example
```bash
mkdir -p /var/www/html
echo "<h1>Hello from Proton</h1>" > /var/www/html/index.html

cat > proton.conf << EOF
worker_processes auto;
events { worker_connections 1024; }
http {
    server {
        listen 8080;
        location / {
            root /var/www/html;
        }
    }
}
EOF

./build/proton -c proton.conf
curl http://localhost:8080/
```

### Reverse Proxy Example
```bash
# Start backend server
python3 -m http.server 3000 &

cat > proton.conf << EOF
worker_processes auto;
events { worker_connections 1024; }
http {
    upstream backend {
        server 127.0.0.1:3000;
    }
    server {
        listen 8080;
        location / {
            proxy_pass http://backend;
        }
    }
}
EOF

./build/proton -c proton.conf
curl http://localhost:8080/
```

---

## 12. Roadmap

### v0.1 - MVP (Q1 2026)
- ✅ Master/worker process model
- ✅ epoll event loop
- ✅ HTTP/1.1 parsing
- ✅ Static file serving
- ✅ Basic reverse proxy
- ✅ Configuration parser
- ✅ Graceful reload

### v0.2 - Production Ready (Q2 2026)
-  TLS/SSL support (OpenSSL)
-  gzip compression
-  Connection pooling
-  Load balancing (round-robin, least-conn)
-  Access/error logging
-  Full test suite

### v0.3 - Advanced Features (Q3 2026)
-  Content caching (memory + disk)
-  JWT authentication module
-  Rate limiting
-  Prometheus metrics endpoint
-  WebSocket proxy support
- 
### v1.0 - Stable Release (Q4 2026)
-  HTTP/2 support
-  Brotli compression
-  Performance optimizations (SIMD, zero-copy)
-  Complete documentation
-  Production deployments

---

## 13. Contributing

We welcome contributions! Please see [CONTRIBUTING.md](CONTRIBUTING.md) for guidelines.

### Development Setup
```bash
# Install dev dependencies (Ubuntu)
sudo apt-get install build-essential clang-format valgrind

# Build in debug mode
make debug

# Run with Valgrind
valgrind --leak-check=full ./build/proton -c proton.conf

# Format code
clang-format -i src/**/*.c include/*.h
```

### Code Style
- **Standard**: C11
- **Naming**: `snake_case` for functions/variables, `UPPER_CASE` for macros
- **Indentation**: 4 spaces (no tabs)
- **Max line length**: 100 characters

---

## License

MIT License - see [LICENSE](LICENSE) file.

---

## Design Philosophy

**Performance First**  
Every design decision prioritizes performance: event-driven I/O, zero-copy proxying, custom memory pools.

**Minimal Allocations**  
Use memory pools for request-scoped allocations, pre-allocate worker shared memory, avoid malloc/free in hot path.

**Explicit Ownership**  
No garbage collection. Every pointer has a clear owner responsible for freeing it.

**Event-Driven Design**  
Single-threaded workers with epoll. No blocking operations. Async all the way.

**Clear Separation**  
Master handles orchestration, workers handle requests. Modules are isolated. Python stays out of C's way.

---
<img width="1430" height="827" alt="image" src="https://github.com/user-attachments/assets/76547df0-2036-4eaf-a373-1b3dba568769" />

## Acknowledgments

Inspired by the architecture of:
- **NGINX** - Process model and event-driven design
- **HAProxy** - Load balancing strategies
- **Caddy** - Modern configuration approach

Built from scratch with ❤️ and C.

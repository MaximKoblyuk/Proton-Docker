# Building Proton Web Server

## System Requirements

Proton Web Server is designed for **Linux** systems and uses Linux-specific APIs (epoll).

### Supported Platforms
- ✅ **Linux** (Ubuntu 20.04+, CentOS 8+, Debian 11+, Fedora 35+)
- ✅ **WSL2** (Windows Subsystem for Linux 2)
- ❌ **Windows** (native) - not supported
- ⚠️ **macOS** - requires kqueue implementation (planned for v0.2)

## Prerequisites

### Linux / WSL2
```bash
# Ubuntu/Debian
sudo apt-get update
sudo apt-get install build-essential gcc make

# CentOS/RHEL
sudo yum groupinstall "Development Tools"

# Fedora
sudo dnf groupinstall "Development Tools"
```

## Building the Project

### 1. Clone or Navigate to Project
```bash
cd My-web-server---Proton-web
```

### 2. Build Options

**Standard Build (Optimized)**
```bash
make
```

**Debug Build (with AddressSanitizer)**
```bash
make debug
```

**Release Build (Maximum Optimization)**
```bash
make release
```

### 3. Verify Build
```bash
./build/proton -h
```

You should see:
```
Proton Web Server v0.1.0
Usage: ./build/proton [-c config_file] [-h]
  -c config_file  Specify configuration file
  -h              Show this help message
```

## Running the Server

### Basic Usage
```bash
./build/proton -c conf/proton.conf.example
```

### Testing
```bash
# In another terminal
curl http://localhost:8080/

# Or open in browser
# http://localhost:8080/
```

## Building on WSL2 (Windows)

If you're using Windows, you need WSL2:

### 1. Check WSL Installation
```powershell
wsl --version
```

### 2. Install Ubuntu (if not already)
```powershell
wsl --install -d Ubuntu
```

### 3. Open WSL Terminal
```powershell
wsl
```

### 4. Navigate to Project
```bash
# Your Windows drives are mounted under /mnt/
cd /mnt/c/path/to/My-web-server---Proton-web
```

### 5. Install Build Tools
```bash
sudo apt-get update
sudo apt-get install build-essential
```

### 6. Build
```bash
make
```

## Troubleshooting

### "make: command not found"
You need to install build tools (see Prerequisites above).

### "gcc: command not found"
```bash
sudo apt-get install gcc
```

### Permission Denied on Port 8080
```bash
# Change listen port in config file to 8080+ (non-privileged port)
# Or run with sudo (not recommended for development)
sudo ./build/proton -c conf/proton.conf.example
```

### "epoll_create1: Function not implemented"
You're probably on macOS or old kernel. Proton requires Linux kernel 2.6.27+ with epoll support.

## Clean Build

```bash
make clean
make
```

## Installation (Optional)

```bash
# System-wide installation (requires root)
sudo make install

# Now you can run from anywhere
proton -c /etc/proton/proton.conf
```

## Uninstallation

```bash
sudo make uninstall
```

## Development Workflow

```bash
# Edit code
vim src/core/worker.c

# Rebuild
make debug

# Run
./build/proton -c conf/proton.conf.example

# Test
curl http://localhost:8080/
```

## Next Steps

1. ✅ Build completed - Server binary at `./build/proton`
2. ✅ Test page ready - Located at `./public/index.html`
3. 🚀 Run server - `./build/proton -c conf/proton.conf.example`
4. 🌐 Open browser - http://localhost:8080/
5. 📖 Read docs - See [README.md](README.md) for architecture details

## Performance Testing

### Using curl
```bash
# Simple test
curl -v http://localhost:8080/

# Measure response time
time curl http://localhost:8080/
```

### Using ApacheBench
```bash
# Install ab
sudo apt-get install apache2-utils

# Run 10,000 requests with 100 concurrent
ab -n 10000 -c 100 http://localhost:8080/
```

### Using wrk
```bash
# Install wrk
sudo apt-get install wrk

# Run load test
wrk -t4 -c100 -d30s http://localhost:8080/
```

## Support

- 🐛 **Issues**: Report bugs on GitHub
- 💬 **Discussions**: Ask questions in GitHub Discussions
- 📧 **Contact**: See README.md for maintainer info

---

**Happy Building! 🚀**

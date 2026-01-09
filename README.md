# Proton Web Server - Docker Edition

**High-Performance Event-Driven Web Server in Docker**
![DALL·E 2026-01-08 15 29 27 - High‑quality modern vector logo for an open‑source infrastructure project named "Proton Web Server" with CLEAR, LEGIBLE text  Combine a minimalist abs](https://github.com/user-attachments/assets/f82660d8-4b8c-4119-838a-cd6704c547cb)

Production-ready Docker images for Proton Web Server - a lightweight, high-performance HTTP server built in C.

[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](LICENSE)
[![Docker](https://img.shields.io/badge/docker-ready-blue.svg)]()


---

## 🚀 Quick Start

Get Proton Web Server running in seconds with Docker:

```bash
# Pull and run (coming soon)
docker run -d -p 8080:8080 --name proton proton-web-server

# Or build locally
docker build -t proton-web-server .
docker run -d -p 8080:8080 --name proton proton-web-server

# Using Docker Compose (recommended)
docker compose up -d

# View logs
docker compose logs -f
```

Test the server:
```bash
curl http://localhost:8080/
```

---

## 📦 Docker Images

### Image Specifications

- **Base Image**: Alpine Linux (minimal footprint)
- **Image Size**: ~10-15MB (compressed)
- **Architecture**: Multi-architecture support (amd64, arm64)
- **User**: Runs as non-root user (`proton`)
- **Port**: Exposes port 8080 by default

### Available Tags

```bash
# Latest stable release
docker pull proton-web-server:latest

# Specific version
docker pull proton-web-server:0.1.0

# Development build
docker pull proton-web-server:dev
```

---

## 🛠️ Building the Docker Image

### Basic Build

```bash
# Build with default settings
docker build -t proton-web-server:latest .

# Build with specific tag
docker build -t proton-web-server:0.1.0 .

# View build output
docker build --progress=plain -t proton-web-server .
```

### Using the Build Script

The included build script provides advanced options:

```bash
# Make executable (first time)
chmod +x build-docker.sh

# Standard build
./build-docker.sh

# Build with custom version
./build-docker.sh --version 1.0.0

# Build for multiple architectures
./build-docker.sh --multi-arch

# Build and push to registry
./build-docker.sh --registry docker.io/yourusername --push
```

### Multi-Stage Build Process

The Dockerfile uses an optimized multi-stage build:

1. **Builder Stage**: Compiles C source code with Alpine build tools
2. **Runtime Stage**: Creates minimal production image with only runtime dependencies

This approach ensures:
- Small final image size (~10-15MB)
- No build tools in production image
- Enhanced security (minimal attack surface)
- Fast image pulls and deployments

---

## 🚢 Running the Container

### Basic Docker Run

```bash
# Run in detached mode
docker run -d -p 8080:8080 --name proton proton-web-server

# Run in foreground with logs
docker run --rm -p 8080:8080 proton-web-server

# Custom port mapping
docker run -d -p 9000:8080 --name proton proton-web-server

# With restart policy
docker run -d -p 8080:8080 --restart unless-stopped --name proton proton-web-server
```

### With Volume Mounts

Customize content and configuration:

```bash
docker run -d \
  -p 8080:8080 \
  -v $(pwd)/public:/var/www/html:ro \
  -v $(pwd)/conf/proton.conf.example:/etc/proton/proton.conf:ro \
  --name proton \
  proton-web-server
```

### With Environment Variables

Configure runtime behavior:

```bash
docker run -d \
  -p 8080:8080 \
  -e PROTON_WORKERS=4 \
  -e PROTON_LOG_LEVEL=info \
  -e TZ=America/New_York \
  --name proton \
  proton-web-server
```

### Container Management

```bash
# View logs
docker logs proton
docker logs -f proton  # Follow logs

# Check status
docker ps | grep proton

# Execute commands in container
docker exec -it proton sh

# Restart container
docker restart proton

# Stop and remove
docker stop proton
docker rm proton
```

---

## 🐳 Docker Compose

### Development Setup

The included `docker-compose.yml` provides a complete development environment:

```yaml
services:
  proton:
    build: .
    ports:
      - "8080:8080"
    volumes:
      - ./public:/var/www/html:ro
    environment:
      - PROTON_WORKERS=4
      - PROTON_LOG_LEVEL=info
    healthcheck:
      test: ["CMD", "curl", "-f", "http://localhost:8080/"]
      interval: 30s
      timeout: 3s
      retries: 3
    restart: unless-stopped
```

### Usage Commands

```bash
# Start services
docker compose up -d

# View logs
docker compose logs -f

# Restart services
docker compose restart

# Stop services
docker compose stop

# Stop and remove containers
docker compose down

# Rebuild and start
docker compose up -d --build

# Scale workers (if supported)
docker compose up -d --scale proton=3
```

### Production Setup

For production deployments, use `docker-compose.prod.yml`:

```bash
# Start production environment
docker compose -f docker-compose.prod.yml up -d

# View production logs
docker compose -f docker-compose.prod.yml logs -f
```

---

## ⚙️ Configuration

### Environment Variables

Configure the container using these environment variables:

| Variable | Default | Description |
|----------|---------|-------------|
| `PROTON_WORKERS` | Auto (CPU cores) | Number of worker processes |
| `PROTON_LOG_LEVEL` | `info` | Logging level: debug, info, warn, error |
| `TZ` | `UTC` | Container timezone |
| `PROTON_ENTRYPOINT_QUIET_LOGS` | `0` | Set to `1` to disable entrypoint logs |

### Volume Mounts

Common volume mount points:

| Container Path | Purpose | Example Mount |
|----------------|---------|---------------|
| `/var/www/html` | Static web content | `-v ./public:/var/www/html:ro` |
| `/etc/proton/proton.conf` | Server configuration | `-v ./proton.conf:/etc/proton/proton.conf:ro` |
| `/var/log/proton` | Log files | `-v ./logs:/var/log/proton` |

### Custom Configuration File

Mount your own configuration:

```bash
# Create custom config
cp conf/proton.conf.example my-proton.conf

# Edit configuration
nano my-proton.conf

# Run with custom config
docker run -d \
  -p 8080:8080 \
  -v $(pwd)/my-proton.conf:/etc/proton/proton.conf:ro \
  proton-web-server
```

---

## 🏥 Health Checks

### Built-in Health Monitoring

The Docker image includes automatic health checks:

```dockerfile
HEALTHCHECK --interval=30s --timeout=3s --start-period=5s --retries=3 \
  CMD curl -f http://localhost:8080/ || exit 1
```

### Check Health Status

```bash
# View health status
docker inspect --format='{{.State.Health.Status}}' proton

# View health check logs
docker inspect --format='{{range .State.Health.Log}}{{.Output}}{{end}}' proton
```

### Custom Health Check

Override the default health check:

```bash
docker run -d \
  -p 8080:8080 \
  --health-cmd='curl -f http://localhost:8080/health || exit 1' \
  --health-interval=10s \
  --health-timeout=5s \
  --health-retries=3 \
  proton-web-server
```

---

## 🔒 Security

### Security Features

- **Non-root user**: Container runs as user `proton` (UID 1000)
- **Minimal image**: Alpine-based with only essential dependencies
- **Read-only root filesystem**: Supports `--read-only` flag
- **No unnecessary privileges**: Drops all capabilities except required ones

### Running with Enhanced Security

```bash
docker run -d \
  -p 8080:8080 \
  --read-only \
  --tmpfs /tmp \
  --cap-drop ALL \
  --cap-add NET_BIND_SERVICE \
  --security-opt=no-new-privileges \
  proton-web-server
```

### Security Best Practices

1. **Always use specific image tags** (not `latest`) in production
2. **Scan images regularly** for vulnerabilities
3. **Run as non-root** (enabled by default)
4. **Use secrets management** for sensitive configuration
5. **Keep images updated** with security patches
6. **Limit resource usage** with Docker resource constraints

```bash
# Resource limits
docker run -d \
  -p 8080:8080 \
  --memory="256m" \
  --cpus="1.0" \
  --pids-limit 100 \
  proton-web-server
```

---

## 📊 Monitoring and Logs

### Viewing Logs

```bash
# View all logs
docker logs proton

# Follow logs in real-time
docker logs -f proton

# View last 100 lines
docker logs --tail 100 proton

# View logs with timestamps
docker logs -t proton

# Logs since specific time
docker logs --since 10m proton
```

### Log Management with Docker Compose

```bash
# View all service logs
docker compose logs

# Follow specific service
docker compose logs -f proton

# View last 50 lines
docker compose logs --tail=50
```

### Container Statistics

```bash
# Real-time stats
docker stats proton

# One-time stats
docker stats --no-stream proton

# All containers
docker stats
```

---

## 🚀 Production Deployment

### Production Checklist

- [ ] Use specific image tags (not `latest`)
- [ ] Configure health checks
- [ ] Set up log aggregation
- [ ] Implement resource limits
- [ ] Use secrets for sensitive data
- [ ] Enable restart policies
- [ ] Configure proper networking
- [ ] Set up monitoring/alerting
- [ ] Implement backup strategy
- [ ] Test rollback procedures

### Production Docker Run Example

```bash
docker run -d \
  --name proton \
  -p 8080:8080 \
  -v /opt/proton/public:/var/www/html:ro \
  -v /opt/proton/config/proton.conf:/etc/proton/proton.conf:ro \
  -e PROTON_WORKERS=8 \
  -e PROTON_LOG_LEVEL=warn \
  -e TZ=UTC \
  --memory="512m" \
  --cpus="2.0" \
  --restart unless-stopped \
  --health-cmd='curl -f http://localhost:8080/ || exit 1' \
  --health-interval=30s \
  --health-retries=3 \
  --log-opt max-size=10m \
  --log-opt max-file=3 \
  proton-web-server:0.1.0
```

### Production Docker Compose Example

```yaml
services:
  proton:
    image: proton-web-server:0.1.0
    deploy:
      replicas: 3
      resources:
        limits:
          cpus: '2'
          memory: 512M
        reservations:
          cpus: '1'
          memory: 256M
      restart_policy:
        condition: on-failure
        delay: 5s
        max_attempts: 3
    ports:
      - "8080:8080"
    volumes:
      - /opt/proton/public:/var/www/html:ro
      - /opt/proton/config:/etc/proton:ro
    environment:
      - PROTON_WORKERS=8
      - PROTON_LOG_LEVEL=warn
    healthcheck:
      test: ["CMD", "curl", "-f", "http://localhost:8080/"]
      interval: 30s
      timeout: 5s
      retries: 3
    logging:
      driver: "json-file"
      options:
        max-size: "10m"
        max-file: "3"
```

---

## 🔧 Troubleshooting

### Common Issues

#### Container Won't Start

```bash
# Check logs
docker logs proton

# Check if port is already in use
netstat -ano | findstr :8080  # Windows
lsof -i :8080                 # Linux/Mac

# Try different port
docker run -d -p 9000:8080 --name proton proton-web-server
```

#### Worker Processes Crashing

```bash
# Check detailed logs
docker logs proton

# Run in foreground for debugging
docker run --rm -e PROTON_LOG_LEVEL=debug proton-web-server

# Reduce worker count
docker run -d -e PROTON_WORKERS=1 proton-web-server
```

#### Health Check Failing

```bash
# Check health status
docker inspect --format='{{.State.Health.Status}}' proton

# Test manually
docker exec proton curl -f http://localhost:8080/

# Increase health check timeout
docker run -d --health-timeout=10s proton-web-server
```

#### Permission Issues

```bash
# Check file permissions on mounted volumes
ls -la ./public

# Fix permissions
chmod -R 755 ./public

# Run with user mapping
docker run -d --user 1000:1000 proton-web-server
```

### Debugging Commands

```bash
# Enter container shell
docker exec -it proton sh

# Check process status
docker exec proton ps aux

# View container configuration
docker inspect proton

# Check resource usage
docker stats proton

# View network settings
docker port proton
```

---

## 📚 Additional Resources

- **Full Documentation**: See [DOCKER.md](DOCKER.md) for comprehensive Docker details
- **Build Instructions**: See [BUILD.md](BUILD.md) for compilation details
- **Quick Start Guide**: See [QUICKSTART.md](QUICKSTART.md)
- **Configuration**: See `conf/proton.conf.example` for configuration options

---

## 🤝 Contributing

Contributions are welcome! Please ensure:
- Docker images build successfully
- Documentation is updated
- Changes are tested with Docker Compose

---

## 📄 License

MIT License - See [LICENSE](LICENSE) file for details

---

## ⚠️ Current Status

**Version**: 0.1.0 (Development)

- ✅ Docker container builds successfully (~10-15MB)
- ✅ Master process runs stably
- ✅ Workers spawn automatically
- ⚠️ Worker HTTP handling has known issues (workers may crash and respawn)
- ✅ Health checks monitor server status

**Note**: This is an active development project. The HTTP handling implementation is being refined for production stability.

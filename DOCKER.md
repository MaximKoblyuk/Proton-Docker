# Docker Documentation for Proton Web Server

This document provides comprehensive information about running Proton Web Server in Docker containers.

## Table of Contents

- [Quick Start](#quick-start)
- [Building the Image](#building-the-image)
- [Running the Container](#running-the-container)
- [Configuration](#configuration)
- [Docker Compose](#docker-compose)
- [Health Checks and Monitoring](#health-checks-and-monitoring)
- [Multi-Architecture Support](#multi-architecture-support)
- [Production Deployment](#production-deployment)
- [Troubleshooting](#troubleshooting)
- [Security Considerations](#security-considerations)

## Quick Start

The fastest way to get Proton Web Server running:

```bash
# Build the Docker image
docker build -t proton-web-server .

# Run the container
docker run -d -p 8080:8080 --name proton proton-web-server

# Check if it's running
docker ps | grep proton

# View logs to confirm startup
docker logs proton

# Test the server (Note: Workers are currently unstable during HTTP handling)
# curl http://localhost:8080/
```

**Current Status:**
- ✅ Container builds successfully (~25MB final size)
- ✅ Master process runs stably
- ✅ Workers spawn automatically
- ⚠️  Worker HTTP handling has known issues (workers crash and respawn)
- ✅ Health checks monitor server status

## Building the Image

### Standard Build

Build the Docker image using the multi-stage Dockerfile:

```bash
docker build -t proton-web-server:0.1.0 .
```

### Using the Build Script

The included `build-docker.sh` script provides additional options:

```bash
# Make the script executable (first time only)
chmod +x build-docker.sh

# Build with default settings
./build-docker.sh

# Build specific version
./build-docker.sh --version 1.0.0

# Build and push to registry
./build-docker.sh --registry docker.io/myuser --push

# Build multi-architecture images
./build-docker.sh --registry docker.io/myuser --multi-arch --push
```

### Build Arguments

The Dockerfile uses a multi-stage build:

- **Stage 1 (Builder)**: Compiles the application using Alpine Linux with build tools
- **Stage 2 (Runtime)**: Creates minimal runtime image (~10-15MB) with only necessary dependencies

### Image Size Optimization

The final image is optimized for size:
- Multi-stage build removes build dependencies
- Alpine Linux base (~5MB)
- Static binary (~42KB)
- Minimal runtime dependencies (curl, tzdata)

Expected final image size: **10-15MB**

## Running the Container

### Basic Usage

```bash
# Run in foreground (see all logs)
docker run --rm -p 8080:8080 proton-web-server

# Run in background (detached)
docker run -d -p 8080:8080 --name proton proton-web-server

# Run with custom port mapping
docker run -d -p 9000:8080 --name proton proton-web-server

# View live logs
docker logs -f proton

# Check server status
docker ps -a | grep proton

# Stop the container
docker stop proton

# Remove the container
docker rm proton
```

**Container Lifecycle:**
- The entrypoint script initializes worker process tuning
- Master process spawns worker processes based on CPU cores
- Workers handle incoming HTTP connections
- Health checks run every 30 seconds
- Container uses PID 1 signal handling for graceful shutdown

### With Volume Mounts

Mount local directories for custom content and configuration:

```bash
docker run -d \
  -p 8080:8080 \
  -v $(pwd)/public:/var/www/html:ro \
  -v $(pwd)/conf/proton.conf.example:/etc/proton/proton.conf:ro \
  --name proton \
  proton-web-server
```

### With Environment Variables

Configure the server using environment variables:

```bash
docker run -d \
  -p 8080:8080 \
  -e PROTON_WORKERS=4 \
  -e PROTON_LOG_LEVEL=debug \
  -e TZ=America/New_York \
  --name proton \
  proton-web-server
```

## Configuration

### Environment Variables

| Variable | Description | Default | Example |
|----------|-------------|---------|---------|
| `PROTON_WORKERS` | Number of worker processes | Auto-detected (CPU cores) | `4` |
| `PROTON_LOG_LEVEL` | Logging level | `info` | `debug`, `info`, `warn`, `error` |
| `TZ` | Timezone | `UTC` | `America/New_York` |
| `PROTON_ENTRYPOINT_QUIET_LOGS` | Disable entrypoint logs | Not set | `1` |
| `PROTON_PORT` | Server port (template mode) | `8080` | `9000` |
| `PROTON_DOC_ROOT` | Document root (template mode) | `/var/www/html` | `/app/public` |

### Volume Mounts

| Container Path | Purpose | Recommended Mode |
|----------------|---------|------------------|
| `/var/www/html` | Document root (web files) | `ro` (read-only) |
| `/etc/proton/proton.conf` | Configuration file | `ro` (read-only) |
| `/var/log/proton` | Log files (forwarded to stdout/stderr) | - |

### Configuration Templates

To use environment variable substitution in configuration:

1. Create a template file: `conf/proton.conf.template`
2. Use `${VARIABLE_NAME}` syntax for substitution
3. Mount the template as `/etc/proton/proton.conf.template`
4. The entrypoint script will process it automatically

Example template:

```nginx
worker_processes ${PROTON_WORKERS};

http {
    server {
        listen ${PROTON_PORT};
        root ${PROTON_DOC_ROOT};
    }
}
```

## Docker Compose

### Development Setup

Use `docker-compose.yml` for local development with hot-reload:

```bash
# Start the service
docker-compose up -d

# View logs
docker-compose logs -f

# Stop the service
docker-compose down
```

Features:
- Volume mounts for live file updates
- Easy environment variable configuration
- Automatic restart on failure
- Health checks enabled

### Production Setup

Use `docker-compose.prod.yml` for production with NGINX load balancer:

```bash
# Start production stack with 3 replicas
docker-compose -f docker-compose.prod.yml up -d

# Scale replicas
docker-compose -f docker-compose.prod.yml up -d --scale proton=5

# Check status
docker-compose -f docker-compose.prod.yml ps

# View logs
docker-compose -f docker-compose.prod.yml logs -f proton

# Stop production stack
docker-compose -f docker-compose.prod.yml down
```

Features:
- NGINX load balancer on port 80
- Multiple Proton instances (default: 3)
- Resource limits (CPU, memory)
- Health checks for high availability
- Automatic restart policy

### Accessing the Services

```bash
# Development (direct access)
curl http://localhost:8080/

# Production (through NGINX load balancer)
curl http://localhost/

# NGINX health endpoint
curl http://localhost/nginx-health
```

## Health Checks and Monitoring

### Container Health

The container includes built-in health checks:

```bash
# Check health status
docker inspect proton | grep -A 10 Health

# View health check logs
docker inspect proton | jq '.[0].State.Health'
```

Health check configuration:
- **Interval**: 30 seconds
- **Timeout**: 3 seconds
- **Retries**: 3
- **Start period**: 5 seconds

### Viewing Logs

All logs are forwarded to Docker's logging system:

```bash
# View all logs
docker logs proton

# Follow logs in real-time
docker logs -f proton

# View last 100 lines
docker logs --tail 100 proton

# View logs with timestamps
docker logs -t proton
```

### Resource Monitoring

Monitor container resource usage:

```bash
# Real-time stats
docker stats proton

# One-time snapshot
docker stats --no-stream proton
```

## Multi-Architecture Support

The build system supports multiple architectures using Docker Buildx.

### Supported Platforms

- `linux/amd64` - x86_64 / AMD64
- `linux/arm64` - ARM 64-bit (Apple Silicon, AWS Graviton)

### Building Multi-Arch Images

```bash
# Create and use buildx builder
docker buildx create --name proton-builder --use

# Build for multiple platforms
docker buildx build \
  --platform linux/amd64,linux/arm64 \
  -t myregistry/proton-web-server:0.1.0 \
  --push \
  .

# Or use the build script
./build-docker.sh --registry myregistry --multi-arch --push
```

### Platform-Specific Builds

```bash
# Build for ARM64 only
docker buildx build \
  --platform linux/arm64 \
  -t proton-web-server:arm64 \
  .
```

## Production Deployment

### Best Practices

1. **Resource Limits**: Always set CPU and memory limits

```yaml
deploy:
  resources:
    limits:
      cpus: '0.5'
      memory: 256M
```

2. **Replicas**: Run multiple instances for high availability

```bash
docker-compose -f docker-compose.prod.yml up -d --scale proton=3
```

3. **Health Checks**: Enable health checks for automatic recovery

4. **Logging**: Configure centralized logging (e.g., ELK stack, Splunk)

```bash
docker run -d \
  --log-driver=json-file \
  --log-opt max-size=10m \
  --log-opt max-file=3 \
  proton-web-server
```

5. **Secrets Management**: Use Docker secrets or environment files

```bash
docker run -d --env-file .env.production proton-web-server
```

6. **Network Security**: Use custom networks and firewall rules

```bash
docker network create --driver bridge proton-network
```

### Load Balancing

The production setup includes NGINX as a load balancer:

- **Algorithm**: Least connections (`least_conn`)
- **Health checks**: Automatic failover
- **Keepalive**: Connection pooling to backends
- **Headers**: X-Real-IP, X-Forwarded-For forwarding

### Rolling Updates

```bash
# Build new version
docker build -t proton-web-server:0.2.0 .

# Update production deployment
docker-compose -f docker-compose.prod.yml up -d --no-deps --build proton
```

### Backup and Recovery

```bash
# Export configuration
docker cp proton:/etc/proton/proton.conf ./backup/

# Export logs
docker logs proton > ./backup/proton-logs-$(date +%Y%m%d).log
```

## Known Issues

### Worker Process Stability

**Issue:** Worker processes crash when handling HTTP requests and are continuously respawned by the master process.

**Symptoms:**
```
[WARN] Worker 0 (pid=123) died, respawning
[WARN] Worker 1 (pid=124) died, respawning
```

**Status:** The master process correctly detects and respawns crashed workers. This is expected behavior while worker HTTP handling code is being stabilized.

**Fixed Issues (v0.1.0):**
- ✅ `trim()` function buffer underflow with empty strings
- ✅ `strdup()` crashes in Alpine musl libc (replaced with malloc+strcpy)
- ✅ Config parser memory initialization
- ✅ Master process stability
- ✅ Signal handling and graceful shutdown

### HTTP Connection Handling

**Issue:** HTTP connections may close unexpectedly due to worker crashes.

**Workaround:** Monitor logs with `docker logs -f <container>` to see worker respawn activity.

**Future:** Worker process HTTP handling improvements are in progress.

## Troubleshooting

### Common Issues

#### Container Fails to Start

**Symptom**: Container exits immediately after starting

**Solutions**:
```bash
# Check logs
docker logs proton

# Check configuration
docker exec proton cat /etc/proton/proton.conf

# Verify binary
docker exec proton /usr/local/bin/proton --version
```

#### Port Already in Use

**Symptom**: Error: "port is already allocated"

**Solutions**:
```bash
# Check what's using the port
netstat -ano | findstr :8080  # Windows
lsof -i :8080                  # Linux/Mac

# Use different port
docker run -p 9000:8080 proton-web-server
```

#### Health Check Failing

**Symptom**: Container marked as "unhealthy"

**Solutions**:
```bash
# Test health check manually
docker exec proton curl -f http://localhost:8080/

# Check if server is listening
docker exec proton netstat -tlnp

# View detailed health check logs
docker inspect proton | jq '.[0].State.Health'
```

#### Cannot Access from Host

**Symptom**: Connection refused when accessing http://localhost:8080

**Solutions**:
```bash
# Verify container is running
docker ps | grep proton

# Check port mapping
docker port proton

# Test from inside container
docker exec proton curl http://localhost:8080/

# Check firewall rules
# Windows: Check Windows Firewall
# Linux: sudo iptables -L
```

#### Permission Issues

**Symptom**: Permission denied errors

**Solutions**:
```bash
# The container runs as non-root user 'proton' (UID 101)
# Ensure mounted volumes have appropriate permissions

# Fix permissions on host
chmod -R 755 ./public
chown -R 101:101 ./public  # Match container UID/GID
```

### Debugging Tips

1. **Interactive Shell**:
```bash
# Start shell in running container
docker exec -it proton sh

# Or start new container with shell
docker run -it --rm proton-web-server sh
```

2. **Verbose Logging**:
```bash
docker run -e PROTON_LOG_LEVEL=debug proton-web-server
```

3. **Inspect Configuration**:
```bash
# View configuration
docker exec proton cat /etc/proton/proton.conf

# Check environment variables
docker exec proton env
```

4. **Network Debugging**:
```bash
# Install debugging tools
docker exec proton apk add --no-cache tcpdump

# Check network connectivity
docker exec proton ping -c 3 google.com
```

### Getting Help

If you encounter issues not covered here:

1. Check the logs: `docker logs proton`
2. Verify configuration: `docker exec proton cat /etc/proton/proton.conf`
3. Test connectivity: `docker exec proton curl http://localhost:8080/`
4. Review the main [README.md](README.md) and [BUILD.md](BUILD.md)

## Security Considerations

### Non-Root User

The container runs as a non-root user (`proton`, UID 101) for security:

```dockerfile
USER proton
```

### Read-Only Filesystem

For additional security, run with read-only root filesystem:

```bash
docker run -d \
  --read-only \
  --tmpfs /tmp \
  --tmpfs /var/log/proton \
  -p 8080:8080 \
  proton-web-server
```

### Security Scanning

Scan the image for vulnerabilities:

```bash
# Using Docker Scout
docker scout cves proton-web-server

# Using Trivy
trivy image proton-web-server
```

### Minimal Attack Surface

- Alpine Linux base (minimal packages)
- No unnecessary tools or libraries
- Single static binary
- No shell interpreters in final image (except /bin/sh for entrypoint)

### Network Security

```bash
# Run on custom network
docker network create --driver bridge --subnet 172.18.0.0/16 proton-network
docker run -d --network proton-network proton-web-server

# Limit inter-container communication
docker network create --internal proton-internal
```

### Secret Management

Never embed secrets in the image:

```bash
# Use environment files
docker run -d --env-file .env.secret proton-web-server

# Or use Docker secrets (Swarm mode)
echo "secret_value" | docker secret create my_secret -
docker service create --secret my_secret proton-web-server
```

---

## Additional Resources

- [Main README](README.md) - Project overview
- [BUILD.md](BUILD.md) - Building from source
- [QUICKSTART.md](QUICKSTART.md) - Quick start guide
- [nginx/docker-nginx](https://github.com/nginx/docker-nginx) - Reference implementation

## License

Same as the main Proton Web Server project.

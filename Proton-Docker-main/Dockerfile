# Multi-stage Alpine build for Proton Web Server
# Following nginx/docker-nginx best practices

# ============================================================================
# Stage 1: Builder - Compile the application
# ============================================================================
FROM alpine:3.20 AS builder

# Install build dependencies including AddressSanitizer runtime
RUN apk add --no-cache \
    gcc \
    libc-dev \
    make \
    linux-headers \
    libgcc

# Set working directory
WORKDIR /build

# Copy source code
COPY Makefile ./
COPY src/ ./src/
COPY include/ ./include/

# Build with debugging symbols and no optimization to avoid compiler bugs
RUN make CFLAGS="-Wall -Wextra -std=c11 -Iinclude -g -O0" clean all

# ============================================================================
# Stage 2: Runtime - Minimal production image
# ============================================================================
FROM alpine:3.20

# Metadata
LABEL maintainer="Proton Web Server"
LABEL description="NGINX-inspired web server written in C"
LABEL version="0.1.0"

# Install runtime dependencies including ASan runtime
RUN apk add --no-cache \
    curl \
    tzdata \
    libgcc \
    && addgroup -g 101 -S proton \
    && adduser -S -D -H -u 101 -h /var/cache/proton -s /sbin/nologin -G proton -g proton proton

# Create necessary directories
RUN mkdir -p /etc/proton \
    && mkdir -p /var/www/html \
    && mkdir -p /var/log/proton \
    && mkdir -p /docker-entrypoint.d \
    && mkdir -p /var/cache/proton \
    && chown -R proton:proton /etc/proton \
    && chown -R proton:proton /var/www/html \
    && chown -R proton:proton /var/log/proton \
    && chown -R proton:proton /var/cache/proton

# Copy binary from builder stage
COPY --from=builder /build/build/proton /usr/local/bin/proton

# Copy configuration files
COPY conf/proton.conf.example /etc/proton/proton.conf

# Copy public files
COPY public/ /var/www/html/

# Forward request and error logs to docker log collector
RUN ln -sf /dev/stdout /var/log/proton/access.log \
    && ln -sf /dev/stderr /var/log/proton/error.log

# Copy entrypoint scripts
COPY docker-entrypoint.sh /docker-entrypoint.sh
COPY docker-entrypoint.d/ /docker-entrypoint.d/

# Make entrypoint scripts executable
RUN chmod +x /docker-entrypoint.sh \
    && chmod +x /docker-entrypoint.d/*.sh

# Switch to non-root user
USER proton

# Expose HTTP port
EXPOSE 8080

# Use SIGQUIT for graceful shutdown (following NGINX pattern)
STOPSIGNAL SIGQUIT

# Health check
HEALTHCHECK --interval=30s --timeout=3s --start-period=5s --retries=3 \
    CMD curl -f http://localhost:8080/ || exit 1

# Entrypoint and command
ENTRYPOINT ["/docker-entrypoint.sh"]
CMD ["proton", "-c", "/etc/proton/proton.conf"]

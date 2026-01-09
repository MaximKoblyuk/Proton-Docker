#!/bin/sh
# Auto-tune worker processes based on available CPU cores

set -e

# Source the logging function from entrypoint
entrypoint_log() {
    if [ -z "${PROTON_ENTRYPOINT_QUIET_LOGS:-}" ]; then
        echo "$@"
    fi
}

# Check if PROTON_WORKERS is set
if [ -n "${PROTON_WORKERS:-}" ]; then
    entrypoint_log "10-tune-worker-processes: Using PROTON_WORKERS=${PROTON_WORKERS}"
    WORKERS=$PROTON_WORKERS
else
    # Auto-detect number of CPU cores
    if [ -f /proc/cpuinfo ]; then
        WORKERS=$(grep -c ^processor /proc/cpuinfo 2>/dev/null || echo 1)
        entrypoint_log "10-tune-worker-processes: Auto-detected ${WORKERS} CPU cores"
    else
        WORKERS=1
        entrypoint_log "10-tune-worker-processes: Could not detect CPU cores, defaulting to 1"
    fi
fi

# Update config file if it exists
CONFIG_FILE="/etc/proton/proton.conf"
if [ -f "$CONFIG_FILE" ]; then
    # Check if worker_processes directive exists
    if grep -q "worker_processes" "$CONFIG_FILE"; then
        # Try to update in-place if writable, otherwise skip
        if [ -w "$CONFIG_FILE" ]; then
            sed -i "s/worker_processes [0-9]\+;/worker_processes ${WORKERS};/" "$CONFIG_FILE"
            entrypoint_log "10-tune-worker-processes: Set worker_processes to ${WORKERS}"
        else
            entrypoint_log "10-tune-worker-processes: Config file is read-only, skipping worker tuning"
        fi
    else
        entrypoint_log "10-tune-worker-processes: Warning - worker_processes directive not found in config"
    fi
else
    entrypoint_log "10-tune-worker-processes: Warning - config file not found at ${CONFIG_FILE}"
fi

#!/bin/sh
# Main entrypoint script for Proton Web Server container
# Following nginx/docker-nginx entrypoint.d pattern

set -e

# Function for logging
entrypoint_log() {
    if [ -z "${PROTON_ENTRYPOINT_QUIET_LOGS:-}" ]; then
        echo "$@"
    fi
}

# Check if we're running the proton binary
if [ "$1" = "proton" ] || [ "$1" = "/usr/local/bin/proton" ]; then
    entrypoint_log "Proton entrypoint: starting initialization"
    
    # Find and execute init scripts in /docker-entrypoint.d/
    if [ -d /docker-entrypoint.d/ ]; then
        entrypoint_log "Proton entrypoint: looking for init scripts in /docker-entrypoint.d/"
        
        # Find all .sh files, sort them, and execute
        find /docker-entrypoint.d/ -type f -name "*.sh" -print | sort -V | while read -r script; do
            case "$script" in
                *.sh)
                    if [ -x "$script" ]; then
                        entrypoint_log "Proton entrypoint: launching $script"
                        "$script"
                    else
                        entrypoint_log "Proton entrypoint: ignoring $script (not executable)"
                    fi
                    ;;
                *)
                    entrypoint_log "Proton entrypoint: ignoring $script"
                    ;;
            esac
        done
        
        entrypoint_log "Proton entrypoint: initialization complete"
    fi
fi

# Execute the main command
exec "$@"

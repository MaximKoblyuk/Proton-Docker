#!/bin/sh
# Process configuration templates with environment variable substitution

set -e

# Source the logging function from entrypoint
entrypoint_log() {
    if [ -z "${PROTON_ENTRYPOINT_QUIET_LOGS:-}" ]; then
        echo "$@"
    fi
}

# Check if template file exists
TEMPLATE_FILE="/etc/proton/proton.conf.template"
CONFIG_FILE="/etc/proton/proton.conf"

if [ -f "$TEMPLATE_FILE" ]; then
    entrypoint_log "20-envsubst-on-templates: Processing template $TEMPLATE_FILE"
    
    # Check if envsubst is available
    if ! command -v envsubst >/dev/null 2>&1; then
        entrypoint_log "20-envsubst-on-templates: Warning - envsubst not available, installing gettext"
        apk add --no-cache gettext
    fi
    
    # Define which environment variables to substitute
    # Add more variables as needed
    TEMPLATE_VARS='$PROTON_PORT:$PROTON_WORKERS:$PROTON_LOG_LEVEL:$PROTON_DOC_ROOT'
    
    # Perform substitution
    envsubst "$TEMPLATE_VARS" < "$TEMPLATE_FILE" > "$CONFIG_FILE"
    
    entrypoint_log "20-envsubst-on-templates: Generated $CONFIG_FILE from template"
else
    entrypoint_log "20-envsubst-on-templates: No template file found at $TEMPLATE_FILE, skipping"
fi

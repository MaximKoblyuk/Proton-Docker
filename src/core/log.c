#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <time.h>
#include <unistd.h>
#include "proton.h"

static FILE *log_file = NULL;
static int log_level = LOG_INFO;

static const char* level_strings[] = {
    "DEBUG", "INFO", "WARN", "ERROR"
};

void proton_log_init(const char *filename, int level) {
    log_level = level;
    
    if (!filename || strcmp(filename, "stderr") == 0) {
        log_file = stderr;
        return;
    }
    
    log_file = fopen(filename, "a");
    if (!log_file) {
        fprintf(stderr, "Failed to open log file: %s\n", filename);
        log_file = stderr;
    }
}

void proton_log(int level, const char *fmt, ...) {
    if (level < log_level) return;
    if (!log_file) log_file = stderr;
    
    /* Get timestamp */
    time_t now = time(NULL);
    struct tm *tm_info = localtime(&now);
    char timestamp[32];
    strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", tm_info);
    
    /* Print log entry */
    fprintf(log_file, "[%s] [%s] [%d] ", 
            timestamp, 
            level_strings[level], 
            getpid());
    
    va_list args;
    va_start(args, fmt);
    vfprintf(log_file, fmt, args);
    va_end(args);
    
    fprintf(log_file, "\n");
    fflush(log_file);
}

void proton_log_close(void) {
    if (log_file && log_file != stderr) {
        fclose(log_file);
        log_file = NULL;
    }
}

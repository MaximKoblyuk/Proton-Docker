#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <sys/types.h>
#include "proton.h"

/* Global state */
volatile sig_atomic_t proton_quit = 0;
volatile sig_atomic_t proton_reload = 0;
pid_t proton_pid;

/* Signal handlers */
static void signal_handler(int signo) {
    switch (signo) {
        case SIGINT:
        case SIGTERM:
            proton_quit = 1;
            break;
        case SIGHUP:
            proton_reload = 1;
            break;
        case SIGCHLD:
            /* Child process terminated */
            break;
    }
}

static void setup_signals(void) {
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = signal_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;

    sigaction(SIGINT, &sa, NULL);
    sigaction(SIGTERM, &sa, NULL);
    sigaction(SIGHUP, &sa, NULL);
    sigaction(SIGCHLD, &sa, NULL);
    
    /* Ignore SIGPIPE */
    signal(SIGPIPE, SIG_IGN);
}

static void usage(const char *prog) {
    fprintf(stderr, "Proton Web Server v%s\n", PROTON_VERSION);
    fprintf(stderr, "Usage: %s [-c config_file] [-h]\n", prog);
    fprintf(stderr, "  -c config_file  Specify configuration file\n");
    fprintf(stderr, "  -h              Show this help message\n");
}

int main(int argc, char *argv[]) {
    const char *config_file = "proton.conf";
    int opt;

    /* Parse command line arguments */
    while ((opt = getopt(argc, argv, "c:h")) != -1) {
        switch (opt) {
            case 'c':
                config_file = optarg;
                break;
            case 'h':
                usage(argv[0]);
                return 0;
            default:
                usage(argv[0]);
                return 1;
        }
    }

    proton_pid = getpid();

    printf("Proton Web Server v%s starting...\n", PROTON_VERSION);
    printf("Configuration file: %s\n", config_file);
    fflush(stdout);

    /* Parse configuration */
    fprintf(stderr, "[MAIN] About to parse config\n");
    proton_config_t *config = proton_config_parse(config_file);
    fprintf(stderr, "[MAIN] Config parser returned: %p\n", (void*)config);
    
    if (!config) {
        fprintf(stderr, "Failed to parse configuration file\n");
        return 1;
    }
    
    fprintf(stderr, "[MAIN] Config parsed OK, initializing logging\n");
    fprintf(stderr, "[MAIN] config->error_log ptr: %p\n", (void*)config->error_log);

    /* Initialize logging - use stderr if error_log is NULL */
    const char *log_file = config->error_log ? config->error_log : "stderr";
    fprintf(stderr, "[MAIN] Log file: %s\n", log_file);
    fprintf(stderr, "[MAIN] Calling proton_log_init\n");
    proton_log_init(log_file, LOG_INFO);
    fprintf(stderr, "[MAIN] Log init complete\n");
    
    proton_log(LOG_INFO, "Proton v%s starting (pid=%d)", PROTON_VERSION, proton_pid);
    fprintf(stderr, "[MAIN] proton_log called\n");
    
    /* Setup signal handlers */
    fprintf(stderr, "[MAIN] Setting up signals\n");
    setup_signals();
    fprintf(stderr, "[MAIN] Signals set up\n");

    /* Start master process */
    fprintf(stderr, "[MAIN] Starting master process\n");
    int ret = proton_master_process(config);
    fprintf(stderr, "[MAIN] Master process returned: %d\n", ret);

    /* Cleanup */
    proton_log(LOG_INFO, "Proton shutting down");
    proton_log_close();
    proton_config_destroy(config);

    return ret;
}

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>
#include "proton.h"
#include "module.h"

static pid_t *worker_pids = NULL;
static int num_workers = 0;
static proton_config_t *master_config = NULL;

static void spawn_worker(proton_config_t *config, int worker_id) {
    pid_t pid = fork();
    
    if (pid < 0) {
        proton_log(LOG_ERROR, "Failed to fork worker process");
        return;
    }
    
    if (pid == 0) {
        /* Child process - worker */
        proton_log(LOG_INFO, "Worker %d started (pid=%d)", worker_id, getpid());
        exit(proton_worker_process(config));
    }
    
    /* Parent process - master */
    worker_pids[worker_id] = pid;
    proton_log(LOG_INFO, "Spawned worker %d (pid=%d)", worker_id, pid);
}

static void spawn_workers(proton_config_t *config) {
    num_workers = config->worker_processes;
    
    if (num_workers <= 0) {
        num_workers = sysconf(_SC_NPROCESSORS_ONLN);
        if (num_workers <= 0) num_workers = 1;
    }
    
    worker_pids = calloc(num_workers, sizeof(pid_t));
    if (!worker_pids) {
        proton_log(LOG_ERROR, "Failed to allocate worker PID array");
        return;
    }
    
    for (int i = 0; i < num_workers; i++) {
        spawn_worker(config, i);
    }
}

static void stop_workers(void) {
    if (!worker_pids) return;
    
    /* Send SIGTERM to all workers */
    for (int i = 0; i < num_workers; i++) {
        if (worker_pids[i] > 0) {
            proton_log(LOG_INFO, "Stopping worker %d (pid=%d)", i, worker_pids[i]);
            kill(worker_pids[i], SIGTERM);
        }
    }
    
    /* Wait for workers to exit */
    for (int i = 0; i < num_workers; i++) {
        if (worker_pids[i] > 0) {
            int status;
            waitpid(worker_pids[i], &status, 0);
            proton_log(LOG_INFO, "Worker %d exited", i);
        }
    }
    
    free(worker_pids);
    worker_pids = NULL;
}

static void reap_children(void) {
    int status;
    pid_t pid;
    
    /* Reap any terminated child processes */
    while ((pid = waitpid(-1, &status, WNOHANG)) > 0) {
        /* Find which worker died */
        for (int i = 0; i < num_workers; i++) {
            if (worker_pids[i] == pid) {
                proton_log(LOG_WARN, "Worker %d (pid=%d) died, respawning", i, pid);
                spawn_worker(master_config, i);
                break;
            }
        }
    }
}

int proton_master_process(proton_config_t *config) {
    master_config = config;
    
    proton_log(LOG_INFO, "Master process started (pid=%d)", getpid());
    fprintf(stderr, "[MASTER] proton_quit = %d\n", proton_quit);
    
    /* Initialize modules */
    fprintf(stderr, "[MASTER] Initializing modules\n");
    if (proton_modules_init(config) != PROTON_OK) {
        proton_log(LOG_ERROR, "Failed to initialize modules");
        return 1;
    }
    fprintf(stderr, "[MASTER] Modules initialized\n");
    
    /* Spawn worker processes */
    fprintf(stderr, "[MASTER] Spawning workers\n");
    spawn_workers(config);
    fprintf(stderr, "[MASTER] Workers spawned\n");
    
    proton_log(LOG_INFO, "Proton is ready to handle connections on port %d", config->listen_port);
    fprintf(stderr, "[MASTER] Entering main loop, proton_quit = %d\n", proton_quit);
    
    /* Master process main loop */
    while (!proton_quit) {
        fprintf(stderr, "[MASTER] In loop iteration, sleeping\n");
        sleep(1);
        
        /* Check for reload signal */
        if (proton_reload) {
            proton_log(LOG_INFO, "Received reload signal");
            proton_reload = 0;
            
            /* In a real implementation, would:
             * 1. Parse new config
             * 2. Spawn new workers with new config
             * 3. Send SIGQUIT to old workers
             * 4. Wait for old workers to finish requests
             */
            proton_log(LOG_INFO, "Hot reload not yet implemented");
        }
        
        /* Reap any dead workers */
        reap_children();
    }
    
    proton_log(LOG_INFO, "Master process shutting down");
    
    /* Stop all workers */
    stop_workers();
    
    /* Cleanup modules */
    proton_modules_cleanup();
    
    return 0;
}

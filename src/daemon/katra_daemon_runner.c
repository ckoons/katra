/* © 2025 Casey Koons All rights reserved */

/*
 * katra_daemon_runner.c - Standalone daemon process for Katra
 *
 * This program runs the Katra daemon in the background, processing
 * CI memories to extract patterns, form associations, detect themes,
 * and generate insights.
 *
 * Usage:
 *   katra_daemon [--once] [--ci CI_ID] [--config PATH]
 *
 * Options:
 *   --once     Run one cycle and exit (default: continuous loop)
 *   --ci ID    Process only this CI (default: all CIs)
 *   --config   Path to config file (default: ~/.katra/daemon/daemon.conf)
 *   --help     Show this help
 *
 * The daemon respects quiet hours and won't run during active CI sessions.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <time.h>
#include <sys/stat.h>

#include "katra_daemon.h"
#include "katra_error.h"
#include "katra_log.h"
#include "katra_config.h"
#include "katra_path_utils.h"

/* Signal handling */
static volatile sig_atomic_t g_running = 1;
static volatile sig_atomic_t g_reload_config = 0;

/* Command line options */
typedef struct {
    bool run_once;
    char ci_id[KATRA_CI_ID_SIZE];
    char config_path[KATRA_PATH_MAX];
    bool verbose;
} runner_options_t;

/* Signal handlers */
static void handle_sigterm(int sig) {
    (void)sig;
    g_running = 0;
}

static void handle_sighup(int sig) {
    (void)sig;
    g_reload_config = 1;
}

/* Print usage */
static void print_usage(const char* prog) {
    printf("Katra Daemon Runner - Autonomous memory processing\n\n");
    printf("Usage: %s [OPTIONS]\n\n", prog);
    printf("Options:\n");
    printf("  --once        Run one processing cycle and exit\n");
    printf("  --ci ID       Process only specified CI (default: all CIs)\n");
    printf("  --config PATH Path to config file\n");
    printf("  --verbose     Enable verbose output\n");
    printf("  --help        Show this help\n\n");
    printf("The daemon runs continuously, processing CI memories during quiet periods.\n");
    printf("It extracts patterns, forms associations, detects themes, and generates\n");
    printf("insights that CIs discover on their next sunrise.\n\n");
    printf("Signals:\n");
    printf("  SIGTERM/SIGINT  Graceful shutdown\n");
    printf("  SIGHUP          Reload configuration\n");
}

/* Parse command line arguments */
static int parse_args(int argc, char** argv, runner_options_t* opts) {
    memset(opts, 0, sizeof(*opts));

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--once") == 0) {
            opts->run_once = true;
        } else if (strcmp(argv[i], "--verbose") == 0) {
            opts->verbose = true;
        } else if (strcmp(argv[i], "--help") == 0 || strcmp(argv[i], "-h") == 0) {
            print_usage(argv[0]);
            exit(0);
        } else if (strcmp(argv[i], "--ci") == 0) {
            if (i + 1 >= argc) {
                /* GUIDELINE_APPROVED: CLI arg error before logging init */
                fprintf(stderr, "Error: --ci requires an argument\n");
                return -1;
            }
            strncpy(opts->ci_id, argv[++i], sizeof(opts->ci_id) - 1);
        } else if (strcmp(argv[i], "--config") == 0) {
            if (i + 1 >= argc) {
                /* GUIDELINE_APPROVED: CLI arg error before logging init */
                fprintf(stderr, "Error: --config requires an argument\n");
                return -1;
            }
            strncpy(opts->config_path, argv[++i], sizeof(opts->config_path) - 1);
        } else {
            /* GUIDELINE_APPROVED: CLI arg error before logging init */
            fprintf(stderr, "Unknown option: %s\n", argv[i]);
            return -1;
        }
    }

    return 0;
}

/* Get list of CIs with memories */
static int get_ci_list(char*** ci_ids, size_t* count) {
    /* For now, use "default" CI. In the future, query all CIs from memory DB */
    *count = 1;
    *ci_ids = malloc(sizeof(char*));
    if (!*ci_ids) return E_SYSTEM_MEMORY;

    (*ci_ids)[0] = strdup("default");
    if (!(*ci_ids)[0]) {
        free(*ci_ids);
        return E_SYSTEM_MEMORY;
    }

    return KATRA_SUCCESS;
}

/* Free CI list */
static void free_ci_list(char** ci_ids, size_t count) {
    for (size_t i = 0; i < count; i++) {
        free(ci_ids[i]);
    }
    free(ci_ids);
}

/* Run one daemon cycle for all configured CIs */
static int run_daemon_cycle(const runner_options_t* opts, const daemon_config_t* config) {
    int result = KATRA_SUCCESS;
    daemon_result_t cycle_result;

    if (opts->ci_id[0] != '\0') {
        /* Process single CI */
        printf("[%ld] Processing CI: %s\n", (long)time(NULL), opts->ci_id);

        result = katra_daemon_run_cycle(opts->ci_id, config, &cycle_result);

        if (result == KATRA_SUCCESS) {
            printf("[%ld] Cycle complete: patterns=%zu, associations=%zu, "
                   "themes=%zu, insights=%zu\n",
                   (long)time(NULL),
                   cycle_result.patterns_found,
                   cycle_result.associations_formed,
                   cycle_result.themes_detected,
                   cycle_result.insights_generated);
        } else {
            printf("[%ld] Cycle error: %s\n", (long)time(NULL), katra_error_message(result));
        }
    } else {
        /* Process all CIs */
        char** ci_ids = NULL;
        size_t ci_count = 0;

        result = get_ci_list(&ci_ids, &ci_count);
        if (result != KATRA_SUCCESS) {
            return result;
        }

        for (size_t i = 0; i < ci_count && g_running; i++) {
            printf("[%ld] Processing CI: %s\n", (long)time(NULL), ci_ids[i]);

            int rc = katra_daemon_run_cycle(ci_ids[i], config, &cycle_result);

            if (rc == KATRA_SUCCESS) {
                printf("[%ld] CI %s: patterns=%zu, associations=%zu, "
                       "themes=%zu, insights=%zu\n",
                       (long)time(NULL), ci_ids[i],
                       cycle_result.patterns_found,
                       cycle_result.associations_formed,
                       cycle_result.themes_detected,
                       cycle_result.insights_generated);
            } else {
                printf("[%ld] CI %s error: %s\n",
                       (long)time(NULL), ci_ids[i], katra_error_message(rc));
            }
        }

        free_ci_list(ci_ids, ci_count);
    }

    return result;
}

/* Main daemon loop */
static int daemon_main_loop(const runner_options_t* opts) {
    daemon_config_t config;
    int result;

    /* Load initial config */
    katra_daemon_default_config(&config);
    result = katra_daemon_load_config(&config);
    if (result != KATRA_SUCCESS && result != E_NOT_FOUND) {
        /* GUIDELINE_APPROVED: Non-fatal startup warning, logging may not be ready */
        fprintf(stderr, "Warning: Failed to load config, using defaults\n");
    }

    printf("Katra Daemon started\n");
    printf("  Interval: %d minutes\n", config.interval_minutes);
    printf("  Quiet hours: %02d:00 - %02d:00\n",
           config.quiet_hours_start, config.quiet_hours_end);
    printf("  Pattern extraction: %s\n", config.pattern_extraction ? "enabled" : "disabled");
    printf("  Association formation: %s\n", config.association_formation ? "enabled" : "disabled");
    printf("  Theme detection: %s\n", config.theme_detection ? "enabled" : "disabled");
    printf("  Insight generation: %s\n", config.insight_generation ? "enabled" : "disabled");

    if (opts->run_once) {
        /* Single run mode */
        printf("\nRunning single cycle...\n");
        return run_daemon_cycle(opts, &config);
    }

    /* Continuous loop mode */
    printf("\nEntering main loop (Ctrl+C to stop)...\n\n");

    time_t last_run = 0;

    while (g_running) {
        /* Check for config reload request */
        if (g_reload_config) {
            printf("[%ld] Reloading configuration...\n", (long)time(NULL));
            katra_daemon_load_config(&config);
            g_reload_config = 0;
        }

        time_t now = time(NULL);

        /* Check if we should run */
        if (katra_daemon_should_run(&config)) {
            /* Check if enough time has passed since last run */
            int interval_seconds = config.interval_minutes * 60;

            if (now - last_run >= interval_seconds) {
                run_daemon_cycle(opts, &config);
                last_run = now;
            }
        }

        /* Sleep for a bit before checking again */
        /* Check every minute for better responsiveness to signals */
        for (int i = 0; i < 60 && g_running; i++) {
            sleep(1);
        }
    }

    printf("\nDaemon shutdown complete\n");
    return KATRA_SUCCESS;
}

/* Main entry point */
int main(int argc, char** argv) {
    runner_options_t opts;
    int result;

    /* Parse command line */
    if (parse_args(argc, argv, &opts) != 0) {
        return 1;
    }

    /* Set up signal handlers */
    signal(SIGTERM, handle_sigterm);
    signal(SIGINT, handle_sigterm);
    signal(SIGHUP, handle_sighup);

    /* Initialize Katra subsystems */
    result = katra_daemon_init();
    if (result != KATRA_SUCCESS) {
        /* GUIDELINE_APPROVED: Critical startup error before logging available */
        fprintf(stderr, "Failed to initialize daemon: %s\n", katra_error_message(result));
        return 1;
    }

    /* Run daemon */
    result = daemon_main_loop(&opts);

    /* Cleanup */
    katra_daemon_cleanup();

    return (result == KATRA_SUCCESS) ? 0 : 1;
}

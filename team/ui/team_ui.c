/* © 2025 Casey Koons All rights reserved */

/* team_ui.c - Terminal coordinator for the katra CI team.
 *             Fans Casey's input to the team over the meeting room and
 *             collects their replies. MVP: broadcast, direct message,
 *             hear, who, status. */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "team_ui.h"
#include "katra_mcp_tcp.h"   /* KATRA_MCP_DEFAULT_PORT */
#include "katra_error.h"

/* Coordinator runtime configuration. */
typedef struct {
    const char* host;
    int port;
    const char* persona;
    const char* role;
} team_ui_config_t;

/* Run one MCP call and print its human-readable text. Consumes args. */
static void team_ui_run(const team_ui_config_t* cfg, const char* tool, json_t* args) {
    char* text = NULL;
    bool is_error = false;

    int rc = team_ui_mcp_call(cfg->host, cfg->port, tool, args, &text, &is_error);
    if (rc != KATRA_SUCCESS) {
        printf("  [call failed: %s]\n", katra_error_message(rc));
        return;
    }
    if (text && text[0] != '\0') {
        printf("%s%s\n", is_error ? "  [error] " : "", text);
    }
    free(text);
}

/* Register the coordinator persona in the meeting room. */
static void team_ui_register(const team_ui_config_t* cfg) {
    json_t* args = json_object();
    if (!args) {
        return;
    }
    json_object_set_new(args, MCP_PARAM_NAME, json_string(cfg->persona));
    json_object_set_new(args, MCP_PARAM_ROLE, json_string(cfg->role));
    team_ui_run(cfg, MCP_TOOL_REGISTER, args);
}

/* Send a message. recipients == NULL/"" broadcasts to the whole team. */
static void team_ui_say(const team_ui_config_t* cfg, const char* message,
                        const char* recipients) {
    json_t* args = json_object();
    if (!args) {
        return;
    }
    json_object_set_new(args, MCP_PARAM_MESSAGE, json_string(message));
    json_object_set_new(args, TEAM_UI_PARAM_CI_NAME, json_string(cfg->persona));
    if (recipients && recipients[0] != '\0') {
        json_object_set_new(args, TEAM_UI_PARAM_RECIPIENTS, json_string(recipients));
    }
    team_ui_run(cfg, MCP_TOOL_SAY, args);
}

/* Drain and print all messages waiting in the coordinator's queue. */
static void team_ui_hear(const team_ui_config_t* cfg) {
    json_t* args = json_object();
    if (!args) {
        return;
    }
    json_object_set_new(args, TEAM_UI_PARAM_CI_NAME, json_string(cfg->persona));
    json_object_set_new(args, TEAM_UI_PARAM_MAX_COUNT, json_integer(0));
    team_ui_run(cfg, MCP_TOOL_HEAR_ALL, args);
}

/* List active CIs in the meeting room (no arguments). */
static void team_ui_who(const team_ui_config_t* cfg) {
    team_ui_run(cfg, MCP_TOOL_WHO_IS_HERE, NULL);
}

/* Report system status for the coordinator persona. */
static void team_ui_status(const team_ui_config_t* cfg) {
    json_t* args = json_object();
    if (!args) {
        return;
    }
    json_object_set_new(args, TEAM_UI_PARAM_CI_NAME, json_string(cfg->persona));
    team_ui_run(cfg, MCP_TOOL_STATUS, args);
}

static void team_ui_help(void) {
    printf("\nCommands:\n");
    printf("  <text>             broadcast to the whole team\n");
    printf("  %s <ci> <text>    send only to <ci> (e.g. Lyra, Grace)\n", TEAM_UI_CMD_TO);
    printf("  %s               collect replies waiting for you\n", TEAM_UI_CMD_HEAR);
    printf("  %s                list CIs currently in the room\n", TEAM_UI_CMD_WHO);
    printf("  %s             system status\n", TEAM_UI_CMD_STATUS);
    printf("  %s / %s        leave the coordinator\n", TEAM_UI_CMD_QUIT, TEAM_UI_CMD_EXIT);
    printf("  %s               this help\n\n", TEAM_UI_CMD_HELP);
}

/* Dispatch a slash-command line. Returns false to stop the REPL. */
static bool team_ui_command(const team_ui_config_t* cfg, char* line) {
    if (strcmp(line, TEAM_UI_CMD_QUIT) == 0 || strcmp(line, TEAM_UI_CMD_EXIT) == 0) {
        return false;
    }
    if (strcmp(line, TEAM_UI_CMD_HELP) == 0) {
        team_ui_help();
        return true;
    }
    if (strcmp(line, TEAM_UI_CMD_WHO) == 0) {
        team_ui_who(cfg);
        return true;
    }
    if (strcmp(line, TEAM_UI_CMD_HEAR) == 0) {
        team_ui_hear(cfg);
        return true;
    }
    if (strcmp(line, TEAM_UI_CMD_STATUS) == 0) {
        team_ui_status(cfg);
        return true;
    }

    /* /to <ci> <message> */
    size_t to_len = strlen(TEAM_UI_CMD_TO);
    if (strncmp(line, TEAM_UI_CMD_TO, to_len) == 0 &&
        (line[to_len] == ' ' || line[to_len] == '\0')) {
        char* recipient = line + to_len;
        while (*recipient == ' ') {
            recipient++;
        }
        char* message = strchr(recipient, ' ');
        if (*recipient == '\0' || !message) {
            printf("  usage: %s <ci> <message>\n", TEAM_UI_CMD_TO);
            return true;
        }
        *message = '\0';
        message++;
        while (*message == ' ') {
            message++;
        }
        if (*message == '\0') {
            printf("  usage: %s <ci> <message>\n", TEAM_UI_CMD_TO);
            return true;
        }
        team_ui_say(cfg, message, recipient);
        return true;
    }

    printf("  unknown command: %s (try %s)\n", line, TEAM_UI_CMD_HELP);
    return true;
}

static void team_ui_usage(const char* prog) {
    printf("usage: %s [%s <name>] [%s <addr>] [%s <n>] [%s <role>]\n",
           prog, TEAM_UI_FLAG_KATRA, TEAM_UI_FLAG_HOST,
           TEAM_UI_FLAG_PORT, TEAM_UI_FLAG_ROLE);
    printf("  coordinator terminal for the katra CI team meeting room\n");
}

/* Parse CLI flags into cfg. Returns 0 to run, 1 for --help, -1 on error. */
static int team_ui_parse_args(int argc, char** argv, team_ui_config_t* cfg) {
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], TEAM_UI_FLAG_HELP) == 0) {
            return 1;
        } else if (strcmp(argv[i], TEAM_UI_FLAG_KATRA) == 0 && i + 1 < argc) {
            cfg->persona = argv[++i];
        } else if (strcmp(argv[i], TEAM_UI_FLAG_HOST) == 0 && i + 1 < argc) {
            cfg->host = argv[++i];
        } else if (strcmp(argv[i], TEAM_UI_FLAG_PORT) == 0 && i + 1 < argc) {
            cfg->port = atoi(argv[++i]);
        } else if (strcmp(argv[i], TEAM_UI_FLAG_ROLE) == 0 && i + 1 < argc) {
            cfg->role = argv[++i];
        } else {
            printf("unknown or incomplete option: %s\n", argv[i]);
            return -1;
        }
    }
    return 0;
}

int main(int argc, char** argv) {
    team_ui_config_t cfg = {
        .host = TEAM_UI_DEFAULT_HOST,
        .port = KATRA_MCP_DEFAULT_PORT,
        .persona = TEAM_UI_DEFAULT_PERSONA,
        .role = TEAM_UI_DEFAULT_ROLE
    };
    char line[TEAM_UI_INPUT_MAX];
    bool running = true;

    int parsed = team_ui_parse_args(argc, argv, &cfg);
    if (parsed != 0) {
        team_ui_usage(argv[0]);
        return (parsed < 0) ? 1 : 0;
    }

    printf("katra team coordinator - %s@%s:%d as \"%s\"\n",
           cfg.role, cfg.host, cfg.port, cfg.persona);
    team_ui_register(&cfg);
    printf("Type a message to broadcast, %s for commands, %s to leave.\n",
           TEAM_UI_CMD_HELP, TEAM_UI_CMD_QUIT);

    while (running) {
        printf("%s> ", cfg.persona);
        fflush(stdout);

        if (!fgets(line, sizeof(line), stdin)) {
            printf("\n");
            break;  /* EOF (Ctrl-D) */
        }
        line[strcspn(line, "\n")] = '\0';

        char* p = line;
        while (*p == ' ' || *p == '\t') {
            p++;
        }
        if (*p == '\0') {
            continue;
        }

        if (*p == TEAM_UI_CMD_PREFIX) {
            running = team_ui_command(&cfg, p);
        } else {
            team_ui_say(&cfg, p, NULL);
        }
    }

    printf("coordinator closed.\n");
    return 0;
}

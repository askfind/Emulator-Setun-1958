/**
 * Filename: "command.h"
 *
 * Command interpreter for setun1958emu (C port from C++ Command class)
 */

#ifndef COMMAND_H
#define COMMAND_H

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

#define CMD_MAX_CMDS        32
#define CMD_HISTORY_SIZE    10
#define CMD_INPUT_BUF_SIZE  80
#define CMD_MAX_ARGS        8

typedef void (*cmd_handler_t)(int argc, char *argv[]);

typedef struct {
    const char *name;
    const char *alias;
    cmd_handler_t handler;
} cmd_entry_t;

typedef struct {
    cmd_entry_t entries[CMD_MAX_CMDS];
    int cmd_count;
    char history[CMD_HISTORY_SIZE][CMD_INPUT_BUF_SIZE];
    int history_count;
    int history_index;
    char last_command[CMD_INPUT_BUF_SIZE];
    int max_args;
} command_t;

/* Init */
void command_init(command_t *cmd, int max_args);

/* Add command */
void command_add(command_t *cmd, const char *name, const char *alias, cmd_handler_t handler);

/* Parse line into argc/argv (modifies line in place) */
int command_parse_line(char *line, char *argv[], int max_args);

/* Execute command line. Returns 1 if found and executed, 0 otherwise. */
int command_execute(command_t *cmd, const char *line);

/* History */
void command_save_history(command_t *cmd, const char *cmd_line);
int command_recall_up(command_t *cmd, char *buf, size_t buf_size);
int command_recall_down(command_t *cmd, char *buf, size_t buf_size);
void command_reset_history_index(command_t *cmd);
void command_clear_history(command_t *cmd);

const char *command_last_command(const command_t *cmd);

#ifdef __cplusplus
}
#endif

#endif

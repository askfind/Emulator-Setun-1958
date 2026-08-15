/**
 * Filename: "command.c"
 *
 * Command interpreter implementation (C port from C++ Command class)
 */

#include "command.h"
#include <string.h>
#include <strings.h>  /* strcasecmp */

void command_init(command_t *cmd, int max_args)
{
    cmd->cmd_count = 0;
    cmd->history_count = 0;
    cmd->history_index = -1;
    cmd->max_args = max_args;
    cmd->last_command[0] = '\0';

    for (int i = 0; i < CMD_HISTORY_SIZE; i++) {
        cmd->history[i][0] = '\0';
    }
    for (int i = 0; i < CMD_MAX_CMDS; i++) {
        cmd->entries[i].name = NULL;
        cmd->entries[i].alias = NULL;
        cmd->entries[i].handler = NULL;
    }
}

void command_add(command_t *cmd, const char *name, const char *alias, cmd_handler_t handler)
{
    if (cmd->cmd_count >= CMD_MAX_CMDS || !name || !handler)
        return;
    cmd->entries[cmd->cmd_count].name = name;
    cmd->entries[cmd->cmd_count].alias = alias;
    cmd->entries[cmd->cmd_count].handler = handler;
    cmd->cmd_count++;
}

int command_parse_line(char *line, char *argv[], int max_args)
{
    int argc = 0;
    char *p = line;
    while (*p && argc < max_args) {
        while (*p == ' ' || *p == '\t')
            p++;
        if (!*p)
            break;
        argv[argc++] = p;
        while (*p && *p != ' ' && *p != '\t')
            p++;
        if (*p) {
            *p = '\0';
            p++;
        }
    }
    return argc;
}

int command_execute(command_t *cmd, const char *line)
{
    if (!line || line[0] == '\0')
        return 0;

    char buf[CMD_INPUT_BUF_SIZE];
    strncpy(buf, line, sizeof(buf) - 1);
    buf[sizeof(buf) - 1] = '\0';

    char *argv[CMD_MAX_ARGS];
    int argc = command_parse_line(buf, argv, cmd->max_args);
    if (argc == 0)
        return 0;

    strncpy(cmd->last_command, line, sizeof(cmd->last_command) - 1);
    cmd->last_command[sizeof(cmd->last_command) - 1] = '\0';

    for (int i = 0; i < cmd->cmd_count; i++) {
        if (strcasecmp(argv[0], cmd->entries[i].name) == 0 ||
            (cmd->entries[i].alias && strcasecmp(argv[0], cmd->entries[i].alias) == 0)) {
            cmd->entries[i].handler(argc, argv);
            return 1;
        }
    }
    return 0;
}

void command_save_history(command_t *cmd, const char *cmd_line)
{
    if (!cmd_line || cmd_line[0] == '\0')
        return;
    for (int i = CMD_HISTORY_SIZE - 1; i > 0; i--) {
        strcpy(cmd->history[i], cmd->history[i - 1]);
    }
    strncpy(cmd->history[0], cmd_line, CMD_INPUT_BUF_SIZE - 1);
    cmd->history[0][CMD_INPUT_BUF_SIZE - 1] = '\0';
    if (cmd->history_count < CMD_HISTORY_SIZE)
        cmd->history_count++;
    cmd->history_index = -1;
}

int command_recall_up(command_t *cmd, char *buf, size_t buf_size)
{
    if (cmd->history_index + 1 >= cmd->history_count || buf_size == 0)
        return 0;
    cmd->history_index++;
    strncpy(buf, cmd->history[cmd->history_index], buf_size - 1);
    buf[buf_size - 1] = '\0';
    return 1;
}

int command_recall_down(command_t *cmd, char *buf, size_t buf_size)
{
    if (cmd->history_index <= 0 || buf_size == 0) {
        cmd->history_index = -1;
        buf[0] = '\0';
        return 0;
    }
    cmd->history_index--;
    strncpy(buf, cmd->history[cmd->history_index], buf_size - 1);
    buf[buf_size - 1] = '\0';
    return 1;
}

void command_reset_history_index(command_t *cmd)
{
    cmd->history_index = -1;
}

void command_clear_history(command_t *cmd)
{
    cmd->history_count = 0;
    cmd->history_index = -1;
    for (int i = 0; i < CMD_HISTORY_SIZE; i++) {
        cmd->history[i][0] = '\0';
    }
}

const char *command_last_command(const command_t *cmd)
{
    return cmd->last_command;
}

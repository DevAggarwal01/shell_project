/*
  utcsh - The UTCS Shell

  <Put your name and CS login ID here>
  Dev Aggarwal - dev4dev
  Hrutvik Rao Palutla Venkata - hrutvikp
*/

/* Read the additional functions from util.h. They may be beneficial to you
in the future */
#include "util.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include <fcntl.h> 
#include <wait.h>


/* Global variables */
/* The array for holding shell paths. Can be edited by the functions in util.c*/
char shell_paths[MAX_ENTRIES_IN_SHELLPATH][MAX_CHARS_PER_CMDLINE];
static char prompt[] = "utcsh> "; /* Command line prompt */
static char *default_shell_path[2] = {"/bin", NULL};
static bool isScript = false;
pid_t child_pids[MAX_WORDS_PER_CMDLINE]; // to hold pids of children launched by eval
int nchildren = 0; // number of children launched by eval
/* End Global Variables */


/* Convenience struct for describing a command. Modify this struct as you see
 * fit--add extra members to help you write your code. */
struct Command {
    char **args;      /* Argument array for the command */
    char *program;
    char *outputFile; /* Redirect target for file (NULL means no redirect) */
};


/* MAIN FUNCTIONS*/
char **tokenize_command_line (char *cmdline);
struct Command *split_into_commands(char **tokens);
struct Command parse_command (char **tokens);
bool eval (struct Command *cmd);
int try_exec_builtin (struct Command *cmd);
void exec_external_cmd (struct Command *cmd);

/*HELPER FUNCTIONS*/
int count_args(char **args);
void print_errors();
void remove_tabs(char *str);


/* MAIN METHOD */

/* Main REPL: read, evaluate, and print. This function should remain relatively
   short: if it grows beyond 60 lines, you're doing too much in main() and
   should try to move some of that work into other functions. */
int main (int argc, char **argv) {
    // two arguments: command, script file [more is error]
    if (argc == 2) {
        isScript = true;
    } if (argc > 2) {
        print_errors();
        exit(1);
    }
    // input: either script file or standard input
    FILE *input = isScript ? fopen(argv[1], "r") : stdin;
    if (isScript && input == NULL) {
        // failed to open file
        print_errors();
        exit(1);
    }
    if (isScript) {
        // checks if script is empty by checking if first character is EOF
        int c = fgetc(input);
        if (c == EOF) {
            print_errors();
            fclose(input);
            exit(1);
        }
        // put character back for future reading
        c = ungetc(c, input);
    }

    // initialize shell path to default path (may be changed later by path command)
    set_shell_path(default_shell_path);
    
    // REPl (read-evaluate-print loop)
    while (true) {
        // print shell prompt if no script
        if (!isScript) {
            printf("%s", prompt);
        }

        // read part of REPL
        // get a line of input
        __ssize_t lineSize;
        size_t len = 0;
        char* line = NULL;
        lineSize = getline(&line, &len, input);
        if(lineSize == -1) {
            free(line);
            break;
        }
        // strip newline character if present
        if (lineSize > 0 && line[lineSize - 1] == '\n') {
            line[lineSize - 1] = '\0';
            lineSize--;
        }
        // guards against empty lines
        remove_tabs(line);
        if (*line == '\0') {
            free(line);
            continue;
        }

        // evaluate part of REPL
        // tokenize command and split it into multiple commands if necessary
        char** tokens = tokenize_command_line(line);
        if (tokens == NULL) {
            free(line);
            continue;
        }
        struct Command *commands = split_into_commands(tokens);
        if (commands == NULL) {
            free(line);
            free(tokens);
            continue;
        }
        // evaluate all commands on the line (concurrently if multiple)
        bool exitCmd = false;
        for (int i = 0; commands[i].program != NULL; i++) {
            exitCmd = eval(&commands[i]) ? true : exitCmd;
        }
        // wait for all external children launched by eval
        for (int i = 0; i < nchildren; i++) {
            int status;
            waitpid(child_pids[i], &status, 0);
        }
        nchildren = 0;

        // print part of REPL: nothing to print for this shell

        // free allocated memory
        if (line != NULL) {
            free(line); // tokens arrays has pointers into line (and cmds also point to tokens), so free line first
        }
        if (tokens != NULL) {
            free(tokens); // free tokens array (not the strings inside, those are already freed)
        }
        if (commands != NULL) {
            free(commands); // free commands array (not the strings inside, those are already freed)
        }
        if (exitCmd) {
            // exit with appropriate code if we need to exit early due to errors, closing input first
            fclose(input);
            exit(0);
        }
    }
    // close the input
    fclose(input);
    return 0;
}


/* HELPER FUNCTIONS */

/**
 * Prints the standard error message to stderr.
 */
void print_errors() {
    // Attribution (slighly modified here): Section 2.4, https://www.cs.utexas.edu/~ans/classes/cs439/projects/shell_project/shell.html
    char emsg[30] = "An error has occurred\n";
    size_t nbytes_written = write(STDERR_FILENO, emsg, strlen(emsg));
    if (nbytes_written != strlen(emsg)) {
        exit(2);
    }
}


/**
 * Removes all tab characters from a string in place.
 */
void remove_tabs(char *s) {
    // in place removal of tabs
    char* src = s;
    char* dst = s;
    // copy over non-tab characters
    while (*src != '\0') {
        if (*src != '\t') {
            *dst = *src;
            dst += 1;
        }
        src += 1;
    }
    // null terminate the new string
    *dst = '\0';
}


/** 
 * Count the number of arguments in a command.
 */
int count_args(char **args) {
    int count = 0;
    while (args[count] != NULL) {
        count++;
    }
    return count;
}


/* PRIMARY FUNCTIONS */

/** 
 * Turns a command line into tokens with strtok.
 *
 * This function turns a command line into an array of arguments, making it
 * much easier to process. First, you should figure out how many arguments you
 * have, then allocate a char** of sufficient size and fill it using strtok()
 */
char** tokenize_command_line(char *cmdline) {
    // allocate array of tokens
    char** tokens = malloc(sizeof(char*) * MAX_WORDS_PER_CMDLINE);
    if (tokens == NULL) {
        // if allocation fails, return NULL
        return NULL;
    }
    // tokenize command line by spaces, but also separate out & if present
    char* saveptr;
    char* token = strtok_r(cmdline, " ", &saveptr);
    int i = 0;
    while (token && i < MAX_WORDS_PER_CMDLINE) {
        char* p = token;
        // logic to separate out & operators. each & is its own token
        while (*p) {
            if (*p == '&') {
                // terminate before &
                if (p != token) {
                    *p = '\0';
                    tokens[i] = token;
                    i += 1;
                }
                // add "&"
                tokens[i] = "&";
                token = p + 1;
                i += 1;
            }
            p += 1;
        }
        // add the final token if not empty
        if (*token != '\0') {
            tokens[i] = token;
            i += 1;
        }
        // get next token
        token = strtok_r(NULL, " ", &saveptr);
    }
    // null terminate and return the array
    tokens[i] = NULL;
    return tokens;
}


/**
 * Splits tokens into multiple commands separated by "&".
 *
 * Returns an array of these commands, terminated by a command with all fields NULL.
 */
struct Command *split_into_commands(char **tokens) {
    // allocate array of commands
    struct Command* cmds = malloc(sizeof(struct Command) * MAX_CMDS_PER_CMDLINE);
    if (cmds == NULL) {
        // if allocation fails, return NULL
        return NULL;
    }
    int cmd_count = 0;
    int start = 0;
    // loop through tokens to find & and split commands
    for (int i = 0; tokens[i] != NULL; i++) {
        if (strcmp(tokens[i], "&") == 0) {
            // terminate this command’s args
            tokens[i] = NULL;
            cmds[cmd_count] = parse_command(&tokens[start]);
            cmd_count++;
            // next command starts after &
            start = i + 1;
        }
    }
    // to account for final command (after last & or whole line if no &)
    if (tokens[start] != NULL) {
        cmds[cmd_count] = parse_command(&tokens[start]);
        cmd_count++;
    }
    // mark end of array
    cmds[cmd_count].program = NULL;
    cmds[cmd_count].args = NULL;
    cmds[cmd_count].outputFile = NULL;
    return cmds;
}


/** 
 * Turns a sequence of tokens into a single command.
 *
 * The `struct Command` represents a command to execute. This is the preferred
 * format for storing information about a command, though you are free to change
 * it. This function takes a sequence of tokens and turns them into a struct
 * Command.
 */
struct Command parse_command(char **tokens) {
    // initialize command struct with nulls
    struct Command cmd;
    cmd.args = NULL;
    cmd.outputFile = NULL;
    cmd.program = NULL;
    // case for empty command
    if (tokens == NULL || tokens[0] == NULL) {
        return cmd;
    }
    // index in tokens of the redirect operatior >, -1 if not found
    int redirectIndex = -1;
    // look for > in tokens
    for (int i = 0; tokens[i] != NULL; i++) {
        if (strcmp(tokens[i], ">") == 0) {
            if (redirectIndex != -1) {
                // more than one > found
                print_errors();
                return cmd;
            }
            // found >, store index
            redirectIndex = i;
        }
    }
    if (redirectIndex != -1) {
        // > cannot be the first token, print errors if so
        if (redirectIndex == 0) {
            print_errors();
            return cmd;
        }
        // there must be exactly one token after >, print errors if not
        if (tokens[redirectIndex + 1] == NULL || tokens[redirectIndex + 2] != NULL) {
            print_errors();
            return cmd;
        }
        // set outputFile to the token after >
        cmd.outputFile = tokens[redirectIndex + 1];
        tokens[redirectIndex] = NULL;  // truncate args at >
    }
    // set args and program
    cmd.args = tokens;
    cmd.program = tokens[0];
    return cmd;
}

/** 
 * Evaluates a single command.
 *
 * Both built-ins and external commands can be passed to this function--it
 * should work out what the correct type is and take the appropriate action.
 * Returns false if no exit is needed, true if exit is needed.
 */
bool eval(struct Command *cmd) {
    // null check
    if (cmd == NULL || cmd->program == NULL) {
        return false;
    }
    // if it was a built-in command and we've executed it, can return
    int builtinStatus = try_exec_builtin(cmd);
    if (builtinStatus == 1) {
        return false; 
    } else if (builtinStatus == 2) {
        return true; // indicate exit
    }
    // we have an external command
    // check if absolute path
    if(!is_absolute_path(cmd->program)) {
        bool found = false;
        // if not absolute, loop through shell paths and find the right shell path
        for(int i = 0; i < MAX_ENTRIES_IN_SHELLPATH; i++) {
            if (shell_paths[i][0] == '\0') {
                break;
            }
            char* dir = shell_paths[i];
            char* fullPath = exe_exists_in_dir(dir, cmd->program, false);
            // alter cmd->program to be the full path if found
            if (fullPath != NULL) {
                cmd->program = fullPath;
                cmd->args[0] = fullPath;
                found = true;
                break;
            }
        }
        // if not external not found, print errors and return
        if (!found) {
            print_errors();
            return false;
        }
    }
    // execute the external command
    exec_external_cmd(cmd);
    return false;
}


/** 
 * Executes built-in commands (exit, cd, path)
 * - If the command is "exit", and it is valid, return 2.
 * - If the command is a built-in command, execute it and return 1 (regardless of whether error happens or not).
 * - If the command is not a built-in command, do nothing and return 0.
 */
int try_exec_builtin (struct Command *cmd) {
    // cases for built-in commands
    if (strcmp(cmd->program, "exit") == 0) {
        // case for exit: make sure no extra args and exit process if command valid
        if (count_args(cmd->args) != 1) {
            print_errors();
            return 1;
        } else {
            return 2; // indicate exit
        }
    } else if (strcmp(cmd->program, "cd") == 0) {
        // case for cd: make sure exactly one arg (if not, error) and chdir to that directory (if fails, error)
        if (count_args(cmd->args) != 2 || chdir(cmd->args[1]) == -1) {
            print_errors();
        }
        return 1;
    } else if (strcmp(cmd->program, "path") == 0) {
        // case for path: set shell path to args after "path" (if fails, error)
        if(set_shell_path(&cmd->args[1]) == 0) {
            print_errors();
        }
        return 1;
    }
    // not a built-in command
    return 0;
}


/** 
 * Executes an external command.
 *
 * Execute an external command by fork-and-exec. Should also take care of
 * output redirection, if any is requested
 */
void exec_external_cmd(struct Command *cmd) {
    int pid = fork();
    if (pid < 0) {
        // fork failed
        print_errors();
    } else if (pid == 0) {
        // in child process: execute the command
        // if redirection requested
        if (cmd->outputFile != NULL) {
            // create/truncate output file, permissions rw-rw-rw-
            int fileDescriptor = open(cmd->outputFile, O_CREAT | O_WRONLY | O_TRUNC, 0666);
            if (fileDescriptor < 0) {
                print_errors();
            }
            // duplicate file descriptor onto stdout and stderr, prnt errors if fails
            if (dup2(fileDescriptor, STDOUT_FILENO) < 0 || dup2(fileDescriptor, STDERR_FILENO) < 0) {
                print_errors();
                close(fileDescriptor);
            }
            close(fileDescriptor);
        }
        // now execute the external command
        if (execv(cmd->program, cmd->args) == -1) {
            print_errors();
            exit(1);
        }
    } else { 
        // in parent process: store child pid to wait for later
        if (nchildren < MAX_WORDS_PER_CMDLINE) {
            child_pids[nchildren] = pid;
            nchildren += 1;
        } else {
            print_errors();
        }
    }
}

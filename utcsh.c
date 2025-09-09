/*
  utcsh - The UTCS Shell

  <Put your name and CS login ID here>
  Dev Aggarwal - dev4dev
*/

/* Read the additional functions from util.h. They may be beneficial to you
in the future */
#include "util.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>


/* Global variables */
/* The array for holding shell paths. Can be edited by the functions in util.c*/
char shell_paths[MAX_ENTRIES_IN_SHELLPATH][MAX_CHARS_PER_CMDLINE];
static char prompt[] = "utcsh> "; /* Command line prompt */
static char *default_shell_path[2] = {"/bin", NULL};
static bool isScript = false;
/* End Global Variables */

/* Convenience struct for describing a command. Modify this struct as you see
 * fit--add extra members to help you write your code. */
struct Command
{
  char **args;      /* Argument array for the command */
  char *program;
  char *outputFile; /* Redirect target for file (NULL means no redirect) */
};

/* Here are the functions we recommend you implement */

char **tokenize_command_line (char *cmdline);
struct Command parse_command (char **tokens);
void eval (struct Command *cmd);
int try_exec_builtin (struct Command *cmd);
void exec_external_cmd (struct Command *cmd);

int count_args(char **args);
void print_errors();
void remove_tabs(char *str);

/* Main REPL: read, evaluate, and print. This function should remain relatively
   short: if it grows beyond 60 lines, you're doing too much in main() and
   should try to move some of that work into other functions. */
int main (int argc, char **argv)
{
  if(argc == 2) { // command is 1 argument, script file is 2nd argument
    isScript = true;
  }
  FILE *input = isScript ? fopen(argv[1], "r") : stdin;
  // checks if script is empty
  if (isScript) {
    int c = fgetc(input);
    if (c == EOF) {
        // file is empty
        print_errors();
        exit(1);
    }
    ungetc(c, input);  // put the char back so reading starts from beginning
  }
  if(count_args(argv) > 2 || (isScript && input == NULL)) {
    print_errors();
    exit(1);
  }
  set_shell_path (default_shell_path);

  /* These two lines are just here to suppress certain warnings. You should
   * delete them when you implement Part 1.4 */
  (void) argc;
  (void) argv;

  __ssize_t lineSize;
  size_t len = 0;

  while (1)
    {
      if(!isScript) printf("%s", prompt);

      /* Read */
      // TODO line needs to be freed at the end
      char *line = NULL;

      lineSize = getline(&line, &len, input);
      if(lineSize == -1) {
        free(line);
        exit(0);
      }

      if (lineSize > 0 && line[lineSize - 1] == '\n') {
          line[lineSize - 1] = '\0';
          lineSize--;
      }
      remove_tabs(line);
      if(*line == '\0') {
        free(line);
        continue;
      }
      // TODO could be syntax errors in input?
      char **tokens = tokenize_command_line(line);
      struct Command cmd = parse_command(tokens);
      // guards against whitespace or empty commands
      /* Evaluate */
      eval(&cmd);
      /* Print (optional) */
    }
  return 0;
}

void remove_tabs(char *str) {
    char *src = str;
    char *dst = str;
    while (*src != '\0') {
        if (*src != '\t') {
            *dst = *src;
            dst += 1;
        }
        src += 1;
    }
    *dst = '\0';
}

/* NOTE: In the skeleton code, all function bodies below this line are dummy
implementations made to avoid warnings. You should delete them and replace them
with your own implementation. */

/** Turn a command line into tokens with strtok
 *
 * This function turns a command line into an array of arguments, making it
 * much easier to process. First, you should figure out how many arguments you
 * have, then allocate a char** of sufficient size and fill it using strtok()
 */
char **tokenize_command_line (char *cmdline)
{
  char **tokens = malloc(sizeof(char*) * MAX_WORDS_PER_CMDLINE);
  char *saveptr;
  char *token = strtok_r(cmdline, " ", &saveptr);
  int i = 0;
  while (token != NULL) {
    tokens[i] = token;
    i++;
    token = strtok_r(NULL, " ", &saveptr);
  }
  tokens[i] = '\0';
  return tokens;
}

/** Turn tokens into a command.
 *
 * The `struct Command` represents a command to execute. This is the preferred
 * format for storing information about a command, though you are free to change
 * it. This function takes a sequence of tokens and turns them into a struct
 * Command.
 */
struct Command parse_command (char **tokens)
{
  struct Command dummy = {.args = tokens, .outputFile = NULL, .program = tokens[0]};
  return dummy;
}

/** Evaluate a single command
 *
 * Both built-ins and external commands can be passed to this function--it
 * should work out what the correct type is and take the appropriate action.
 */
void eval (struct Command *cmd)
{
  if(cmd == NULL || cmd->program == NULL) {
    return;
  }
  if(try_exec_builtin(cmd) == 1) {
    return; // If it was a built-in command, we executed it and can return
  }
  // external command
  // 1. check if absolute path using is_absolute_path
  // 2. if absolute path, check that file exists and is executable using exe_exists_in_dir. exe_exists will check both
  // 3. if not absolute path, then do a loop through shell_paths and call joinpath between each shell_path command like "ls". 
  // if path contains "/bin" and "/usr/bin", then loop will check /bin/ls and /usr/bin/ls using exe_exists_in_dir
  // alter cmd->program to be the full path if found. if not found, print error and return
  if(is_absolute_path(cmd->program) == 1) {

  } else {
    bool found = false;
    for(int i = 0; i < MAX_ENTRIES_IN_SHELLPATH; i++) {
      if(shell_paths[i][0] == '\0') {
        break;
      }
      char *dir = shell_paths[i];
      char *fullPath = exe_exists_in_dir(dir, cmd->program, false);
      if(fullPath != NULL) {
        cmd->program = fullPath;
        cmd->args[0] = fullPath;
        found = true;
        break;
      }
    }
    if(!found) {
      print_errors();
      return;
    }
  }

  exec_external_cmd(cmd);
  
}

/** Execute built-in commands
 *
 * If the command is a built-in command, execute it and return 1 if appropriate
 * If the command is not a built-in command, do nothing and return 0
 */
int try_exec_builtin (struct Command *cmd)
{
  if(strcmp(cmd->program, "exit") == 0) {
    if(count_args(cmd->args) != 1) {
      print_errors();
      return 1;
    }
    exit(0);
  }

  if(strcmp(cmd->program, "cd") == 0) {
    if(count_args(cmd->args) != 2) {
      print_errors();
      return 1;
    }
    if(chdir(cmd->args[1]) == -1) {
      print_errors();
    }
    
    return 1;
  }

  if(strcmp(cmd->program, "path") == 0) {
    if(set_shell_path(&cmd->args[1]) == 0) {
      print_errors();
    }
    return 1;
  }

  return 0;
}

int count_args(char **args) {
    int count = 0;
    while (args[count] != NULL) {
        count++;
    }
    return count;
}

void print_errors() {
  char emsg[30] = "An error has occurred\n";
  int nbytes_written = write(STDERR_FILENO, emsg, strlen(emsg));
  if(nbytes_written != strlen(emsg)){
    exit(2);
  }
}

/** Execute an external command
 *
 * Execute an external command by fork-and-exec. Should also take care of
 * output redirection, if any is requested
 */
void exec_external_cmd (struct Command *cmd)
{
  int pid = fork();
  if(pid < 0) {
    print_errors();
    return;
  } else if(pid == 0) { // Child process
    if(execv(cmd->program, cmd->args) == -1) {
      print_errors();
      exit(1); // kills child process. return would fail to kill it.
    }
  } else { // Parent process
    int status;
    waitpid(pid, &status, 0);
    if(WIFEXITED(status) && WEXITSTATUS(status) != 0) {
      // print_errors();
    }
  }
  return;
}

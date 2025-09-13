#include <ctype.h>
#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

const int MAX = 13;

static void doFib (int n, int doPrint);

/*
 * unix_error - unix-style error routine.
 */
inline static void unix_error (char *msg)
{
  fprintf (stdout, "%s: %s\n", msg, strerror (errno));
  exit (1);
}

int main (int argc, char **argv)
{
  int arg;
  int print = 1;

  if (argc != 2)
    {
      fprintf (stderr, "Usage: fib <num>\n");
      exit (-1);
    }

  arg = atoi (argv[1]);
  if (arg < 0 || arg > MAX)
    {
      fprintf (stderr, "number must be between 0 and %d\n", MAX);
      exit (-1);
    }

  doFib (arg, print);

  return 0;
}

/*
 * Recursively compute the specified number. If print is
 * true, print it. Otherwise, provide it to my parent process.
 *
 * NOTE: The solution must be recursive and it must fork
 * a new child for each call. Each process should call
 * doFib() exactly once.
 */
static void doFib (int n, int doPrint) {
    // base case
    if (n == 0 || n == 1) {
        if (doPrint) {
            // prints only happen when you need the output for the fib command
            printf("%d\n", n);
            exit(0);
        }
        // status is just Fibonacci number if result is needed
        exit(n);
    }
    
    // need two processes to get the two numbers before this one in the Fibonacci sequence
    // process 1: previous previous Fibonacci number (fib1)
    pid_t process1 = fork();
    if (process1 < 0) {
        // fork failed
        unix_error("There was an error while forking a the process.");
    } else if (process1 == 0) {
        // in forked child process
        doFib(n - 2, 0);
    }
    // only parent process here, because child will go into recursion and exit there
    int process1Status;
    waitpid(process1, &process1Status, 0);
    int fib1 = WEXITSTATUS(process1Status);

    // process 2: previous Fibonacci number (fib2)
    pid_t process2 = fork();
    if (process2 < 0) {
        // fork failed
        unix_error("There was an error while forking a process.");
    } else if (process2 == 0) {
        // in forked child process
        doFib(n - 1, 0);
    }
    // only parent process here, because child will go into recursion and exit there
    int process2Status;
    waitpid(process2, &process2Status, 0);
    int fib2 = WEXITSTATUS(process2Status);

    // calculate result and print out result if needed 
    int result = fib1 + fib2;
    if (doPrint) {
        // prints only happen when you need the output for the fib command
        printf("%d\n", result);
        exit(0);
    }
    // status is just Fibonacci number if result is needed
    exit(result);
}

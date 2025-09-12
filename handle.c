#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <time.h>
#include <unistd.h>
#include "util.h"


/**
 * Handles SIGINT (Ctrl + C/ ^C).
 */
void interrupt_handler() {
    // attribution: Section 1.2, https://www.cs.utexas.edu/~ans/classes/cs439/projects/shell_project/shell.html
    ssize_t bytes;
    const int STDOUT = 1;
    bytes = write(STDOUT, "Nice try.\n", 10);
    if (bytes != 10) {
        exit(-999);
    }
}


/**
 * Handles SIGUSR1 (user-defined signal).
 */
void exit_handler() {
    // attribution (slightly modified here): Section 1.2, https://www.cs.utexas.edu/~ans/classes/cs439/projects/shell_project/shell.html
    ssize_t bytes;
    const int STDOUT = 1;
    bytes = write(STDOUT, "exiting\n", 8);
    if (bytes != 8) {
        exit(-999);
    }
    // end the process
    exit(0);
}


/*
 * First, print out the process ID of this process.
 *
 * Then, set up the signal handler so that ^C causes
 * the program to print "Nice try.\n" and continue looping.
 *
 * Finally, loop forever, printing "Still here\n" once every
 * second.
 */
int main(int argc, char **argv) {
    // print out the process ID
    printf("%d\n", (int) getpid());
    // for interrupt signal
    signal_action(2, interrupt_handler);
    // for user-defined kill signal
    signal_action(10, exit_handler);
    // loop every second to print 
    while (true) {
        printf("Still here\n");
        nanosleep((const struct timespec[]){{1, 0}}, NULL);
    }
    return 0;
}


#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <time.h>
#include <unistd.h>


int main(int argc, char **argv)
{
    // two arguments needed: mykill executable, process
    if (argc == 2) {
        // get process ID, kill process by passing in SIGUSR1
        const int pid = atoi(argv[1]);
        kill(pid, 10);
    }
    return 0;
}

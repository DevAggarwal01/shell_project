#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <time.h>
#include <unistd.h>
#include "util.h"


void handler(int sig) {
    ssize_t bytes;
    const int STDOUT = 1;
    bytes = write(STDOUT, "Nice try.\n", 10);
    if(bytes != 10)
      exit(-999);
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
int main(int argc, char **argv)
{
  printf("%d", (int) getpid());
  // ctrl + c is #2 signal
  signal_action(2, handler);

  while(1) {
    printf("Still here\n");
    nanosleep((const struct timespec[]){{1, 0}}, NULL);
  }
  return 0;
}





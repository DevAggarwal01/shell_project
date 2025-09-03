#include <assert.h>
#include <dirent.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <ctype.h>
#include <signal.h>
#include <sys/types.h>

#include "sig_util.h"

/*
 * signal_action - wrapper for the sigaction function
 * marginally modified from that distributed with B&O shell project
 */
handler_t *signal_action(int signum, handler_t *handler)
{
    struct sigaction action, old_action;

    action.sa_handler = handler;
    sigemptyset(&action.sa_mask); /* block sigs of type being handled */
    action.sa_flags = SA_RESTART; /* restart syscalls if possible */

    if (sigaction(signum, &action, &old_action) < 0)
    {
        fprintf(stdout, "Signal error: %s\n", strerror(errno));
        exit(1);
    }

    return (old_action.sa_handler);
}




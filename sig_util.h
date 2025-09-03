

typedef void handler_t(int);

/**
 * Set the signal handler to the desired function with appropriate flags
 * to avoid race conditions.
 * */

handler_t *signal_action(int signum, handler_t *handler);


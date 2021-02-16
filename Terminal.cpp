#include <iostream>
#include <signal.h>
#include <stdio.h>

#include <readline/readline.h>

#include "PromptString.h"
#include "Terminal.h"

namespace BShell {
void handler$posix_sig(int) {
    // Main objective of this handler is to stop the shell process
    // from exiting upon SIGINT/CTRL+C.

    // This function is empty so the signal doesn't get processed
    // and exit out of the process, however, we don't want to
    // pass SIG_IGN to ignore the signal, as we need to execute
    // the handler$readline_sig() function below. With SIG_IGN
    // that function will not be called.
}

void handler$readline_sig(void) {
    // This function handles when readline is interrupted by a signal
    // In the case of CTRL+C, the SIGINT sends ETX which the terminal
    // displays as ^C. This function clears the line buffer and goes
    // to a new line, so the user can continue to use the shell
    // without garbage in the terminal display or line buffer.

    // I'm not entirely sure how to check to see if it was SIGINT
    // that caused this handler function to run. From the readline
    // docs it says that rl_hook_func_t takes no parameters.

    rl_crlf();
    rl_reset_line_state();
    rl_replace_line("", 1);
    rl_on_new_line();
    rl_redisplay();
}
}
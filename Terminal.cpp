#include <iostream>
#include <signal.h>
#include <stdio.h>

#include <readline/readline.h>

#include "PromptString.h"
#include "Terminal.h"

namespace BShell {
volatile bool g_fsigint = false;

void rl$reset() {
    rl_crlf();
    rl_reset_line_state();
    rl_replace_line("", 1);
    rl_on_new_line();
    rl_redisplay();
}

void handler$posix_sig(int) {
    // Main objective of this handler is to stop the shell process
    // from exiting upon SIGINT/CTRL+C.

    // This function does not reset the prompt string due to unsafe
    // malloc from GNU readline within signal handler. Use
    // rl_hook_func_t to hook into readline's signal handler to
    // reset the prompt string. Use flag to determine when we have
    // received SIGINT, because rl_hook_func_t does not tell us
    // which signal has been called.

    g_fsigint = true;
}

void handler$readline_sig(void) {
    // This function handles when readline is interrupted by a signal

    if (g_fsigint) {
        // In the case of CTRL+C, the SIGINT sends ETX which the terminal
        // displays as ^C. This function clears the line buffer and goes
        // to a new line, so the user can continue to use the shell
        // without garbage in the terminal display or line buffer.

        rl$reset();
        g_fsigint = false;
    }
}
}
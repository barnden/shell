#include <iostream>
#include <signal.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <unistd.h>

#include <stdio.h>
#include <readline/readline.h>
#include <readline/history.h>

#include "Terminal.h"
#include "Shell.h"
#include "Parser.h"
#include "PromptString.h"

// As readline(3) man page only covers user usage of GNU readline
// the programmer documentation is found below.
// https://tiswww.case.edu/php/chet/readline/readline.html#IDX338

int main(int argc, int*argv[]) {
    // I can't find any resources on whether this is actually "safe", as cannot use the
    // GNU readline library in the BShell::handler$posix_sig(int) function, as readline
    // uses malloc for most functions that display to the terminal.
    rl_signal_event_hook = reinterpret_cast<rl_hook_func_t*>(BShell::handler$readline_sig);

    // Ignore SIGINT from CTRL+C, only exit on CTRL+D
    if (signal(SIGINT, BShell::handler$posix_sig) == SIG_ERR) {
        // Pass the handler$posix_sig(int) instead of SIG_IGN so the
        // rl_signal_event_hook function gets called.
        std::cerr << "Could not bind signal handler handler$posix_sig() to SIGINT.\n";
        exit(1);
    }

    // Use the GNU history library to store previous commands.
    using_history();
    auto * hstate = history_get_history_state();
    auto ** hlist = history_list();
    stifle_history(64);

    // Continually prompt the user for input
    while (true) {
        // GNU readline reads from STDIN as char*, allocating the C string with malloc
        // The advantage over std::getline or (>>) from std::cin is that readline
        // allows the user to use control keys like the arrows or home keys to modify
        // their given input, rather than putting a control https://tiswww.case.edu/php/chet/readline/readline.html#IDX338sequence, i.e. [[D instead
        // of moving the cursor to the left.
        auto*input = readline(BShell::get$PS1().c_str());

        // CTRL+D causes terminal to send EOF which GNU readline interprets as NULL
        if (input == NULL) break;

        add_history(input);

        BShell::input$parse(const_cast<const char*>(input));

        free(input);
    }

    // Presumably there is a way to serialize the HISTORY_STATE struct,
    // and store it in some file, then load that file the next time the
    // shell is launched and initialize the history with the previous
    // session's commands a la bash.
    free(hstate);
    free(hlist);

    std::cout << "brandon shell exited\n";

    return 0;
}

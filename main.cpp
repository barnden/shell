#include <iostream>
// #include <format>

#include <readline/readline.h>
#include <readline/history.h>
#include <signal.h>
#include <stdio.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <unistd.h>

#include "Terminal.h"
#include "Shell.h"
#include "Parser.h"
#include "PromptString.h"

void print$bgproc() {
    for (auto&proc : BShell::g_processes) {
        if (waitpid(proc.pid, &BShell::g_exit_bg, WNOHANG) == 0) continue;

        #if 0
        // Turns out that std::format is not implemented by gcc 10 as of time of writing
        std::cout
        << std::format(
            "[{}] {:12} {}\n",
            &proc - &*BShell::g_processes.begin() + 1, "Done", proc.name
        );
        #else
        std::cout
        << '['
        << (&proc - &*BShell::g_processes.begin() + 1)
        << "] Done "
        << proc.name
        << '\n';
        #endif
    }
}

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
        auto* input = readline(BShell::get$PS1().c_str());

        // CTRL+D causes terminal to send EOF which GNU readline interprets as NULL
        if (input == NULL) break;

        print$bgproc();
        BShell::edproc();

        if (strlen(input)) {
            add_history(input);

            auto tokens = BShell::Tokenizer(const_cast<const char*>(input)).tokens();
            auto asts = BShell::Parser(tokens).asts();

            std::cout << asts.size() << '\n';

            for (auto*ast : asts) {
                // BShell::handle$ast(ast);
                BShell::ast$delete_children(ast);
            }
        }

        free(input);
    }

    // Presumably there is a way to serialize the HISTORY_STATE struct,
    // and store it in some file, then load that file the next time the
    // shell is launched and initialize the history with the previous
    // session's commands a la bash.
    free(hstate);
    free(hlist);

    // Terminate all children using SIGTERM
    // TODO: Should we hang parent to ensure children are dead, or even SIGKILL children?
    // Currently we use WNOHANG to see if child is alive, inside BShell::edproc()
    // if waitpid returns non-zero child is alive and we attempt to kill it again
    while (BShell::g_processes.size()) {
        BShell::edproc();

        for (auto&proc : BShell::g_processes)
            kill(proc.pid, SIGTERM);
    }

    std::cout << "brandon shell exited\n";

    return 0;
}

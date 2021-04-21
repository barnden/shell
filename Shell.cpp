#include <cstdlib>
#include <iostream>
// #include <format>

#include <signal.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <termios.h>
#include <unistd.h>

#include "Interpreter.h"
#include "Parser.h"
#include "PromptString.h"
#include "Terminal.h"

void print$bgproc() {
    for (auto& proc : BShell::g_processes) {
        if (waitpid(proc.pid, &BShell::g_exit_bg, WNOHANG) == 0)
            continue;

        std::cout
            << '[' << (&proc - &*BShell::g_processes.begin() + 1) << "] Done "
            << proc.name
            << '\n';
    }
}

int main(int argc, int* argv[]) {
    if (signal(SIGINT, BShell::handle$sigint) == SIG_ERR) {
        std::cerr << "Failed to bind SIGINT.\n";
        exit(1);
    }

    std::atexit(BShell::terminal$restore);

    // Continually prompt the user for input
    while (true) {
        auto input = BShell::get$input(BShell::get$PS1());

        // CTRL+D causes terminal to send EOF, which is interpreted as \x1b[EOF
        if (input == "\x1b[EOF")
            break;

        print$bgproc();
        BShell::erase_dead_children();

        if (input.size()) {
            BShell::g_history.push_back(input);

            auto tokens = BShell::Tokenizer(input).tokens();
            auto asts = BShell::Parser(std::move(tokens)).asts();

            for (auto ast : asts)
                BShell::handle$ast(ast);
        }
    }

    return 0;
}

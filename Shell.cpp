#include <cstdlib>
#include <iostream>
// #include <format>

#include <signal.h>
#include <stdio.h>
#include <sys/wait.h>
#include <termios.h>
#include <sys/types.h>
#include <unistd.h>

#include "Terminal.h"
#include "Interpreter.h"
#include "Parser.h"
#include "PromptString.h"

#define DEBUG_TOKEN false
#define DEBUG_AST false

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
        << '[' << (&proc - &*BShell::g_processes.begin() + 1) << "] Done "
        << proc.name
        << '\n';
        #endif
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
        BShell::terminal$control();
        auto input = BShell::get$input(BShell::get$PS1());
        std::cout << "\x1b[2K\x1b[1G";
        BShell::terminal$restore();

        // CTRL+D causes terminal to send EOF, which is interpreted as \x1b[EOF
        if (input == "\x1b[EOF") break;

        print$bgproc();
        BShell::erase_dead_children();

        if (input.size()) {
            BShell::g_history.push_back(input);

            auto tokens = BShell::Tokenizer(input).tokens();
            auto asts = BShell::Parser(tokens).asts();

            #if DEBUG_TOKEN
            std::cout << "--{Token Begin}--\n";
            for (auto& t : tokens)
                std::cout << t << '\n';
            std::cout << "--{Token End}--\n";
            #endif

            for (auto ast : asts) {
                #if DEBUG_AST
                std::cout << "--{AST Begin}--\n";
                BShell::ast$print(ast);
                std::cout << "--{AST End}--\n";
                #endif

                BShell::handle$ast(ast);
            }
        }
    }

    std::cout << "\nbrandon shell exited\n";

    return 0;
}

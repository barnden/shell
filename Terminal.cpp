#include <string>

#include <iostream>
#include <vector>

#include <stdio.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>

#include "PromptString.h"
#include "Terminal.h"
#include "Tokenizer.h"

namespace BShell {
termios g_term, g_oterm;
std::vector<std::string> g_history = std::vector<std::string> {};

void handle$sigint(int) {
    // We do not want to completely SIG_IGN SIGINT, however,
    // we also do not want the terminal to exit on SIGINT.
}

std::vector<std::string> g_token_colors = {
    "\x1b[0m",  // NullToken
    "\x1b[0m",  // String
    "\x1b[0m",  // Equal
    "\x1b[34m", // Executable
    "\x1b[31m", // Background
    "\x1b[31m", // Sequential
    "\x1b[32m", // SequentialIf
    "\x1b[32m", // RedirectPipe
    "\x1b[32m", // RedirectOut
    "\x1b[32m", // RedirectIn
    "\x1b[34m", // Key
    "\x1b[36m", // Eval
    "\x1b[0m",  // StickyRight
    "\x1b[0m",  // StickyLeft
    "\x1b[0m",  // WhiteSpace
};

std::string color_tokens(std::string input) {
    auto str = std::string {};
    auto tokens = Tokenizer(input, true).tokens();

    for (auto &t : tokens)
        str += g_token_colors[static_cast<std::size_t>(t.type)] + t.content;

    return str + "\x1b[0m";
}

std::string get$input(std::string prompt) {
    auto c = char {};
    auto line_buf = std::string {};

    int cursor[2] = {0, 0};

    std::cout << "\x1b[2K\x1b[1G" << prompt;

    while (read(STDIN_FILENO, &c, 1) == 1) {
        // termios::c_cc is runtime; no switches ;(
        if (c == g_term.c_cc[VEOF]) {
            // CTRL+D (EOF to STDIN)
            std::cout << "^D\x1b[1G";
            return "\x1b[EOF";
        }
        
        if (c == '\r') {
            std::cout << '\n';
            return line_buf;
        }

        if (c == g_term.c_cc[VINTR]) {
            // CTRL+C (SIGINT)
            std::cout << "^C";
            std::cout << "\n\x1b[1G" << prompt;
            cursor[0] = 0;
            line_buf = "";

            continue;
        } else if (c == g_term.c_cc[VERASE]) {
            // Backspace
            if (line_buf.size()) {
                line_buf.erase(--cursor[0], 1);
                std::cout
                    << "\x1b[2K\r"
                    << prompt << color_tokens(line_buf)
                    << "\x1b[1G\x1b[" << (cursor[0] + prompt.size()) << "C";
            }

            continue;
        } else if (c == '\x1b') {
            // Special characters
            char inp[4];
            auto count = 0;

            if ((count = read(STDIN_FILENO, &inp, 3)) > 0) {
                inp[count] = '\0';

                if (strcmp(inp, "[A") == 0) {
                    // UP
                    if (cursor[1] < g_history.size())
                        ++cursor[1];
                } else if (strcmp(inp, "[B") == 0) {
                    // DOWN
                    if (cursor[1] > 0)
                        --cursor[1];
                } else if (strcmp(inp, "[C") == 0) {
                    if (cursor[0] < line_buf.size()) {
                        ++cursor[0];
                        std::cout << "\x1b[1C";
                    }
                } else if (strcmp(inp, "[D") == 0) {
                    if (cursor[0] > 0) {
                        --cursor[0];
                        std::cout << "\x1b[1D";
                    }
                } else if (strcmp(inp, "[3~") == 0) {
                    if (line_buf.size()) {
                        line_buf.erase(cursor[0], 1);
                        std::cout
                            << "\x1b[2K\r"
                            << prompt << color_tokens(line_buf)
                            << "\x1b[1G\x1b[" << (cursor[0] + prompt.size()) << "C";
                    }
                } else if (strcmp(inp, "[5~") == 0) {
                    // PGUP
                    if (cursor[1] + 5 < g_history.size())
                        cursor[1] += 5;
                } else if (strcmp(inp, "[6~") == 0) {
                    // PGDN
                    if (cursor[1] >= 5)
                        cursor[1] -= 5;
                } else if (strcmp(inp, "[H") == 0) {
                    // HOME
                    if (cursor[0])
                        std::cout << "\x1b[" << cursor[0] << "D";

                    cursor[0] = 0;
                } else if (strcmp(inp, "[F") == 0) {
                    // END
                    if (cursor[0] < line_buf.size()) {
                        std::cout << "\x1b[" << (line_buf.size() - cursor[0]) << "C";
                        cursor[0] = line_buf.size();
                    }
                }
            }

            continue;
        }

        if (cursor[0] < line_buf.size()) {
            if (line_buf.size())
                line_buf.insert(cursor[0], 1, c);
            else
                line_buf = std::string { c };

            std::cout
                << "\x1b[2K\r" << prompt << color_tokens(line_buf)
                << "\x1b[1G\x1b[" << (++cursor[0] + prompt.size()) << "C";
        } else {
            ++cursor[0];
            line_buf += c;
            std::cout << "\x1b[1K\x1b[1G\r" << prompt << color_tokens(line_buf);
        }
    }
}

void terminal$control() {
    tcgetattr(STDIN_FILENO, &BShell::g_term);
    BShell::g_oterm = BShell::g_term;

    // Set terminal into raw mode
    BShell::g_term.c_iflag &= ~(BRKINT | ICRNL | INPCK | ISTRIP | IXON);
    BShell::g_term.c_oflag &= ~OPOST;
    BShell::g_term.c_lflag &= ~(ECHO | ICANON | IEXTEN | ISIG);
    tcsetattr(STDIN_FILENO, TCSANOW, &BShell::g_term);

    std::cout.setf(std::ios::unitbuf);
}

void terminal$restore() {
    tcsetattr(STDIN_FILENO, TCSANOW, &BShell::g_oterm);
    std::cout.setf(~std::ios::unitbuf);
}
}
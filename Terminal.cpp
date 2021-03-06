#include <string>

#include <filesystem>
#include <iostream>
#include <vector>

#include <stdio.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>

#include "PromptString.h"
#include "Shell.h"
#include "Terminal.h"
#include "Tokenizer.h"

namespace BShell {
termios g_term, g_oterm;
std::vector<std::string> g_history = std::vector<std::string> {};

void handle$sigint(int) {
    // We do not want the shell to exit on SIGINT
    // but we also do not want to SIG_IGN SIGINT.
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

std::string line$color(std::string input) {
    auto str = std::string {};
    auto tokens = Tokenizer(input, true).tokens();

    for (auto &t : tokens)
        str += g_token_colors[static_cast<std::size_t>(t.type)] + t.content;

    return str + "\x1b[0m";
}

void line$reprint(const std::string& prompt, const std::string& line_buf, int x) {
    std::cout
        << "\x1b[2K\r"
        << prompt << line$color(line_buf);

    if (x > 0)
        std::cout << "\x1b[1G\x1b[" << std::to_string(x) << "C";
}

void history$prev(int* cursor, bool& lup, const std::string& prompt, std::string& line_buf) {
    if (g_history.size() && cursor[1] < g_history.size()) {
        lup = true;

        line_buf = *(g_history.rbegin() + cursor[1]++);
        cursor[0] = line_buf.size();

        line$reprint(prompt, line_buf, cursor[0] + prompt.size());
    }
}

void history$next(int* cursor, bool& lup, const std::string& prompt, std::string& line_buf) {
    if (lup) {
        lup = false;
        cursor[1]--;
    }

    line_buf = cursor[1] ? *(g_history.rbegin() + --cursor[1]) : "";
    cursor[0] = line_buf.size();

    line$reprint(prompt, line_buf, cursor[0] + prompt.size());
}

std::string get$input(std::string prompt) {
    auto c = char {};
    auto line_buf = std::string {};
    auto lup = false;
    int cursor[2] = {0, 0};

    // Print out prompt before starting loop
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
        }

        if (c == g_term.c_cc[VERASE]) {
            // Backspace
            if (line_buf.size() && cursor[0] > 0){
                line_buf.erase(--cursor[0], 1);
                line$reprint(prompt, line_buf, cursor[0] + prompt.size());
            }

            continue;
        }

        if (c == '\t') {
            auto last = line_buf;
            auto ac_index = 0;
            std::vector<std::filesystem::path> files = {std::filesystem::directory_iterator(get$cwd()), {}};
            std::vector<std::string> filenames;

            for (auto& f : files) {
                auto fname = f.string();
                auto find = fname.find_last_of('/');

                if (find != std::string::npos)
                    fname = fname.substr(find + 1);

                filenames.push_back(fname);
            }

            if ((ac_index = line_buf.find_last_of(' ')) != std::string::npos)
                last = line_buf.substr(ac_index + 1);

            filenames.erase(
                std::remove_if(filenames.begin(), filenames.end(),
                    [=](std::string str) {
                        return str.size() < last.size() || str.substr(0, last.size()) != last;
                    }
                ), filenames.end()
            );

            if (filenames.size()) {
                line_buf += filenames[0].substr(last.size());
                cursor[0] = line_buf.size();
                line$reprint(prompt, line_buf, cursor[0] + prompt.size());
            }
            continue;
        } else if (c == '\x1b') {
            // Special characters
            char inp[4];
            auto count = 0;

            if ((count = read(STDIN_FILENO, &inp, 3)) > 0) {
                inp[count] = '\0';

                if (strcmp(inp, "[A") == 0) {
                    // ARROW UP
                    history$prev(cursor, lup, prompt, line_buf);
                } else if (strcmp(inp, "[B") == 0) {
                    // ARROW DOWN
                    history$next(cursor, lup, prompt, line_buf);
                } else if (strcmp(inp, "[C") == 0) {
                    // ARROW RIGHT
                    if (cursor[0] < line_buf.size()) {
                        ++cursor[0];
                        std::cout << "\x1b[1C";
                    }
                } else if (strcmp(inp, "[D") == 0) {
                    // ARROW LEFT
                    if (cursor[0] > 0) {
                        --cursor[0];
                        std::cout << "\x1b[1D";
                    }
                } else if (strcmp(inp, "[3~") == 0) {
                    // DEL
                    if (line_buf.size()) {
                        line_buf.erase(cursor[0], 1);
                        line$reprint(prompt, line_buf, cursor[0] + prompt.size());
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

        cursor[1] = 0;

        if (cursor[0] < line_buf.size())
            line_buf.insert(cursor[0], 1, c);
        else
            line_buf += c;

        line$reprint(prompt, line_buf, ++cursor[0] + prompt.size());
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

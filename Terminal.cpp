#include <filesystem>
#include <iostream>
#include <string>
#include <unordered_map>
#include <vector>

#include <stdio.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>

#include "PromptString.h"
#include "System.h"
#include "Terminal.h"
#include "Tokenizer.h"

namespace BShell {
termios g_term, g_oterm;
std::vector<std::string> g_history = std::vector<std::string> {};

std::unordered_map<TokenType, std::string> g_token_colors = {
    { NullToken, "\x1b[0m" },     { String, "\x1b[0m" },        { Equal, "\x1b[0m" },
    { Executable, "\x1b[34m" },   { Background, "\x1b[31m" },   { Sequential, "\x1b[31m" },
    { SequentialIf, "\x1b[32m" }, { RedirectPipe, "\x1b[32m" }, { RedirectOut, "\x1b[32m" },
    { RedirectIn, "\x1b[32m" },   { Key, "\x1b[34m" },          { Eval, "\x1b[36m" },
    { StickyRight, "\x1b[0m" },   { StickyLeft, "\x1b[0m" },    { WhiteSpace, "\x1b[0m" },
};

void handle$sigint(int) {
    // We do not want the shell to exit on SIGINT
    // but we also do not want to SIG_IGN SIGINT.
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

std::string line$color(std::string input) {
    auto str = std::string {};
    auto tokens = Tokenizer(input, true).tokens();

    for (auto const& t : tokens)
        str += g_token_colors[t.type] + t.content;

    return str + "\x1b[0m";
}

void line$reprint(std::string const& prompt, std::string const& input, int x) {
    std::cout << "\x1b[2K\r" << prompt << line$color(input);

    if (x > 0)
        std::cout << "\x1b[1G\x1b[" << std::to_string(x) << "C";
}

void history$prev(int& x, int& y, bool& lup, std::string const& prompt, std::string& input) {
    if (!g_history.size() || y >= g_history.size())
        return;

    lup = true;

    input = *(g_history.rbegin() + y++);
    x = input.size();

    line$reprint(prompt, input, x + prompt.size());
}

void history$next(int& x, int& y, bool& lup, std::string const& prompt, std::string& input) {
    if (lup) {
        lup = false;
        y--;
    }

    input = y ? *(g_history.rbegin() + --y) : "";
    x = input.size();

    line$reprint(prompt, input, x + prompt.size());
}

void terminal$ansi_handler(std::string const& prompt, int& x, int& y, std::string& input,
                           bool& lup) {
    char ansi[4];
    auto count = read(STDIN_FILENO, &ansi, 3);

    if (count == 0)
        return;

    if (count <= 0) {
        perror("read()");
        exit(1);
    }

    ansi[count] = '\0';

    if (strcmp(ansi, "[A") == 0) {
        // ARROW UP
        history$prev(x, y, lup, prompt, input);
        return;
    }

    if (strcmp(ansi, "[B") == 0) {
        // ARROW DOWN
        history$next(x, y, lup, prompt, input);
        return;
    }

    if (strcmp(ansi, "[C") == 0) {
        // ARROW RIGHT
        if (x < input.size()) {
            ++x;
            std::cout << "\x1b[1C";
        }
        return;
    }

    if (strcmp(ansi, "[D") == 0) {
        // ARROW LEFT
        if (x > 0) {
            --x;
            std::cout << "\x1b[1D";
        }
        return;
    }

    if (strcmp(ansi, "[3~") == 0) {
        // DEL
        if (input.size()) {
            input.erase(x, 1);
            line$reprint(prompt, input, x + prompt.size());
        }
        return;
    }

    if (strcmp(ansi, "[5~") == 0) {
        // PGUP
        if (y + 5 < g_history.size())
            y += 5;
        return;
    }

    if (strcmp(ansi, "[6~") == 0) {
        // PGDN
        history$prev(x, y, lup, prompt, input);
        return;
    }

    if (strcmp(ansi, "[H") == 0) {
        // HOME
        if (x)
            std::cout << "\x1b[" << x << "D";

        x = 0;
        return;
    }

    if (strcmp(ansi, "[F") == 0) {
        // END
        if (x < input.size()) {
            std::cout << "\x1b[" << (input.size() - x) << "C";
            x = input.size();
        }
        return;
    }
}

bool terminal$cache_fnames(std::string const& shadow, std::vector<std::string>& fnames) {
    auto files
        = std::vector<std::filesystem::path> { std::filesystem::directory_iterator(get$cwd()), {} };

    if (!files.size())
        return false;

    std::transform(files.begin(), files.end(), std::back_inserter(fnames),
                   [](std::filesystem::path const& path) { return path.filename(); });

    return !!fnames.size();
}

void terminal$autocomplete(int& x, std::vector<std::string>& fnames, std::string const& prompt,
                           std::string& shadow, std::string& input, int c) {
    if (!fnames.size())
        if (!terminal$cache_fnames(shadow, fnames))
            return;

    auto index = c % fnames.size();
    input = shadow + fnames[index];
    x = input.size();
    line$reprint(prompt, input, x + prompt.size());
}

std::string get$input(std::string const& prompt) {
    auto chr = char {};
    auto input = std::string {};
    auto shadow = std::string {};
    auto fname_cache = std::vector<std::string> {};

    auto x = 0, y = 0, z = 0;
    auto lup = false;

    terminal$control();

    // Print prompt string before starting loop
    std::cout << "\x1b[2K\x1b[1G" << prompt;

    while (read(STDIN_FILENO, &chr, 1) == 1) {
        // termios::c_cc is runtime; no switches ;(
        if (chr == g_term.c_cc[VEOF]) {
            // CTRL+D (EOF)
            std::cout << "^D\x1b[1G\nbrandon shell exited\n\x1b[2K\x1b[1G";
            return "\x1b[EOF";
        }

        if (chr == g_term.c_cc[VINTR]) {
            // CTRL+C (SIGINT)
            std::cout << "^C\n\x1b[1G" << prompt;

            x = y = z = 0;
            input = shadow = "";

            continue;
        }

        if (chr == g_term.c_cc[VERASE]) {
            // Backspace
            if (input.size() && x) {
                input.erase(--x, 1);
                shadow = input;
                z = 0;
                line$reprint(prompt, input, x + prompt.size());
            }

            continue;
        }

        if (chr == 23) {
            // CTRL+Backspace
        }

        if (chr == '\r') {
            // Return
            std::cout << "\n\x1b[2K\x1b[1G";
            terminal$restore();

            return input;
        }

        if (chr == '\t') {
            // Tab
            terminal$autocomplete(x, fname_cache, prompt, shadow, input, z++);
            continue;
        }

        if (chr == '\x1b') {
            // ANSI control characters
            shadow = input;
            z = 0;
            terminal$ansi_handler(prompt, x, y, input, lup);
            continue;
        }

        input.insert(x, 1, chr);
        shadow = input;

        x++;
        y = z = 0;

        line$reprint(prompt, input, x + prompt.size());
    }

    return input;
}
}
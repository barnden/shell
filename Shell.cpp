#include <algorithm>
#include <fstream>
#include <iostream>
#include <memory>
#include <string>
#include <vector>

#include <fcntl.h>
#include <pwd.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include "Commands.h"
#include "Shell.h"
#include "Terminal.h"

namespace BShell {
std::string g_prev_wd = "";
std::vector<Process> g_processes;
int g_exit_fg = 0;
int g_exit_bg = 0;

std::string get$cwd() {
    // From getcwd(3) it says get_current_dir_name() will malloc a
    // string large enough to fit the current working dir.
    // While getcwd() requires us to create our own string buffer.
    auto* buf = get_current_dir_name();
    auto cwd = std::string(buf);

    free(buf);

    return cwd;
}

std::string get$home() {
    // Use getenv "HOME" otherwise, fallback to getpwuid(3)
    // and getuid(3) to get the user's home directory.

    auto* buf = getenv("HOME");

    if (buf == nullptr)
        buf = getpwuid(getuid())->pw_dir;

    return std::string(buf);
}

std::string get$username() {
    // getlogin(3)
    // Suggests that we use getenv to get "LOGNAME", otherwise
    // fallback to getlogin(), then getlogin_r()

    auto* buf = getenv("LOGNAME");
    auto username = std::string {};

    if (buf == nullptr && (buf = getlogin()) == nullptr){
        // From useradd(8), usernames can be at most 32 chars long.
        auto uptr = std::make_unique<char[]>(32);
        buf = uptr.get();

        getlogin_r(buf, 32);
    }

    username = std::string(buf);

    return username;
}

std::string get$hostname() {
    auto buf = getenv("HOSTNAME");
    auto hostname = std::string {};

    if (buf != nullptr) hostname = std::string(buf);
    else {
        // hostname(7) states that the maximum hostname is 253 chars.
        auto uptr = std::make_unique<char[]>(253);
        gethostname(uptr.get(), 253);

        hostname = std::string { *uptr.get() };
    }

    return hostname;
}

std::string get$pname(pid_t pid) {
    auto proc = std::ifstream("/proc/" + std::to_string(pid) + "/cmdline");

    return std::string { std::istreambuf_iterator { proc.rdbuf() }, {} };
}

std::string get$pname(const std::shared_ptr<Expression>& expr) {
    auto pname = std::string {};

    if (expr->token.type != Executable)
        return pname;

    pname = expr->token.content;

    if (expr->children.size())
        for (const auto&c : expr->children)
            pname += ' ' + c->token.content;

    return pname;
}

std::string get$executable_path(std::string cmd) {
    // Scans the path for executables that match the given string

    auto env_path = std::string(getenv("PATH"));
    auto path = std::string {};

    auto i = size_t {}, j = env_path.find(':');

    for (; j != std::string::npos; j = env_path.find(':', i = j + 1)) {
        auto tmp = env_path.substr(i, j - i) + "/" + cmd;

        if (access(tmp.c_str(), X_OK) != -1) {
            path = tmp;
            break;
        }
    }

    return path;
}

int get$file(std::string path) {

}

void get$eval(std::string& str, Token token) {
    // Recursively tokenize and parse eval string until we get something
    auto tokens = Tokenizer(token.content.c_str()).tokens();
    auto asts = Parser(tokens).asts();
    auto eval_io = Pipe {};

    if (pipe(eval_io.fd) < 0) {
        std::cerr << "pipe()\n";
        exit(1);
    }

    for (auto& ast : asts){
        handle$ast(ast, [&]{
            dup2(eval_io.fd[1], STDOUT_FILENO);
            std::cout.flush();

            close(eval_io.fd[0]);
            close(eval_io.fd[1]);
        });
    }

    write(eval_io.fd[1], NULL, 1);
    close(eval_io.fd[1]);

    auto buf = std::make_unique<char[]>(BUFSIZ);
    auto fmem = fdopen(eval_io.fd[0], "r");

    while (fgets(buf.get(), BUFSIZ, fmem))
        str += std::string { buf.get() };

    close(eval_io.fd[0]);
}

void handle$argv_strings(std::vector<std::string>& argv, bool& sticky, Token token) {
    auto str = std::string {};

    if (token.type == Eval)
        get$eval(str, token);
    else
        str = token.content;

    if (sticky || token.type == StickyLeft) {
        str += argv.back();

        argv.pop_back();
    }

    if (token.type == StickyRight)
        sticky = true;

    // Remove unprintable control characters like SOH, STX, ETX, etc.
    str.erase(std::remove_if(str.begin(), str.end(), [=](int c) { return !std::isprint(c); } ), str.end());

    argv.push_back(str);
}

void handle$keyword(const std::shared_ptr<Expression>& expr) {
    auto kw = expr->token.content;
    auto index = std::distance(g_keywords.find(kw), g_keywords.end());

    // TODO: Maybe use an enum for more readability
    switch (index) {
        case 1: // export
            break;
        case 2: // cd
            command$cd(expr);
            break;
        case 3: // jobs
            break;
    }
}

void handle$redirect_in(const std::shared_ptr<Expression>& redirect_in) {
    if (redirect_in == nullptr) return;

    auto filename = std::string {};

    if (redirect_in->token.type == String || redirect_in->token.type == StickyLeft)
        filename = redirect_in->token.content;
    else
        get$eval(filename, redirect_in->token);

    auto file = open(filename.c_str(), O_RDONLY);

    if (file < 0) {
        std::cerr << filename << ": No such file or directory\n";
        exit(1);
    }

    if (dup2(file, STDIN_FILENO) < 0) {
        std::cerr << "dup2()\n";
        exit(1);
    }

    close(file);
}

void handle$redirect_out(const std::shared_ptr<Expression>& redirect_out) {
    if (redirect_out == nullptr) return;

    auto filename = std::string {};

    if (redirect_out->token.type == String || redirect_out->token.type == StickyLeft)
        filename = redirect_out->token.content;
    else
        get$eval(filename, redirect_out->token);

    auto file = open(filename.c_str(), O_CREAT | O_WRONLY, 0644);

    if (file < 0)
        std::cerr << filename << ": No such file or directory\n";

    if (dup2(file, STDOUT_FILENO) < 0) {
        std::cerr << "dup2()\n";
        exit(1);
    }

    std::cout.flush();

    close(file);
}

std::vector<std::string> handle$argv(const std::shared_ptr<Expression>& expr) {
    auto argv = std::vector<std::string> { expr->token.content };
    auto sticky = false;

    for (auto& child : expr->children) {
        switch (child->token.type) {
            case StickyRight:
            case Eval:
            case String:
            case StickyLeft:
                handle$argv_strings(argv, sticky, child->token);
                break;
            case RedirectIn:
                handle$redirect_in(child->children[0]);
                break;
            case RedirectOut:
                handle$redirect_out(child->children[0]);
                break;
        }
    }

    return argv;
}

template<typename T>
Process execute(const std::shared_ptr<Expression>& expr, T&& child_hook) {
    auto pid = fork();

    if (pid < 0) {
        std::cerr << "Failed to fork()\n";

        exit(1);
    } else if (!pid) {
        auto args = handle$argv(expr);
        auto argv = std::vector<char*> {};

        std::transform(
            args.begin(), args.end(),
            std::back_inserter(argv),
            [](const std::string& str) { return const_cast<char*>(str.c_str()); }
        );

        argv.push_back(NULL);

        child_hook();

        execvp(argv[0], &argv[0]);

        std::cerr << "Failed to execvp()\n";
        exit(1);
    }

    return Process { pid, get$pname(expr) };
}

template<typename T>
void handle$executable(const std::shared_ptr<Expression>& expr, T&& hook) {
    auto proc = execute(expr, hook);

    waitpid(proc.pid, &g_exit_fg, 0);
}

void handle$background(const std::shared_ptr<Expression>& expr) {
    auto proc = execute(expr->children[0], [=]{});

    std::cout << '[' << g_processes.size() + 1 << "] " << proc.pid << '\n';

    waitpid(proc.pid, &g_exit_bg, WNOHANG);
    g_processes.push_back(Process { proc.pid, proc.name });
}

void handle$sequential(const std::shared_ptr<Expression>& expr) {
    for (auto& child : expr->children) {
        auto proc = Process {};

        proc = execute(child, [&]() -> void { });

        waitpid(proc.pid, &g_exit_fg, 0);

        if (expr->token.type == SequentialIf && g_exit_fg != 0)
            break;
    }
}

template<typename T>
void handle$pipe(const std::shared_ptr<Expression>& expr, T&& last_hook) {
    auto last_io = Pipe {};

    for (auto& child : expr->children) {
        // TODO: Maybe figure out something other than this callback structure.
        auto proc = Process {};
        auto proc_io = Pipe {};

        if (pipe(proc_io.fd) < 0) {
            std::cerr << "Unexpected error when opening pipe, proc_io.\n";

            exit(1);
        }

        proc = execute(child,
            [&]() -> void {
                // This entire lambda function executes within the child process.
                if (child != expr->children.front()) {
                    dup2(last_io.fd[0], STDIN_FILENO);

                    close(last_io.fd[0]);
                    close(last_io.fd[1]);
                }

                if (child != expr->children.back())
                    dup2(proc_io.fd[1], STDOUT_FILENO);
                else
                    last_hook();

                close(proc_io.fd[0]);
                close(proc_io.fd[1]);
            }
        );

        if (child != expr->children.front()) {
            // Close the file descriptor in the parent process.
            close(last_io.fd[0]);
            close(last_io.fd[1]);
        }

        last_io = proc_io;

        if (waitpid(-1, &g_exit_fg, 0) < 0) {
            std::cerr << "waitpid()\n";

            exit(1);
        }
    }
}

template<typename T>
void handle$ast(const std::shared_ptr<Expression>& ast, T&& hook) {
    switch (ast->token.type) {
        case Key:
            return handle$keyword(ast);
        case Executable:
            return handle$executable(ast, hook);
        case Background:
            return handle$background(ast);
        case RedirectPipe:
            return handle$pipe(ast, hook);
        case SequentialIf:
        case Sequential:
            return handle$sequential(ast);
        case Equal:
            return command$set_env(ast);
        default:
            std::cerr << "Bad token type passed to handle$ast\n";
            exit(1);
    }
}

void handle$ast(const std::shared_ptr<Expression>& ast) {
    handle$ast(ast, [=](){});
}

void erase_dead_children() {
    g_processes.erase(
        std::remove_if(
            g_processes.begin(), g_processes.end(),
            [=](const Process& proc) -> bool { return waitpid(proc.pid, &g_exit_bg, WNOHANG) != 0; }
        ), g_processes.end()
    );
}
}

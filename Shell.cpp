#include <fstream>
#include <iostream>
#include <string>
#include <vector>

#include <fcntl.h>
#include <pwd.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include "Shell.h"
#include "Terminal.h"

namespace BShell {
std::string g_prev_wd = "";
std::vector<Process> g_processes;
int g_exit_fg = 0;
int g_exit_bg = 0;

bool file$access(std::string path, int tests) {
    return access(path.c_str(), tests) != -1;
}

bool file$is_executable(std::string path) {
    return file$access(path, X_OK);
}

bool file$is_readable(std::string path) {
    return file$access(path, R_OK);
}

bool file$is_writable(std::string path) {
    return file$access(path, W_OK);
}

std::string get$cwd() {
    // From getcwd(3) it says get_current_dir_name() will malloc a
    // string large enough to fit the current working dir.
    // While getcwd() requires us to create our own string buffer.
    auto*buf = get_current_dir_name();
    auto cwd = std::string(buf);

    free(buf);

    return cwd;
}

std::string get$home() {
    // Use getenv "HOME" otherwise, fallback to getpwuid(3)
    // and getuid(3) to get the user's home directory.

    auto*buf = getenv("HOME");

    if (buf == nullptr)
        buf = getpwuid(getuid())->pw_dir;

    return std::string(buf);
}

std::string get$username() {
    // getlogin(3)
    // Suggests that we use getenv to get "LOGNAME", otherwise
    // fallback to getlogin(), then getlogin_r()

    auto*buf = getenv("LOGNAME");
    auto username = std::string {};
    auto del = false;

    if (buf == nullptr && (buf = getlogin()) == nullptr){
        // From useradd(8), usernames can be at most 32 chars long.
        buf = new char [32];

        getlogin_r(buf, 32);

        del = true;
    }

    username = std::string(buf);

    if (del)
        delete[] buf;

    return username;
}

std::string get$hostname() {
    auto buf = getenv("HOSTNAME");
    auto hostname = std::string {};

    if (buf != nullptr) hostname = std::string(buf);
    else {
        // hostname(7) states that the maximum hostname is 253 chars.
        buf = new char [253];
        gethostname(buf, 253);

        hostname = std::string(buf);
        delete[] buf;
    }

    return hostname;
}

std::string get$pname(pid_t pid) {
    auto proc = std::ifstream("/proc/" + std::to_string(pid) + "/cmdline");

    return std::string { std::istreambuf_iterator { proc.rdbuf() }, {} };
}

std::string get$pname(Expression* expr) {
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

        if (file$is_executable(tmp)) {
            path = tmp;
            break;
        }
    }

    return path;
}

int get$file(std::string path) {

}

std::string get$eval(Token eval) {
    return std::string {};
}

void handle$cd(std::string dir) {
    auto* dirc = dir.c_str();
    auto cwd = get$cwd();
    auto stat = 0;

    if (!dir.size())
        stat = chdir(get$home().c_str());
    else if (dir[0] == '~') {
        auto home = get$home();
        auto* path = new char [strlen(dirc) + home.size()];

        strcpy(path, home.c_str());
        strcpy(path + home.size(), dirc + 1);

        stat = chdir(path);

        delete[] path;
    } else if (dir == "-") {
        if (g_prev_wd.size()) {
            stat = chdir(g_prev_wd.c_str());
            std::cout << g_prev_wd << '\n';
        } else std::cerr << "g_prev_wd not set\n";
    } else stat = chdir(dirc);

    // Don't change previous directory on error.
    if (stat < 0)
        std::cerr << "Failed to change directory\n";
    else
        g_prev_wd = cwd;
}

void handle$redirect_in(Expression* redirect_in) {
    if (redirect_in == nullptr) return;

    auto filename = redirect_in->token.type == String ?
        redirect_in->token.content : get$eval(redirect_in->token);
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

void handle$redirect_out(Expression* redirect_out) {
    if (redirect_out == nullptr) return;

    auto filename = redirect_out->token.type == String ?
        redirect_out->token.content : get$eval(redirect_out->token);
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

void handle$argv_sticky(std::vector<char*>& argv, bool& sticky) {

}

std::vector<char*> handle$argv(Expression* expr) {
    auto argv = std::vector<char*>{ const_cast<char*>(expr->token.content.c_str()) };
    auto sticky = false;

    auto handle_strings = [&](std::string str) -> void {
        if (sticky) {
            auto prev = std::string(argv.back());
            sticky = false;

            str = prev + str;
            argv.pop_back();
        }

        argv.push_back(const_cast<char*>(str.c_str()));
    };

    for (auto& child : expr->children) {
        switch (child->token.type) {
            case StickyRight:
                sticky = true;
                argv.push_back(const_cast<char*>(child->token.content.c_str()));
                break;
            case String:
                handle_strings(child->token.content);
                break;
            case Eval:
                handle_strings(get$eval(child->token));
                break;
            case RedirectIn:
                handle$redirect_in(child->children[0]);
                break;
            case RedirectOut:
                handle$redirect_out(child->children[0]);
                break;
        }
    }

    argv.push_back(NULL);

    return argv;
}

template<typename T>
Process execute(Expression* expr, T&& child_hook) {
    auto pid = fork();

    if (pid < 0) {
        std::cerr << "Failed to fork()\n";

        exit(1);
    } else if (!pid) {
        auto argv = handle$argv(expr);

        child_hook(STDIN_FILENO, STDOUT_FILENO);

        execvp(argv[0], &argv[0]);

        std::cerr << "Failed to execvp()\n";
        exit(1);
    }

    return Process { pid, get$pname(expr) };
}

Process execute(Expression* expr) {
    return execute(expr, [=](int, int){});
}

void handle$keyword(Expression* expr) {
    auto kw = expr->token.content;
    auto index = std::distance(g_keywords.find(kw), g_keywords.end());
    auto args = expr->children.size() ? expr->children[0]->token.content : std::string {};

    // TODO: Maybe use an enum for more readability
    switch (index) {
        case 1: // export
            break;
        case 2: // cd
            handle$cd(args);
            break;
        case 3: // jobs
            break;
    }
}

void handle$executable(Expression* expr) {
    auto proc = execute(expr);

    waitpid(proc.pid, &g_exit_fg, 0);
}

void handle$background(Expression* expr) {
    auto proc = execute(expr->children[0]);

    std::cout << '[' << g_processes.size() + 1 << "] " << proc.pid << '\n';

    waitpid(proc.pid, &g_exit_bg, WNOHANG);
    g_processes.push_back(Process { proc.pid, proc.name });
}

void handle$pipe(Expression* expr) {
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
            [&](int fd_in, int fd_out) -> void {
                // This entire lambda function executes within the child process.
                if (child != expr->children.front()) {
                    dup2(last_io.fd[0], fd_in);

                    close(last_io.fd[0]);
                    close(last_io.fd[1]);
                }

                if (child != expr->children.back())
                    dup2(proc_io.fd[1], fd_out);

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
            std::cerr << "waitpid error\n";

            exit(1);
        }
    }
}

void handle$ast(Expression* ast) {
    switch (ast->token.type) {
        case Key:
            return handle$keyword(ast);
        case Executable:
            return handle$executable(ast);
        case Background:
            return handle$background(ast);
        case RedirectPipe:
            return handle$pipe(ast);
        default:
            std::cerr << "Unknown token type passed to handle$ast\n";
            // exit(1);
    }
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

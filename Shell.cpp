#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

#include <string.h>
#include <sys/wait.h>
#include <pwd.h>
#include <unistd.h>

#include "Shell.h"
#include "Terminal.h"

namespace BShell {
std::vector<Process> processes;
int fg_exit = 0;
int bg_exit = 0;

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
    auto ss = std::stringstream {};

    ss << proc.rdbuf();

    return ss.str();
}

std::string get$pname(Expression*expr) {
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

void handle$cd(const char*argc) {
    if (!strlen(argc)) {
        if (chdir(get$home().c_str()) == -1)
            std::cerr << "Failed to change directory\n";
    } else if (*argc == '~') {
        auto home = get$home();
        auto*path = new char [strlen(argc) + home.size()];

        strcpy(path, home.c_str());
        strcpy(path + home.size(), argc + 1);

        if (chdir(path) == -1)
            std::cerr << "Failed to change directory\n";

        delete[] path;
    } else if (chdir(argc) == -1)
        std::cerr << "Failed to change directory\n";
}

Process execute(Expression*expr, auto&&child_hook) {
    auto pid = fork();

    if (!pid) {
        auto cmd = expr->token.content.c_str();

        if (expr->children.size()) {
            auto children = expr->children;
            auto argc = children.size() + 2;
            char**argv = new char* [argc];
            argv[0] = const_cast<char*>(cmd);

            for (auto i = 0; i < children.size(); i++)
                argv[i + 1] = const_cast<char*>(children[i]->token.content.c_str());

            argv[argc - 1] = NULL;

            child_hook();

            execvp(argv[0], argv);
        }

        execlp(cmd, cmd, NULL);
    }

    return Process { pid, get$pname(expr) };
}

Process execute(Expression*expr) { return execute(expr, [=]{}); }

Process execute$pipe_out(Expression*expr) {
    auto pproc = Pipe {};

    pipe(pproc.stdout);

    auto proc = execute(expr, [=]() -> void {
        dup2(pproc.stdout[1], 1);
        close(pproc.stdout[0]);
    });

    proc.pipe = pproc;

    close(pproc.stdout[1]);

    return proc;
}

void handle$keyword(Expression*expr) { }

void handle$executable(Expression*expr) {
    auto proc = execute(expr);

    waitpid(proc.pid, &fg_exit, 0);
}

void handle$background(Expression*expr) {
    auto proc = execute(expr->children[0]);

    std::cout << '[' << processes.size() + 1 << "] " << proc.pid << '\n';

    waitpid(proc.pid, &bg_exit, WNOHANG);
    processes.push_back(Process { proc.pid, proc.name });
}

void handle$pipe(Expression*expr) { }

void handle$ast(Expression* ast) {
    switch (ast->token.type) {
        case Executable:
            return handle$executable(ast);
        case Background:
            return handle$background(ast);
        default:
            std::cerr << "Unknown token type passed to handle$ast\n";
            // exit(1);
    }
}

void edproc() {
    processes.erase(
        std::remove_if(
            processes.begin(), processes.end(),
            [=](const Process&proc) -> bool { return waitpid(proc.pid, NULL, WNOHANG) != 0; }
        ), processes.end()
    );
}
}

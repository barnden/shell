#pragma once

namespace BShell {
extern volatile bool g_fsigint;
void rl$reset();
void handler$readline_sig(void);
void handler$posix_sig(int);
}
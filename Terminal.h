#pragma once

namespace BShell {
void rl$reset();
void handler$readline_sig(void);
void handler$posix_sig(int);

extern volatile bool g_fsigint;
}
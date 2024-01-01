module;

#include "windows.h"

#include <cassert>

export module platform.win64;

import nstd.bits;
import nstd.strings;
import nstd.numbers;

export namespace error {

int32 get_code() { return GetLastError(); }
void set_code(int32 code) { assert(nstd::is_bit_set<29>(code)); SetLastError(code); }

string get_message()
{
    auto code = get_code();
    LPSTR buffer = NULL;
    auto result = FormatMessageA(
        FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
        NULL,
        code,
        LANG_USER_DEFAULT,
        (LPSTR)&buffer,
        0,
        NULL);

    string out(buffer, result);
    LocalFree(buffer);
    return out;
}

} // export namespace error

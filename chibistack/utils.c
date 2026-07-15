// Copyright (c) 2026 Yuumei-02. All Rights Reserved.
// See the LICENSE file for more information.

#include <mcu/core.h>

char sv_tmp_nullify(StringView sv) {
   char tmp = sv.chars[sv.length];
   sv.chars[sv.length] = '\0';
   return tmp;
}

void sv_tmp_restore(StringView sv, char tmp) {
   sv.chars[sv.length] = tmp;
}


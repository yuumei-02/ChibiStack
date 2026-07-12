// Copyright (c) 2026 yuumei-02. All Rights Reserved.
// See the LICENSE file for more information.

#include <mcu/core.h>
#include <mcu/io.h>
#include <mcu/containers.h>

#include "lexer.h"
#include "ir.h"
#include "x86_codegen.h"

void nasm_from_ir(IR* ir, bool asm_dump) {
   unused asm_dump;

   mcu_assert(ir != nullptr, "ir can't be null");
}


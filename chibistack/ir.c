// Copyright (c) 2026 yuumei-02. All Rights Reserved.
// See the LICENSE file for more information.

#include <mcu/core.h>
#include <mcu/io.h>
#include <mcu/containers.h>

#include "lexer.h"
#include "ir.h"

const cstr IrInstrKind_to_cstr(IrInstrKind self);

IR IR_from_token_stream(Lexer* lexer);
void IR_delete(IR* self);

void IR_dump(IR* self);


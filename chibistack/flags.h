// Copyright (c) 2026 yuumei-02. All Rights Reserved.
// See the LICENSE file for more information.

#pragma once

typedef struct {
   bool token_dump;
   bool ir_dump;
   bool asm_dump;
} CompileFlags;

CompileFlags CompileFlags_default();


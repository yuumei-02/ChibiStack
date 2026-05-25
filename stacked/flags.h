#pragma once

typedef struct {
   bool token_dump;
   bool lir_dump;
} CompileFlags;

CompileFlags CompileFlags_default() {
   return (CompileFlags) {0};
}


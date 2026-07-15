// Copyright (c) 2026 Yuumei-02. All Rights Reserved.
// See the LICENSE file for more information.

#include <mcu/core.h>
#include <mcu/io.h>

#include "lexer.h"
#include "ir.h"
#include "reporter.h"

void report_non_existant_word(Lexer* lexer, Token token) {
   Loc loc = Lexer_loc_from_offset(lexer, token.z);
   eprintln("%s:%u:%u: error: Word \"%.*s\" does not exist",
      lexer->file_path, loc.y, loc.x,
      (i32) token.str_view.length, token.str_view.chars);
}

void report_unexpected_token_expected(Lexer* lexer, Token token, TokenType expected) {
   Loc loc = Lexer_loc_from_offset(lexer, token.z);
   eprintln("%s:%u:%u: error: Unexpected token \"%s\", expected \"%s\"",
      lexer->file_path, loc.y, loc.x,
      TokenType_to_cstr(token.type),
      TokenType_to_cstr(expected));
}

void report_unexpected_token(Lexer* lexer, Token token) {
   Loc loc = Lexer_loc_from_offset(lexer, token.z);

   eprintln("%s:%u:%u: error: Unexpected token \"%s\"",
      lexer->file_path, loc.y, loc.x,
      TokenType_to_cstr(token.type));
}

void report_not_enough_stack_elements(const cstr for_instr, Lexer* lexer, u32 offset) {
   Loc loc = Lexer_loc_from_offset(lexer, offset);
   eprintln("%s:%u:%u: error: Not enough items on the stack for operation \"%s\"",
      lexer->file_path, loc.y, loc.x, for_instr);
}

void report_unexpected_type(const cstr got, const cstr expected, Lexer* lexer, u32 offset) {
   Loc loc = Lexer_loc_from_offset(lexer, offset);
   eprintln("%s:%u:%u: error: Expected Type \"%s\" got \"%s\"",
      lexer->file_path, loc.y, loc.x, expected, got);
}

void report_incompatible_binop_types(IrInstrKind operator, const cstr left, const cstr right, Lexer* lexer, u32 offset) {
   Loc loc = Lexer_loc_from_offset(lexer, offset);
   eprintln("%s:%u:%u: error: Type \"%s\" and \"%s\" are not compatible for operator \"%s\"",
      lexer->file_path, loc.y, loc.x,
      left, right, IrInstrKind_to_cstr(operator));
}

void report_unhandled_stack_items(i32 amount, Lexer* lexer, u32 offset) {
   Loc loc = Lexer_loc_from_offset(lexer, offset);
   eprintln("%s:%u:%u: error: %d item(s) of unhandled data on the stack",
      lexer->file_path, loc.y, loc.x, amount);
}


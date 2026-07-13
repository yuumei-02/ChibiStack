// Copyright (c) 2026 Yuumei-02. All Rights Reserved.
// See the LICENSE file for more information.

#pragma once

void report_unexpected_token(Lexer* lexer, Token token);
void report_unexpected_token_expected(Lexer* lexer, Token token, TokenType expected);

void report_not_enough_stack_elements(const cstr for_instr, Lexer* lexer, u32 offset);
void report_unexpected_type(const cstr got, const cstr expected, Lexer* lexer, u32 offset);
void report_incompatible_binop_types(IrInstrKind operator, const cstr left, const cstr right, Lexer* lexer, u32 offset);
void report_unhandled_stack_items(i32 amount, Lexer* lexer, u32 offset);

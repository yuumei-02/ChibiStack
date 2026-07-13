// Copyright (c) 2026 Yuumei-02. All Rights Reserved.
// See the LICENSE file for more information.

#include <mcu/core.h>
#include <mcu/containers.h>
#include <mcu/memory.h>

#include "timing.h"
#include "lexer.h"
#include "ir.h"

HashMap_impl(Type)

static u32 next_type_id = 0;

static void create_build_in_types(IR* ir) {
   HashMap_put(Type)(&ir->type_table, "int",  (Type) { .type_id = next_type_id++ });
   HashMap_put(Type)(&ir->type_table, "uint", (Type) { .type_id = next_type_id++ });
   HashMap_put(Type)(&ir->type_table, "ptr",  (Type) { .type_id = next_type_id++ });
}

bool validate_program(IR* ir, double* semantic_analysis_time) {
   mcu_assert(ir != nullptr, "ir can't be null");
   mcu_assert(semantic_analysis_time != nullptr, "semantic_analysis_time can't be null");

   struct timespec timer;
   clock_start(&timer);

   create_build_in_types(ir);

   *semantic_analysis_time = clock_end(&timer);
   return false;
}


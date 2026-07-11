// Copyright (c) 2026 yuumei-02. All Rights Reserved.
// See the LICENSE file for more information.

#define VMAKE_IMPL
#include <vmake.h>

i32 main(i32 argc, cstr argv[]) {
   Vmake_go_rebuild_yourself(argc, argv);

   ModuleId stacked = Module_new("stacked", "./stacked", MT_Executable);
   BuildOptions build_options = BuildOptions_default_debug();

   return Vmake_build(stacked, build_options);
}


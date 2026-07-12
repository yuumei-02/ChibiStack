// Copyright (c) 2026 yuumei-02. All Rights Reserved.
// See the LICENSE file for more information.

#define VMAKE_IMPL
#include <vmake.h>

i32 main(i32 argc, cstr argv[]) {
   Vmake_go_rebuild_yourself(argc, argv);

   ModuleId chibistack = Module_new("chibistack", "./chibistack", MT_Executable);
   BuildOptions build_options = BuildOptions_default_debug();

   return Vmake_build(chibistack, build_options);
}


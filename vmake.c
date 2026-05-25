#define VMAKE_IMPL
#include <vmake.h>

Vmake vmake;

i32 main(i32 argc, cstr argv[]) {
   vmake = Vmake_go_rebuild_yourself(argc, argv);
   
   ModuleId stacked = Module_new("stacked", "./stacked", MT_Executable);
   BuildOptions build_options;

   if (strcmp(argv[1], "--release") == 0)
      build_options = BuildOptions_default_release();
   else
      build_options = BuildOptions_default_debug();

   return Vmake_build(stacked, build_options);
}


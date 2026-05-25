#include <mcu/core.h>
#include <mcu/handlers.h>
#include <mcu/io.h>

i32 compile(const cstr file_path) {
   return 0;
}

i32 main(i32 argc, cstr argv[]) {
   if (argc < 2) {
      eprintln("[!] Missing input files");
      return 1;
   }

   for (i32 i = 1; i < argc; ++i) {
      if (compile(argv[i])) return 1;
   }

   return 0;
}


file(REMOVE_RECURSE
  "bootloader/bootloader.bin"
  "bootloader/bootloader.elf"
  "bootloader/bootloader.map"
  "config/sdkconfig.cmake"
  "config/sdkconfig.h"
  "flash_project_args"
  "ftm.bin"
  "ftm.map"
  "project_elf_src_esp32.c"
  "CMakeFiles/ftm.elf.dir/project_elf_src_esp32.c.obj"
  "CMakeFiles/ftm.elf.dir/project_elf_src_esp32.c.obj.d"
  "ftm.elf"
  "ftm.elf.pdb"
  "project_elf_src_esp32.c"
)

# Per-language clean rules from dependency scanning.
foreach(lang C)
  include(CMakeFiles/ftm.elf.dir/cmake_clean_${lang}.cmake OPTIONAL)
endforeach()

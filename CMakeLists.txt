project(odd-emu)

add_library(emu8051
  emu8051/core.c
  emu8051/disasm.c
  #emu8051/emu.c
  #emu8051/logicboard.c
  #emu8051/mainview.c
  #emu8051/memeditor.c
  emu8051/opcodes.c
  #emu8051/options.c
  #emu8051/popups.c
  emu8051/emu8051.h
  emu8051/emulator.h
)
target_link_libraries(emu8051)

add_executable(emu
  main.c
)
target_link_libraries(emu emu8051)

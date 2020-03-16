//FIXME: Should add https://github.com/libfuse/libfuse/blob/master/example/cuse.c
//       Could emulate an SCSI device; ideally it would emulate a lower level device only (ATA?)
//       https://www.kernel.org/doc/html/latest/driver-api/scsi.html useful?

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#include <assert.h>
#include <stdint.h>

#include <ctype.h>

#include <arpa/inet.h>

#include "emu8051/emu8051.h"
#include "emu8051/emulator.h"


#define CURRENT_REGISTER_BANK() ((emu->mSFR[REG_PSW] & (PSWMASK_RS0|PSWMASK_RS1))>>PSW_RS0)
#define RX(i) emu->mLowerData[i + 8 * CURRENT_REGISTER_BANK()]



uint8_t scsi_buffer[100];
int scsi_count = 0;

// 512k of RAM that is accessible through debug command FF 05 ...
// Also used for many things starting at base address 0x7BD00 (what JayFoxRox usually calls the "master")
uint8_t ram[0x80000];

uint8_t flash[0x20000];

uint8_t ram8000[0xF0]; //FIXME: How large is the RAM?

static void load_bank(struct em8051* emu) {
  int bank_index = (emu->mSFR[REG_P1] >> 0) & 1;
  memcpy(emu->mCodeMem, &flash[0x10000 * bank_index], 0x10000);
}
  
#if 0
int getregoutput(struct em8051* emu, int pos) {
    int rx = 8 * ((emu->mSFR[REG_PSW] & (PSWMASK_RS0|PSWMASK_RS1))>>PSW_RS0);
    switch (pos) {
    case 0:  return emu->mSFR[REG_ACC];
    case 1:  return emu->mLowerData[rx + 0];
    case 2:  return emu->mLowerData[rx + 1];
    case 3:  return emu->mLowerData[rx + 2];
    case 4:  return emu->mLowerData[rx + 3];
    case 5:  return emu->mLowerData[rx + 4];
    case 6:  return emu->mLowerData[rx + 5];
    case 7:  return emu->mLowerData[rx + 6];
    case 8:  return emu->mLowerData[rx + 7];
    case 9:  return emu->mSFR[REG_B];
    case 10: return emu->mSFR[REG_DPH] << 8 | emu->mSFR[REG_DPL];
    default:
      assert(false);
      break;
    }
    return 0;
}

void setregoutput(struct em8051* emu, int pos, int val) {
    int rx = 8 * ((emu->mSFR[REG_PSW] & (PSWMASK_RS0|PSWMASK_RS1))>>PSW_RS0);
    switch (pos) {
    case 0: emu->mSFR[REG_ACC] = val; break;
    case 1: emu->mLowerData[rx + 0] = val; break;
    case 2: emu->mLowerData[rx + 1] = val; break;
    case 3: emu->mLowerData[rx + 2] = val; break;
    case 4: emu->mLowerData[rx + 3] = val; break;
    case 5: emu->mLowerData[rx + 4] = val; break;
    case 6: emu->mLowerData[rx + 5] = val; break;
    case 7: emu->mLowerData[rx + 6] = val; break;
    case 8: emu->mLowerData[rx + 7] = val; break;
    case 9: emu->mSFR[REG_B] = val; break;
    case 10:
      emu->mSFR[REG_DPH] = (val >> 8) & 0xff;
      emu->mSFR[REG_DPL] = val & 0xff;
      break;
    default:
      assert(false);
      break;
    }
}
#endif

void sfrwrite(struct em8051* emu, int aRegister) {
  aRegister -= 0x80;
 
  uint8_t v = emu->mSFR[aRegister];

  switch(aRegister) {
  case REG_P0:
    // P0 is unused
    assert(false);
    break;
  case REG_P1: {

    // P1.0 = flash bank [for code]
    // P1.1 = ???
    // P1.2 = ???
    // P1.3 = ???
    // P1.4 = ???
    // P1.5 = connect address bus to flash, instead of EXTMEM?
    //        the flasher is seemingly copying itself to RAM or something by moving from 0xE800 to 0xE800.
    //        so it is possible that the memory map will be split here?
    // P1.6 = zero during flashing, 1 otherwise (flash write-enable maybe?)
    // P1.7 = ???

    unsigned int unknown_bits = 0;
    unknown_bits |= 1 << 1;
    unknown_bits |= 1 << 2;
    unknown_bits |= 1 << 3;
    unknown_bits |= 1 << 4;
    unknown_bits |= 1 << 7;

#if 0
    printf("P1.0 = %d (0x%04X)\n", (v >> 0) & 1, v);
#endif

    assert((v & unknown_bits) == unknown_bits);


    load_bank(emu);
    break;
  }
  case REG_P2:
    // P2 is unused
    assert(false);
    break;
  case REG_P3:
    // Only T0 is used
    assert(v & ~(1 << 4) == ~(1 << 4));
    break;
  }
}

int emu_sfrread(struct em8051* emu, int aRegister) {
  return emu->mSFR[aRegister - 0x80];
}

void emu_exception(struct em8051* emu, int aCode) {
#if 0
    WINDOW * exc;

    switch (aCode)
    {
    case EXCEPTION_IRET_SP_MISMATCH:
        if (opt_exception_iret_sp) return;
        break;
    case EXCEPTION_IRET_ACC_MISMATCH:
        if (opt_exception_iret_acc) return;
        break;
    case EXCEPTION_IRET_PSW_MISMATCH:
        if (opt_exception_iret_psw) return;
        break;
    case EXCEPTION_ACC_TO_A:
        if (!opt_exception_acc_to_a) return;
        break;
    case EXCEPTION_STACK:
        if (!opt_exception_stack) return;
        break;
    case EXCEPTION_ILLEGAL_OPCODE:
        if (!opt_exception_invalid) return;
        break;
    }

    nocbreak();
    cbreak();
    nodelay(stdscr, FALSE);
    halfdelay(1);
    while (getch() > 0) {}


    runmode = 0;
    setSpeed(speed, runmode);
    exc = subwin(stdscr, 7, 50, (LINES-6)/2, (COLS-50)/2);
    wattron(exc,A_REVERSE);
    werase(exc);
    box(exc,ACS_VLINE,ACS_HLINE);
    mvwaddstr(exc, 0, 2, "Exception");
    wattroff(exc,A_REVERSE);
    wmove(exc, 2, 2);

    switch (aCode)
    {
    case -1: waddstr(exc,"Breakpoint reached");
             break;
    case EXCEPTION_STACK: waddstr(exc,"SP exception: stack address > 127");
                          wmove(exc, 3, 2);
                          waddstr(exc,"with no upper memory, or SP roll over."); 
                          break;
    case EXCEPTION_ACC_TO_A: waddstr(exc,"Invalid operation: acc-to-a move operation"); 
                             break;
    case EXCEPTION_IRET_PSW_MISMATCH: waddstr(exc,"PSW not preserved over interrupt call"); 
                                      break;
    case EXCEPTION_IRET_SP_MISMATCH: waddstr(exc,"SP not preserved over interrupt call"); 
                                     break;
    case EXCEPTION_IRET_ACC_MISMATCH: waddstr(exc,"ACC not preserved over interrupt call"); 
                                     break;
    case EXCEPTION_ILLEGAL_OPCODE: waddstr(exc,"Invalid opcode: 0xA5 encountered"); 
                                   break;
    default:
        waddstr(exc,"Unknown exception"); 
    }
    wmove(exc, 6, 12);
    wattron(exc,A_REVERSE);
    waddstr(exc, "Press any key to continue");
    wattroff(exc,A_REVERSE);

    wrefresh(exc);

    getch();
    delwin(exc);
    change_view(emu, MAIN_VIEW);
#endif
  assert(false);
}

unsigned int read8(uint8_t* data) {
  return (data[0] << 0);
}
void write8(uint8_t* data, unsigned int value) {
  data[0] = (value >> 0) & 0xFF;
}

unsigned int read16(uint8_t* data) {
  return (data[0] << 8) |
         (data[1] << 0);
}
void write16(uint8_t* data, unsigned int value) {
  data[0] = (value >> 8) & 0xFF;
  data[1] = (value >> 0) & 0xFF;
}

unsigned int read24(uint8_t* data) {
  return (data[0] << 16) |
         (data[1] <<  8) |
         (data[2] <<  0);
}
void write24(uint8_t* data, unsigned int value) {
  data[0] = (value >> 16) & 0xFF;
  data[1] = (value >>  8) & 0xFF;
  data[2] = (value >>  0) & 0xFF;
}





#include "flash.c"
#include "chipset.c"
#include "savestate.c"
#include "commands.c"


int main(int argc, char* argv[]) {

  // Load the firmware
  //FIXME: Turn into a command
  assert(argc > 1);
  FILE* f = fopen(argv[1], "rb");
  size_t ret = fread(flash, 1, sizeof(flash), f);
  assert(ret == sizeof(flash));
  fclose(f);

  // Create the virtual CPU
  struct em8051 _emu;
  memset(&_emu, 0, sizeof(_emu));

  // Set up virtual CPU
  struct em8051* emu = &_emu;
  emu->mCodeMem     = malloc(0x10000);
  emu->mCodeMemSize = 0x10000;
  emu->mExtData     = malloc(0x10000);
  emu->mExtDataSize = 0x10000;
  emu->mLowerData   = malloc(0x100);
  emu->mUpperData   = &emu->mLowerData[0x80];
  emu->mSFR         = malloc(0x80);
  emu->except       = emu_exception;
  emu->sfrread      = emu_sfrread;
  emu->sfrwrite = sfrwrite;
  emu->xread = xread;
  emu->xwrite = xwrite;

  // Reset virtual CPU
  reset(emu, 1);

  // Load bank 0 of firmware to memory
  load_bank(emu);

  // Process commands from command line
  for(int i = 2; i < argc; i++) {
    add_command(argv[i]);
  }

  // Enable pausing of emulator  
  install_pause_handler();


  // Start mainloop
  char* previous_command_line = strdup("");
  while(true) {

    // Check if we have to enter the shell
    if (requested_pause) {

      // Only handle pause if we no longer have anything queued
      if (command_count == 0) {

        printf("> ");
        char command_line[200];
        fgets(command_line, sizeof(command_line), stdin);

        char* newline = strchr(command_line, '\n');
        if (newline == command_line) {
          strcpy(command_line, previous_command_line);
        } else {
          *newline = '\0';

          free(previous_command_line);
          previous_command_line = strdup(command_line);
        }

        //FIXME: Split line by ;
        add_command(command_line);
      }

    }

    // Process all queued commands until we are supposed to run
    while(command_count > 0) {
      char* command_line = commands[0];
      printf("Command '%s'\n", command_line);
      bool run = handle_command(emu, command_line);
      delete_command();
      if (run) {
        requested_pause = false;
        break;
      }
    }

    // Run the CPU
    unsigned long long n = 1;
    while(!requested_pause) {

      // Use step counter
      if (steps != (unsigned int)-1) {
        steps--;
        if (steps == 0) {
          requested_pause = true;
        }
      }

      int ticked = tick(emu);
      unsigned int bank_index = emu->mSFR[REG_P1] & 1;

      bool info = trace;

      if (bank_index == 0) {
        switch(emu->mPC) {
        case 0x27e2:
          printf("Setting sense: SK=%02X ASQ=%02X ASQC=%02X\n", RX(7), RX(5), RX(3));
          //assert(false);
          break;
        case 0x23bb:
          printf("Trying to find ATAPI handler\n");
          break;
        }
      }

      if (bank_index == 1) {
        switch(emu->mPC) {

        case 0x2394 ... 0x23C9:
          // This is a rather tight loop that takes a while
          info = false;
          break;

        }
      }

      if(info) {
        printf("bank %d: 0x%04X", bank_index, emu->mPC);
        printf(" R6: 0x%02X", RX(6));
        printf(" R4R5: 0x%02X%02X", RX(4), RX(5));
        printf(" interrupt-active:%d", emu->mInterruptActive);
        printf(" IE.EA:%d", emu->mSFR[REG_IE] & IEMASK_EA);
        printf(" IE.EX1:%d", emu->mSFR[REG_IE] & IEMASK_EX1);
        printf(" TCON.IE1:%d", emu->mSFR[REG_TCON] & TCONMASK_IE1);
        printf(" IP.PX1:%d", emu->mSFR[REG_IP] & IPMASK_PX1);
        printf(" P1.5:%d", (emu->mSFR[REG_P1] >> 5) & 1);
        printf(" P1.6:%d", (emu->mSFR[REG_P1] >> 6) & 1);
        printf(" flash-state:%d/%d", flash_state, flash_chip_id);
        printf(" SCSI:");
        for(int i = 0; i < scsi_count; i++) {
          printf("%02X", scsi_buffer[i]);
        }
        printf(" %llu", n);
        printf("\n");
      }

      n++;

    }

  }

  return 0;
}

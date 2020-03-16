static bool trace = false;
static unsigned int steps = (unsigned int)-1;
static volatile bool requested_pause = false;

static void request_pause() {
  requested_pause = true;
}

#if defined(__linux__)

#include <signal.h>

static void sigint_handler(int n) {
  request_pause();
}

static void install_pause_handler() {
  signal(SIGINT, sigint_handler);
}

#else

#warning Pause handler not specified for this platform

static void install_pause_handler() {
  printf("Unable to install pause-handler");
}v

#endif

static unsigned int parse_hex(const char* data) {
  unsigned int value = 0;
  const char* cursor = data;
  while(*cursor != '\0') {
    value *= 0x10;
    char digit = tolower(*cursor);
    if (isdigit(digit)) {
      value += 0x0 + (digit - '0');
    } else if (isalpha(digit)) {
      if (digit < 'a') { return 0; }
      if (digit > 'f') { return 0; }
      value += 0xA + (digit - 'a');
    } else {
      return 0;
    }
    cursor++;
  }
  return value;
}

//FIXME: !!!
#define parse_bin parse_hex

static unsigned int parse_dec(const char* data) {
  unsigned int value = 0;
  const char* cursor = data;
  while(*cursor != '\0') {
    value *= 10;
    char digit = tolower(*cursor);
    if (isdigit(digit)) {
      value += 0 + (digit - '0');
    } else {
      return 0;
    }
    cursor++;
  }
  return value;
}


// This should be used in the future so we can have spaces in filenames
static const char* parse_string() {
  assert(false);
}

unsigned int command_count = 0;
char** commands = NULL;

static void add_command(const char* command) {
  command_count++;
  commands = realloc(commands, sizeof(commands[0]) * command_count);
  commands[command_count - 1] = strdup(command);
}

static void delete_command() {
  command_count--;
  free(commands[0]);
  memmove(&commands[0], &commands[1], sizeof(commands[0]) * command_count);
  commands = realloc(commands, sizeof(commands[0]) * command_count);
}

//FIXME: Needs major refactors
static bool handle_command(struct em8051* emu, char* command_line) {
#if 0
  printf("Parsing '%s'\n", command_line);
#endif

  char* command = strtok(command_line, " ");

  if (command == NULL) {
    return false;
  }

  if (!strcmp(command, "help")) {
    //FIXME: implement
    printf("The help command is not supported yet.\n");
  } else if (!strcmp(command, "pause") || !strcmp(command, "p")) {
    // This one is meant to be used via the CLI
    //FIXME: Duplicate of "step 0"?
    requested_pause = true;
  } else if (!strcmp(command, "continue") || !strcmp(command, "c")) {
    steps = (unsigned int)-1;
    return true;
  } else if (!strcmp(command, "step") || !strcmp(command, "s")) {
    char* argument = strtok(NULL, " ");
    if (argument != NULL) {
      steps = parse_dec(argument);
    } else {
      steps = 1;
    }
    return steps > 0;
  } else if (!strcmp(command, "quit") || !strcmp(command, "q")) {
    exit(0);
  } else if (!strcmp(command, "save")) {
    char* path = strtok(NULL, " ");
    printf("Saving to '%s'\n", path);

    transfer_state(false, path, emu);

  } else if (!strcmp(command, "load")) {
    char* path = strtok(NULL, " ");
    printf("Loading from '%s'\n", path);

    transfer_state(true, path, emu);

  } else if (!strcmp(command, "trace")) {
    trace = true;

  } else if (!strcmp(command, "scsi")) {
    char* data = strtok(NULL, " ");

    char* cursor = data;
    while(*cursor != '\0') {
      char byte[3];
      memcpy(byte, cursor, 2);
      byte[2] = '\0';
      unsigned int value = parse_hex(byte);
      scsi_buffer[scsi_count] = value;
      scsi_count++;
      cursor += 2;
    }

  } else if (!strcmp(command, "set")) {

    // Parse arguments
    while(true) {

      // Find the next reg=data
      char* state = strtok(NULL, " ");
      if (state == NULL) {
        break;
      }

      char* reg = state;
      char* equals = strchr(reg, '=');
      *equals = '\0';
      char* data = equals + 1;

      // Register names are case insensitive
      char* cursor = reg;
      while(*cursor != '\0') {
        *cursor = tolower(*cursor);
        cursor++;
      }

      printf("Setting '%s' to '%s'\n", reg, data);

      if (!strcmp(reg, "pc")) {
        unsigned int value = parse_hex(data);
        printf("PC=%04X\n", value);
        emu->mPC = value;
      //FIXME: A,B {AB}, DPL,DPH {DPTR}
      } else if (reg[0] == 'r') {
        // R0 - R7 or R0@0 - R7@3
        char* at = strchr(reg + 1, '@');
        int bank;
        if (at != NULL) {
          *at = '\0';
          bank = parse_dec(at + 1);
        } else {
          bank = CURRENT_REGISTER_BANK();
        }
        int index = parse_dec(reg + 1);

        char* cursor = data;
        while(*cursor != '\0') {
          char byte[3];
          memcpy(byte, cursor, 2);
          byte[2] = '\0';
          unsigned int value = parse_hex(byte);
          printf("R%d@%d=%02X\n", index, bank, value);
          emu->mLowerData[index + 8 * bank] = value;
          index += 1;
          cursor += 2;
        }

      } else if (reg[0] == 'e') {
        int index = parse_hex(reg + 1);

        char* cursor = data;
        while(*cursor != '\0') {
          char byte[3];
          memcpy(byte, cursor, 2);
          byte[2] = '\0';
          unsigned int value = parse_hex(byte);
          printf("EXTMEM_%04X=%02X\n", index, value);
          emu->mExtData[index] = value;
          index += 1;
          cursor += 2;
        }

      } else if ((reg[0] == 'i') || (reg[0] == 's')) {
        char* dot = strchr(reg + 1, '.');
        if (dot != NULL) {
          *dot = '\0';
        }
        int index = parse_hex(reg + 1);
        assert(index < 0x100);
        if (dot == NULL) {

          char* cursor = data;
          while(*cursor != '\0') {
            char byte[3];
            memcpy(byte, cursor, 2);
            byte[2] = '\0';
            unsigned int value = parse_hex(byte);

            if (reg[0] == 'i') {
              printf("INTMEM_%02X=%02X\n", index, value);
              if (index < 0x80) {
                emu->mLowerData[index-0x00] = value;
              } else {
                emu->mUpperData[index-0x80] = value;
              }
            } else if (reg[0] == 's') {
              printf("SFR_%02X=%02X\n", index, value);
              assert(index >= 0x80);
              emu->mSFR[index-0x80] = value;
              sfrwrite(emu, index);
            } else {
              assert(false);
            }

            index += 1;
            cursor += 2;
          }

        } else {
          *dot = '\0';
          int bit = parse_dec(dot + 1);

          bool value = parse_bin(data);

          if (reg[0] == 'i') {
            printf("INTMEM_%02X.%d=%02X\n", index, bit, value);
            if (index < 0x80) {
              if (value) {
                emu->mLowerData[index-0x00] |= (1 << bit);
              } else {
                emu->mLowerData[index-0x00] &= ~(1 << bit);
              }
            } else {
              if (value) {
                emu->mUpperData[index-0x80] |= (1 << bit);
              } else {
                emu->mUpperData[index-0x80] &= ~(1 << bit);
              }
            }
          } else if (reg[0] == 's') {
            printf("SFR_%02X.%d=%02X\n", index, bit, value);
            assert(index >= 0x80);
            if (value) {
              emu->mSFR[index-0x80] |= (1 << bit);
            } else {
              emu->mSFR[index-0x80] &= ~(1 << bit);
            }
            sfrwrite(emu, index);
          } else {
            assert(false);
          }

        }        
      } else {
        printf("Unknown register '%s'\n", reg);
      }
    }

  } else {
    printf("Unknown command '%s'\n", command);
  }

  return false;
}

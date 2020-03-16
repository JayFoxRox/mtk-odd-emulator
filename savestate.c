
static void transfer_data(bool load, const char* path, const char* file, uint8_t* data, size_t size) {
  char path_buffer[1024];
  sprintf(path_buffer, "%s-%s", path, file);
  if (load) {
    FILE* f = fopen(path_buffer, "rb");
    fread(data, 1, size, f);
    fclose(f);
  } else {
    FILE* f = fopen(path_buffer, "wb");
    fwrite(data, 1, size, f);
    fclose(f);
  }
}

static void transfer_state(bool load, const char* path, struct em8051* emu) {
  transfer_data(load, path, "flash.bin", flash, sizeof(flash));
  transfer_data(load, path, "intmem.bin", emu->mLowerData, 0x100);
  transfer_data(load, path, "extmem.bin", emu->mExtData, 0x10000); // Mirror of MMIO
  transfer_data(load, path, "ram.bin", ram, sizeof(ram));
  transfer_data(load, path, "sfr.bin", emu->mSFR, 0x80);

  uint32_t cpu[] = {
    htonl(emu->mPC),
    htonl(emu->mTickDelay),
    htonl(emu->mInterruptActive),
    htonl(emu->int_a[0]), htonl(emu->int_a[1]),
    htonl(emu->int_psw[0]), htonl(emu->int_psw[1]),
    htonl(emu->int_sp[0]), htonl(emu->int_sp[1])
  };
  transfer_data(load, path, "cpu.bin", (uint8_t*)cpu, sizeof(cpu));
  if (load) {
    uint32_t* cursor = cpu;
    emu->mPC = ntohl(*cursor++);
    emu->mTickDelay = ntohl(*cursor++);
    emu->mInterruptActive = ntohl(*cursor++);
    emu->int_a[0] = ntohl(*cursor++); emu->int_a[1] = ntohl(*cursor++);
    emu->int_psw[0] = ntohl(*cursor++); emu->int_psw[1] = ntohl(*cursor++);
    emu->int_sp[0] = ntohl(*cursor++); emu->int_sp[1] = ntohl(*cursor++);
  }

  if (load) {

    // Trigger a bank switch
    emu->sfrwrite(emu, 0x80 + REG_P1);

  }
}


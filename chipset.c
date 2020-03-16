//FIXME: Remove? rarely used.. and kind of dumb
static void transfer(uint8_t* a, uint8_t* b, size_t size, bool b_to_a) {
  if (b_to_a) {
    memcpy(a, b, size);
  } else {
    memcpy(b, a, size);
  }
}

// Bus related things (SCSI, RAM, Flash, ..?)
static void chipset_4xxx(struct em8051* emu, unsigned int address, uint8_t* value, bool write) {
  switch(address) {

  case 0x4000:
    if (write) {
      // master status?
    } else {
      *value = 0x00;
    }
    break;

  case 0x4001:
    if (write) {
    } else {
      *value = 0x00;
    }
    break;
  case 0x4002:
    if (write) {
    } else {
      *value = 0x00;
    }
    break;

  // Write to RAM address?
  case 0x400C: {
    unsigned int address = read24(&emu->mExtData[0x4005]);
    if (address > sizeof(ram)) {
      printf("Trimming RAM address from 0x%X\n", address);
      address &= sizeof(ram) - 1;
      //FIXME: This will be written back
    }
    if (write) {
      printf("writing RAM at 0x%X to 0x%02X\n", address, *value);
      ram[address++] = *value;
    } else {
      *value = ram[address++];
      printf("Reading RAM at 0x%X -> 0x%02X\n", address, *value);
    }
    write24(&emu->mExtData[0x4005], address);
    break;
  }

  case 0x400e:
    if (write) {
    } else {
      // Some bitmask
      *value = 0x00;
    }
    break;

  case 0x400f:
    if (write) {
    } else {
      // Some extented interrupt mask
      // (1 << 2)=0 Required to process the interrupt we want
      // (1 << 4)=1 ATAPI or ATA packet - only some? (ATA if e8037=0x04, anything else ATAPI?!)
      // (1 << 5)=1 Required to process some interrupt we want
      // (1 << 6)=1 ATA packet from 0x4017 ???
      *value = (1 << 4) | (1 << 5) | (1 << 6);
    }
    break;

  // SCSI?
  case 0x4010: {
    if (write) {
      unsigned int count = read8(&emu->mExtData[0x4024]);
      printf("SCSI Write: %02X%s\n", *value, (count == 0) ? " (Ignored?)" : "");
      if (count > 0) {
        count--;      
      }
      write8(&emu->mExtData[0x4024], count);
      break;
    } else {
      printf("Reading from SCSI\n");
      if (scsi_count == 0) {
        printf("SCSI buffer underrun!\n");
        *value = 0x00;
        break;
      }
      *value = scsi_buffer[0];
      scsi_count--;
      memmove(&scsi_buffer[0], &scsi_buffer[1], scsi_count);
    }
    break;
  }

  case 0x4011:
    if (write) {
    } else {
      // Some bitmask? bit 1 is checked somewhere
      *value = 0x00;
    }
    break;

  case 0x4015:
    if (write) {
    } else {
      *value = 0x00;
    }
    break;

  case 0x4017:
    if (write) {
    } else {
      // Must be 0xA1 or 0xEF for the CDB handler to be invoked
      // These are actually ATA commands, and instead of looking at 9A, these seem to be handled then?
      *value = 0x00; //0xA1;
    }
    break;

  case 0x4020:
    if (write) {
    } else {
      // Some bitmask? bit 1 is checked somewhere
      *value = 0x00;
    }
    break;

  case 0x4028:
    if (write) {
    } else {
      *value = 0x00;
    }
    break;
  case 0x4029:
    if (write) {
    } else {
      *value = 0x00;
    }
    break;

  // Probably DMA controller to SCSI
  //FIXME: Probably not the trigger, because 79a4 writes 4034 last. Unless that causes 2 transfers?
  //FIXME: Might be handled by 29.6_dma_out_enable_maybe = 1; or something else in the code reading this flag later?
  case 0x4034:
  case 0x4035: {
    if (write) {
      unsigned int address = read24(&emu->mExtData[0x4031]);
      unsigned int count = read16(&emu->mExtData[0x4034]);
      if (address > sizeof(ram)) {
        printf("Trimming RAM address from 0x%X\n", address);
        address &= sizeof(ram) - 1;
      }
      printf("SCSI DMA Write (0x%05X, %d bytes): ", address, count);
      while(count > 0) {
        printf("%02X", ram[address]);
        address++;
        count--;
      }
      printf("\n");
      //FIXME: Update address?
      //FIXME: Not sure if the count is actually updated
      write16(&emu->mExtData[0x4034], count);
    }
    break;
  }

  case 0x403E:
    if (write) {
      // ???
    }
    break;

  case 0x404A:
    if (write) {
    } else {
      *value = 0x00;
    }
    break;

  // Interrupt reason
  case 0x4079:
    if (write) {
    } else {
      // Some bitmask?
      // (1 << 2) Use interrupt mask from 0x400F
      *value = (1 << 2);
    }
    break;

  case 0x40C1: {
    // Some bitmask?
    // Writing 2 seems to indicate that data at 40C9-40CF is ready
    // Reading 1 seems to signal that the transfer is still going?
    if (write) {
      assert(*value == 0x02);
      unsigned int a = read24(&emu->mExtData[0x409A]);
      unsigned int b = read24(&emu->mExtData[0x40CD]);
      printf("Unknown 0x40Cx transfer: 0x%06X 0x%06X\n", a, b);
    } else {
      *value = 0x00 & ~(1 << 1);
    }
    break;
  }

  case 0x40AC:
    if (write) {
      // Some bitmask
    } else {
      *value = 0x00;
    }
    break;
  }
}

// Direct access to some RAM
static bool chipset_8xxx(struct em8051* emu, unsigned int address, uint8_t* value, bool write) {
  transfer(&emu->mExtData[address], value, 1, write);
  return true;
}

// Probably MMIO for mechanical portion
static bool chipset_Cxxx(struct em8051* emu, unsigned int address, uint8_t* value, bool write) {
  switch(address) {

  case 0xC000:
    if (write) {
    } else {
      *value = 0x00;
    }
    return true;

  case 0xC001:
    if (write) {
    } else {
      *value = 0x00;
    }
    return true;

  case 0xC003: {
    if (write) {
      assert(false);
    } else {
      uint8_t v = 0;

      v |= (1 << 1); // FIXME: Must be set for loop at bank 1 0x9497
      v |= (1 << 5); // FIXME: Must be set for loop at bank 1 0x9707

      *value = v;
    }
    return true;
  }

  case 0xC002:
    if (write) {
    } else {
      *value = 0x00;
    }
    return true;

  case 0xC004: {
    if (write) {
    } else {
      uint8_t v = 0;

      v |= (1 << 4); // FIXME: Must be set for skipping function FUN_CODE_23e6

      *value = v;
    }
    return true;
  }

  case 0xC005:
    if (write) {
    } else {
      *value = 0x00;
    }
    return true;

  // This is set to 1 temporarily, then some work is done, and then it's set to 0 again
  case 0xC006: {
    if (write) {
      assert((*value == 0x00) || (*value == 0x01));
    } else {
      // Write only
      assert(false);
    }
    return true;
  }

  case 0xC020:
    if (write) {
    } else {
      *value = 0x00;
    }
    return true;

  case 0xC021:
    if (write) {
    } else {
      *value = 0x00;
    }
    return true;

  case 0xC022:
    if (write) {
    } else {
      *value = 0x00;
    }
    return true;

  case 0xC023:
    if (write) {
    } else {
      *value = 0x00;
    }
    return true;

  case 0xC024:
    if (write) {
    } else {
      *value = 0x00;
    }
    return true;

  case 0xC025:
    if (write) {
    } else {
      *value = 0x00;
    }
    return true;

  case 0xC026:
    if (write) {
    } else {
      *value = 0x00;
    }
    return true;

  case 0xC028:
    if (write) {
    } else {
      *value = 0x00;
    }
    return true;

  case 0xC029:
    if (write) {
    } else {
      *value = 0x00;
    }
    return true;

  case 0xC02A:
    if (write) {
    } else {
      *value = 0x00;
    }
    return true;

  case 0xC02B:
    if (write) {
    } else {
      *value = 0x00;
    }
    return true;

  case 0xC030:
    if (write) {
    } else {
      *value = 0x00;
    }
    return true;

  case 0xC031:
    if (write) {
    } else {
      *value = 0x00;
    }
    return true;

  case 0xC032:
    if (write) {
    } else {
      *value = 0x00;
    }
    return true;

  case 0xC034:
    if (write) {
    } else {
      *value = 0x00;
    }
    return true;

  case 0xC035:
    if (write) {
    } else {
      *value = 0x00;
    }
    return true;

  case 0xC036:
    if (write) {
    } else {
      *value = 0x00;
    }
    return true;

  case 0xC037:
    if (write) {
    } else {
      *value = 0x00;
    }
    return true;

  case 0xC038:
    if (write) {
    } else {
      *value = 0x00;
    }
    return true;

  case 0xC03A:
    if (write) {
    } else {
      *value = 0x00;
    }
    return true;

  case 0xC040:
    if (write) {
    } else {
      *value = 0x00;
    }
    return true;

  case 0xC041:
    if (write) {
    } else {
      *value = 0x00;
    }
    return true;

  case 0xC042:
    if (write) {
    } else {
      *value = 0x00;
    }
    return true;

  case 0xC043:
    if (write) {
    } else {
      *value = 0x00;
    }
    return true;

  case 0xC044:
    if (write) {
    } else {
      *value = 0x00;
    }
    return true;

  case 0xC047:
    if (write) {
    } else {
      *value = 0x00;
    }
    return true;

  case 0xC048:
    if (write) {
    } else {
      *value = 0x00;
    }
    return true;

  case 0xC049:
    if (write) {
    } else {
      *value = 0x00;
    }
    return true;

  case 0xC04A:
    if (write) {
    } else {
      *value = 0x00;
    }
    return true;

  case 0xC04B:
    if (write) {
    } else {
      *value = 0x00;
    }
    return true;

  case 0xC04D:
    if (write) {
    } else {
      *value = 0x00;
    }
    return true;

  case 0xC050:
    if (write) {
    } else {
      *value = 0x00;
    }
    return true;

  case 0xC051:
    if (write) {
    } else {
      *value = 0x00;
    }
    return true;

  case 0xC052:
    if (write) {
    } else {
      *value = 0x00;
    }
    return true;

  case 0xC053:
    if (write) {
    } else {
      *value = 0x00;
    }
    return true;

  case 0xC054:
    if (write) {
    } else {
      *value = 0x00;
    }
    return true;

  case 0xC055:
    if (write) {
    } else {
      *value = 0x00;
    }
    return true;

  case 0xC056:
    if (write) {
    } else {
      *value = 0x00;
    }
    return true;

  case 0xC057:
    if (write) {
    } else {
      *value = 0x00;
    }
    return true;

  case 0xC058:
    if (write) {
    } else {
      *value = 0x00;
    }
    return true;

  case 0xC059:
    if (write) {
    } else {
      *value = 0x00;
    }
    return true;

  case 0xC05A:
    if (write) {
    } else {
      *value = 0x00;
    }
    return true;

  case 0xC05B:
    if (write) {
    } else {
      *value = 0x00;
    }
    return true;

  case 0xC05C:
    if (write) {
    } else {
      *value = 0x00;
    }
    return true;

  case 0xC05D:
    if (write) {
    } else {
      *value = 0x00;
    }
    return true;

  case 0xC05E:
    if (write) {
    } else {
      *value = 0x00;
    }
    return true;

  case 0xC060:
    if (write) {
    } else {
      *value = 0x00;
    }
    return true;

  case 0xC061:
    if (write) {
    } else {
      *value = 0x00;
    }
    return true;

  case 0xC070:
    // Accessed when the boot is complete?
    // Some bitmask (set explicitly to have only 1 bit = 0x00 or 0x04; also OR'd and AND'd with 1?)
    if (write) {
    } else {
      *value = 0x00;
    }
    return true;

  case 0xC07C:
    if (write) {
    } else {
      *value = 0x00;
    }
    return true;

  case 0xC07E:
    if (write) {
    } else {
      *value = 0x00;
    }
    return true;

  case 0xC07F:
    if (write) {
    } else {
      *value = 0x00;
    }
    return true;

  case 0xC080:
    // Written to 9F when trying to close the tray
    if (write) {
    } else {
      *value = 0x00;
    }
    return true;

  case 0xC083:
    if (write) {
    } else {
      *value = 0x00;
    }
    return true;

  case 0xC084:
    if (write) {
    } else {
      *value = 0x00;
    }
    return true;

  case 0xC085:
    if (write) {
    } else {
      *value = 0x00;
    }
    return true;

  case 0xC091:
    if (write) {
    } else {
      *value = 0x00;
    }
    return true;

  case 0xC092:
    if (write) {
    } else {
      *value = 0x00;
    }
    return true;

  case 0xC093:
    if (write) {
    } else {
      *value = 0x00;
    }
    return true;

  case 0xC094:
    if (write) {
    } else {
      *value = 0x00;
    }
    return true;

  case 0xC096:
    if (write) {
    } else {
      *value = 0x00;
    }
    return true;

  case 0xC097:
    if (write) {
    } else {
      *value = 0x00;
    }
    return true;

  case 0xC099:
    if (write) {
    } else {
      *value = 0x00;
    }
    return true;

  case 0xC09B:
    if (write) {
    } else {
      *value = 0x00;
    }
    return true;

  case 0xC09C:
    if (write) {
    } else {
      *value = 0x00;
    }
    return true;

  case 0xC09D:
    if (write) {
    } else {
      *value = 0x00;
    }
    return true;

  case 0xC09E:
    if (write) {
    } else {
      *value = 0x00;
    }
    return true;

  case 0xC0A0:
    if (write) {
    } else {
      *value = 0x00;
    }
    return true;

  case 0xC0A8:
    if (write) {
    } else {
      *value = 0x00;
    }
    return true;

  case 0xC0AB:
    if (write) {
    } else {
      *value = 0x00;
    }
    return true;

  case 0xC0AC:
    if (write) {
    } else {
      *value = 0x00;
    }
    return true;

  case 0xC0AD:
    if (write) {
    } else {
      *value = 0x00;
    }
    return true;

  case 0xC0AE:
    if (write) {
    } else {
      *value = 0x00;
    }
    return true;

  case 0xC0D4:
    // Written to 04 when trying to close the tray
    if (write) {
    } else {
      *value = 0x00;
    }
    return true;

  case 0xC0D8:
    // Written to 00 when trying to close the tray
    if (write) {
    } else {
      *value = 0x00;
    }
    return true;

  case 0xC0D9:
    // Written to 00 when trying to close the tray
    if (write) {
    } else {
      *value = 0x00;
    }
    return true;

  case 0xC0DB:
    // Written to 00 when trying to close the tray
    if (write) {
    } else {
      *value = 0x00;
    }
    return true;

  case 0xC0DD:
    if (write) {
    } else {
      *value = 0x00;
    }
    return true;

  case 0xC0E0:
    if (write) {
    } else {
      *value = 0x00;
    }
    return true;

  case 0xC0E1:
    if (write) {
    } else {
      *value = 0x00;
    }
    return true;

  case 0xC0E2:
    if (write) {
    } else {
      *value = 0x00;
    }
    return true;

  case 0xC0E9:
    if (write) {
    } else {
      *value = 0x00;
    }
    return true;

  case 0xC0EA:
    if (write) {
    } else {
      *value = 0x00;
    }
    return true;

  case 0xC0EB:
    if (write) {
    } else {
      *value = 0x00;
    }
    return true;

  case 0xC0EE:
    if (write) {
    } else {
      *value = 0x00;
    }
    return true;

  case 0xC0EF:
    if (write) {
    } else {
      *value = 0x00;
    }
    return true;

  }
  return false;
}

static void handle_access(struct em8051* emu, unsigned int address, uint8_t* value, bool write) {
  bool handled;
  switch(address) {
  case 0x4000 ... 0x5000:
    chipset_4xxx(emu, address, value, write);
    handled = true; //FIXME
    break;
  case 0x8000 ... (0x8000+sizeof(ram8000)-1):
    //FIXME: ram8000[aAddress-0x8000];
    //       (not done because not part of savestate)
    handled = chipset_8xxx(emu, address, value, write);
    break;
  case 0xC000 ... 0xD000:
    handled = chipset_Cxxx(emu, address, value, write);
    break;
  default:
    handled = false;
    break;
  }
  assert(handled);
}

int xread(struct em8051* emu, int aAddress) {
  printf("Trying to read from 0x%04X\n", aAddress);

  // This seems to re-route access to the flash
  if (((emu->mSFR[REG_P1] >> 5) & 1) == 0) {
    return flash_read(emu, aAddress);
  }

  uint8_t value = emu->mExtData[aAddress];
  handle_access(emu, aAddress, &value, false);
  return value;
}


void xwrite(struct em8051* emu, int aAddress, int aValue) {
  printf("Trying to write 0x%02X to 0x%04X\n", aValue, aAddress);

  // This seems to re-route access to the flash
  if (((emu->mSFR[REG_P1] >> 5) & 1) == 0) {
    flash_write(emu, aAddress, aValue);
    return;
  }

  // Punch through
  //FIXME: Avoid a large allocation and only back actual registers
  emu->mExtData[aAddress] = aValue;

  assert(aValue >= 0x00);
  assert(aValue <= 0xFF);
  uint8_t value = aValue;
  handle_access(emu, aAddress, &value, true);
  assert(value == aValue);
}


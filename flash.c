typedef enum {
  FLASH_CYCLE1,
  FLASH_CYCLE2,
  FLASH_CYCLE3,
  FLASH_CYCLE4_BYTE_PROGRAM,
  FLASH_CYCLE4_ERASE,
  FLASH_CYCLE5_ERASE,
  FLASH_CYCLE6_ERASE,
  FLASH_CYCLE4_CHIP_ID
} FlashState;

//FIXME: Add to savestate
FlashState flash_state = FLASH_CYCLE1;
bool flash_chip_id = false;

//FIXME: Make configurable
size_t flash_sector_size = 0x10; // 0 if no sectors
uint8_t flash_chip_vendor = 0xBF;
uint8_t flash_chip_product = 0xD5;

int flash_read(struct em8051* emu, int aAddress) {

  // Reset the current programming cycle
  flash_state = FLASH_CYCLE1;

  // Handle programming mode
  if (flash_chip_id) {
    if (aAddress == 0x0000) {
      return flash_chip_vendor;
    } else if (aAddress == 0x0001) {
      return flash_chip_product;
    } else {
      assert(false);
      return 0x00;
    }
  }

  return flash[aAddress];
}


void flash_write(struct em8051* emu, int aAddress, int aValue) {

  // This is probably WE# or something?
  if ((emu->mSFR[REG_P1] >> 6) & 1) {
    printf("Failed attempt to write flash at 0x%X to 0x%02X? (WE# missing)\n", aAddress, aValue);
    return;
  }

  if ((aAddress == 0x5555) && (aValue == 0xAA)) {
    flash_state = FLASH_CYCLE2;
  } else if ((flash_state == FLASH_CYCLE2) && (aAddress == 0x2AAA) && (aValue == 0x55)) {
    flash_state = FLASH_CYCLE3;
  } else if ((flash_state == FLASH_CYCLE3) && (aAddress == 0x5555)) {
    if (aValue == 0x80) {
      flash_state = FLASH_CYCLE4_ERASE;
    } else if (aValue == 0x90) {
      assert(!flash_chip_id);
      flash_chip_id = true;
      flash_state = FLASH_CYCLE1;
    } else if (aValue == 0xA0) {
      flash_state = FLASH_CYCLE4_BYTE_PROGRAM;
    } else if (aValue == 0xF0) {
      assert(flash_chip_id);
      flash_chip_id = false;
      flash_state = FLASH_CYCLE1;
    } else {
      assert(false);
      flash_state = FLASH_CYCLE1;
    }
  } else if (flash_state == FLASH_CYCLE4_BYTE_PROGRAM) {

    // Program byte
    assert(aAddress < sizeof(flash));
    printf("Trying to write byte 0x%02X to 0x%05X\n", aValue, aAddress);
    flash[aAddress] = aValue; //FIXME: How does the hardware actually behave?

    flash_state = FLASH_CYCLE1;
  } else if ((flash_state == FLASH_CYCLE4_ERASE) && (aAddress == 0x5555) && (aValue == 0xAA)) {
    flash_state = FLASH_CYCLE5_ERASE;
  } else if ((flash_state == FLASH_CYCLE5_ERASE) && (aAddress == 0x2AAA) && (aValue == 0x55)) {
    flash_state = FLASH_CYCLE6_ERASE;
  } else if (flash_state == FLASH_CYCLE6_ERASE) {
    if (aValue == 0x30) {
      assert(flash_sector_size != 0);

      //FIXME: Untested, somehow this never ended up here?!
      assert(false);

      // Erase the sector
      unsigned int sector_offset = aAddress & (flash_sector_size - 1);
      printf("Erasing flash sector at 0x%05X (0x%X)\n", sector_offset, aAddress);
      memset(&flash[sector_offset], flash_sector_size, 0xFF);  

      flash_state = FLASH_CYCLE1;
    } else {
      assert(false);
      flash_state = FLASH_CYCLE1;
    }
  } else {
    flash_state = FLASH_CYCLE1;
  }

  // Reload the bank where we mirrored data
  //FIXME: Can do this less often, but not hot-code anyway
  load_bank(emu);
}


# Optical Disc Drive emulator

This software emulates various optical disc drives.


## Supported chipsets

The focus is on drives that are based on Mediatek chipsets.
Most of the knowledge is based on analysis of Samsung SDG-605B (Firmware: x010).

The list of drives per chipset is incomplete.
There are likely also similar chipsets by Mediatek.

As a result, this emulator can probably also emulate other drives not listed here.

**MT1329E**

| Product | Firmware |
| ------- | -------- |
| Samsung SDG-605B | x010 *(used in original Xbox, no public firmware update)* |
| Samsung SD-616T | F308CPQ |
| BTC BCO1612IM *(unconfirmed)* | *(unknown)* |


## Acquiring a firmware

In order to use this emulator, you need a firmware from one of the supported drives.

### Download a firmware upgrade

The list of supported chipsets above has links to official firmware updates.
Only links to reputable websites are listed to avoid potential copyright violations, hacked firmwares or other issues.

### Software flash extraction

*Not supported; there is currently no known method of reading the firmware by software.*

### Hardware flash extraction

You can desolder the flash chip from the PCB.
It can be dumped using a flasher like the TL866.


## Building

The project is using CMake as build-system, and git for version control.

There are no further requirements.

You can simply clone and build it:

```
git clone ... --recursively
mkdir build
cd build
cmake ..
make
```


## Commands

The emulator has a small interactive shell to provide debugging capabilities.
For the list of commands run `./emu help`.


### Examples

These commands are for Samsung SDG605B firmware x010.


**Create savestate of initialized drive**

`./emu sdg-605b_x010.bin "step 240320512" "save initialized"`


**Inject SCSI ATAPI command (up to 12 bytes?) via external interrupt 1**

This injects an SCSI command into a function which retrieves bits from SCSI.
I'm not sure how to reach this normally.

For the packet to be read, we need all of these conditions

* e4079.2=1
* e400f.2=0
* e400f.4=1
* i28.3=1 or i27.5=0
* e8037 can not be 0x04 (appears to load ATA command in a different way?)
* Trigger external interrupt 1 using s88.3=1

- via CLI: `./emu sdg-605b_x010.bin "load initialized" "scsi 120000006000" "set e4079=04 e400f=10 e8037=00 i28.3=1" "set s88.3=1"`
- via CLI: `./emu sdg-605b_x010.bin "step 240320512" "scsi 120000006000" "set s88.3=1" "s 180"`

Full command handler:

* e400f.6=1

**FIXME: Why doesn't this work?**
- via CLI: `./emu sdg-605b_x010.bin ./emu sdg-605b_x010.bin "load initialized" "scsi F2050007BF0000000060" "set s88.3=1"`


**Inject SCSI ATAPI command (up to 12 bytes?) via unknown location**

This injects an SCSI command into a function which retrieves bits from SCSI.
I'm not sure how to reach this normally.

For the packet to be read, we need all of these conditions
* i28.3=1 or i27.5=0
* e8037 can not be 0x04 (appears to load ATA command in a different way?)

- via CLI: `./emu sdg-605b_x010.bin "load initialized" "scsi 120000006000" "set s90.0=0 pc=0660 e8037=00 i28.3=1"`
- via CLI: `./emu sdg-605b_x010.bin "step 240320512" "scsi 120000006000" "set s90.0=0 pc=0660 e8037=00 i28.3=1" "s 180"`

START STOP UNIT with Immed=0, LoEj/Start=0b11
- via CLI: `./emu sdg-605b_x010.bin "load initialized" "scsi 1B0000000300" "set s90.0=0 pc=0660 e8037=00 i28.3=1"`


**Call command handler (ATAPI)**
```
load initialized
set s90.0=0 pc=2358 i27.6=1 e8037=00 i9a=120000006000
step 61
```

- via CLI: `./emu sdg-605b_x010.bin "load initialized" "set s90.0=0 pc=2358 i27.6=1 e8037=00 i9a=120000006000" "step 100"` (ATAPI INQUIRY)
- via CLI: `./emu sdg-605b_x010.bin "load initialized" "set s90.0=0 pc=2358 i27.6=1 e8037=00 i9a=F2020000000000001002" "step 100"` (Debug command to something which selects flash banks
- via CLI: `./emu sdg-605b_x010.bin "load initialized" "set s90.0=0 pc=2358 i27.6=1 e8037=00 i9a=F2030000000000001002" "step 100"` (Debug command to something which reads from RAM???)
- via CLI: `./emu sdg-605b_x010.bin "load initialized" "set s90.0=0 pc=2358 i27.6=1 e8037=00 i9a=F2040001000000000100" "step 100"` (Debug command to flash firmware)
- via CLI: `./emu sdg-605b_x010.bin "load initialized" "set s90.0=0 pc=2358 i27.6=1 e8037=00 i9a=F2050007BF0000000060" "step 100"` (Debug command to read RAM)


**Call command handler (ATA)**

```
load initialized
set s90.0=0 pc=2358 i27.6=0 e8037=00 i9a=EC... // FIXME
continue
```


## License

**(C) 2020 Jannik Vogel**

All Rights Reserved.

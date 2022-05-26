# IVXWAIT

`IVXWAIT` is a MS-DOS program for Intel I430VX based PC motherboards to set the ISA I/O Recovery Timing.

This program exists because some boards, such as **ECS P5VX-Be** have BIOSes which sets this up with an optimistic default and have no option in the BIOS to change this.

As of now, it only supports the **Intel 82371SB (PIIX3)** ISA bridge.

PCI ID: Vendor `8086h`, Device `7000h`.

---

(C) 2022 Eric "oerg866" Voirin

---


## Running

Usage is as follows:

`IVXWAIT.EXE <8-bit IO Recovery Timing> <16-Bit IO Recovery Timing>`

- `8-bit IO Recovery Timing`:

  This is the amount of **extra** clock cycles to wait after performing an 8-Bit
  ISA I/O operation.

  The range is from 1 to 8.

  To disable additional wait states (i.e. default minimum wait of 3.5 clocks),
  set it to `0`.

  If you wish to not change this value, use `-1`.

- `16-bit IO Recovery Timing`:

  This is the amount of **extra** clock cycles to wait after performing a 16-Bit
  ISA I/O operation.

  The range is from 1 to 4.

  To disable additional wait states (i.e. default minimum wait of 3.5 clocks),
  set it to `0`.

  If you wish to not change this value, use `-1`.

**If you just wish to see the current values and change nothing, use:**

**`IVXWAIT.EXE -1 -1`**

This tool works with `EMM386` present, and may be put in `AUTOEXEC.BAT`.
It should play nicely with MS-DOS, IBM PC DOS, DR-DOS, FreeDOS and
Windows 95 / 98.

## Building

I wrote this in and compiled it with **Borland Turbo C++ 3.0** .

Compile the file through the IDE (`TC.EXE`) or by executing this line in the
repository directory (assumiung that the compiler tool chain is in your `PATH`)
variable:

`TCC -eIVXWAIT.EXE IVXWAIT.C`

It is probably buildable with Microsoft C, DJGPP or (Open)Watcom with little to
no effort.

### Difficulties when writing IVXWAIT

I wanted to do this in Real mode using a contemporary toolchain for that.
Unfortunately none of those seem to have any concept or any knowledge of Intel's
386 CPU whatsoever.

As such, the required 32-Bit I/O port operations required to access PCI
configuration space are not available (the `OUT` instruction operates on an
extended register in this case.)

To fix this, I emulate those instructions in inline assembly using the `db`
keyword, and make sure to prefix those with `0x66` to enable the extended
registers (else you'll get missing words ...).

It works, but it is probably a lot more headache-y than it needs to be ... :-P

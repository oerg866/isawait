# ISAWAIT

(C) 2022-2023 Eric Voirin (oerg866)
--

`ISAWAIT` is a MS-DOS program for Intel I430VX based PC motherboards to set the ISA I/O Recovery Timing.

This program exists because some boards, such as **ECS P5VX-Be** have BIOSes which sets this up with an optimistic default and have no option in the BIOS to change this.

As of now, it supports the following chipsets:

|Vendor ID|Device ID|Name|
|-|-|-|
| `8086` | `112E` | Intel PIIX        |
| `8086` | `7000` | Intel PIIX3       |
| `8086` | `7110` | Intel PIIX4(E)    |
| `1039` | `5113` | SiS 5113          |
| `1039` | `0008` | SiS 559x          |

## Running

Usage is as follows:

`ISAWAIT.EXE <8-bit IO Recovery Timing> <16-Bit IO Recovery Timing>`

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

**`ISAWAIT.EXE -1 -1`**

This tool works with `EMM386` present, and may be put in `AUTOEXEC.BAT`.
It should play nicely with MS-DOS, IBM PC DOS, DR-DOS, FreeDOS and
Windows 95 / 98.

## Building

I wrote this in and compiled it with **Borland Turbo C++ 3.0** .

Assumiung that the compiler tool chain is in your `PATH`, compile the program like this:

`MAKE`

It is probably buildable with Microsoft C, DJGPP or (Open)Watcom with little to
no effort.

### Difficulties when writing ISAWAIT

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

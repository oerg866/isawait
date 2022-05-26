/*
 * IVXWAIT
 *
 * (C) 2022 Eric "Oerg866" Voirin
 *
 * LICENSE: CC-BY-NC 3.0
 *
 * Tool to set 8 and 16-bit I/O Recovery Time Clock Cycles
 * in Intel I430VX chipset based motherboards.
 *
 * Refer to README.MD
 */

#include <stdio.h>
#include <dos.h>

#define VERSION "0.1"

typedef unsigned char  u8;
typedef unsigned short u16;
typedef unsigned long  u32;
typedef signed   char  i8;
typedef signed   short i16;
typedef signed   long  i32;

#ifndef outportl
static u32 inportl(u16 port)
/* Emulates 32-bit I/O port reads using
   manual prefixed 32-bit instructions */
{
    u16 retl, reth;
    asm {
        db 0x50                     /* push eax */
        push bx
        push dx
        mov dx, port
        db 0x66, 0xED               /* in  eax, dx */
        mov retl, ax
        db 0x66, 0xC1, 0xE8, 0x10   /* shr eax, 16 */
        mov reth, ax
        pop dx
        pop bx
        db 0x58                     /* pop eax */
    }
    return (u32) retl | ((u32) reth << 16);
}

static void outportl(u16 port, u32 value)
/* Emulates 32-Bit I/O port writes using
   manual prefixed 32-bit instructions. */
{
    u16 vall = (u16) value;
    u16 valh = (u16) (value >> 16);

    asm {
        db 0x50                     /* push eax */
        db 0x53                     /* push ebx */
        push dx

        mov ax, valh
        db 0x66, 0xC1, 0xE0, 0x10   /* shl eax, 16 */
        mov dx, vall

        db 0x66, 0x0F, 0xB7, 0xDA   /* movzx ebx, dx */
        db 0x66, 0x09, 0xD8         /* or eax, ebx */

        mov dx, port
        db 0x66, 0xEF               /* out dx, eax */

        pop dx
        db 0x5B                     /* pop ebx */
        db 0x58                     /* pop eax */
    }
}
#endif

/* PCI Stuff */

#define BUS_MAX     255     /* Maximum of 256 PCI buses per machine */
#define SLOT_MAX    31      /* Maximum of 32 PCI Slots per bus */

/* TODO: Support other chipsets someday
typedef struct {
    u16 ven
    u16 dev
} pci_dev;

static const pci_dev[] supported_devices{

}
*/

#define I430VX_VEN  0x8086
#define I430VX_DEV  0x7000

static u32 pci_read_32(u32 bus, u32 slot, u32 func, u32 offset)
/* Reads DWORD from PCI config space. Assumes offset is DWORD-aligned. */
{
    u32 address = (bus << 16) | (slot << 11)
                | (func << 8) | (offset & 0xFC)
                | 0x80000000;
    outportl(0xCF8, address);
    return inportl(0xCFC);
}


static u16 pci_read_16(u32 bus, u32 slot, u32 func, u32 offset) {
/* Reads WORD from PCI config space. Assumes offset is WORD-aligned. */

    return (offset & 2) ? (u16) (pci_read_32(bus, slot, func, offset) >> 16)
                        : (u16) (pci_read_32(bus, slot, func, offset));
}

static u8 pci_read_8(u32 bus, u32 slot, u32 func, u32 offset)
/* Reads BYTE from PCI config space. */
{
    printf("%08lx\n", pci_read_32(bus, slot, func, offset));
    switch (offset & 3) {
    case 3: return (u8) (pci_read_32(bus, slot, func, offset) >> 24);
    case 2: return (u8) (pci_read_32(bus, slot, func, offset) >> 16);
    case 1: return (u8) (pci_read_32(bus, slot, func, offset) >>  8);
    case 0: return (u8) (pci_read_32(bus, slot, func, offset) >>  0);
    default: return 0; /* to silence the compiler warning... */
    }
}

static void pci_write_32(u32 bus, u32 slot, u32 func, u32 offset, u32 value)
/* Writes DWORD to PCI config space. Assumes offset is DWORD-aligned. */
{
    u32 address = (bus << 16) | (slot << 11)
                | (func << 8) | (offset & 0xFC)
                | 0x80000000;
    outportl(0xCF8, address);
    outportl(0xCFC, value);
}

static void pci_write_16(u32 bus, u32 slot, u32 func, u32 offset, u16 value)
/* Writes WORD to PCI config space. Assumes offset is WORD-aligned. */
{
    u32 temp = pci_read_32(bus, slot, func, offset);
    temp = (offset & 2) ? ((u32) value << 16) | (temp & 0xFFFF)
                        : ((u32) value) | (temp << 16);
    pci_write_32(bus, slot, func, offset, temp);
}

static void pci_write_8(u32 bus, u32 slot, u32 func, u32 offset, u8 value)
/* Writes BYTE to PCI config space. */
{
    u32 temp = pci_read_32(bus, slot, func, offset);
    switch (offset & 3) {
    case 3: temp = (temp & 0x00FFFFFF) | ((u32) value << 24); break;
    case 2: temp = (temp & 0xFF00FFFF) | ((u32) value << 16); break;
    case 1: temp = (temp & 0xFFFF00FF) | ((u32) value <<  8); break;
    case 0: temp = (temp & 0xFFFFFF00) | ((u32) value <<  0); break;
    }
    pci_write_32(bus, slot, func, offset, temp);
}


u16 pci_get_vendor(u8 bus, u8 slot, u8 func)
/* Gets a PCI device's vendor ID for given bus, slot and function number. */
{
    return pci_read_16(bus, slot, func, 0);
}

u16 pci_get_device(u8 bus, u8 slot, u8 func)
/* Gets a PCI device's device ID for given bus, slot and function number. */
{
    if (pci_get_vendor(bus, slot, func) != 0xFFFF) {
        return pci_read_16(bus, slot, func, 2);
    } else {
        return 0xFFFF;
    }
}

int pci_enum_dev(u8 bus, u8 slot) {
    u16 ven = pci_get_vendor(bus, slot, 0);
    u16 dev;

    if (ven != 0xFFFF) {
        dev = pci_get_device (bus, slot, 0);
        printf("PCI Device %02x:%02x:%02x - VEN_%04x&DEV_%04x\n",
               (int) bus,
               (int) slot,
               0,
               ven,
               dev);
        return 1;
    }

    return 0;
}

int pci_is_vx_device(u8 bus, u8 slot)
/* Checks if device at given bus/slot ID is a supported one.
   Returns 1 if so, 0 if not.*/
{
    if (!pci_enum_dev(bus, slot)) return 0;
    if (pci_get_vendor(bus, slot, 0) != I430VX_VEN) return 0;
    if (pci_get_device(bus, slot, 0) != I430VX_DEV) return 0;

    return 1;
}

/* IVXWAIT specific stuff */

typedef struct
/* Describes I430VX "ISA I/O RECOVERY TIMER REGISTER (0x4C)" register. */
{
    u8 IOREC16_CLOCKS : 2;
    u8 IOREC16_ENABLE : 1;
    u8 IOREC8_CLOCKS: 3;
    u8 IOREC8_ENABLE: 1;
    u8 DMAAC : 1;
} REG_IORT;

static void print_iort_info(REG_IORT iort) {
    printf("Raw value:                                   0x%02x\n"
           "DMA Reserved Page Register Aliasing Control: %d\n"
           "8-Bit I/O Recovery Enable:                   %d\n"
           "8-Bit I/O Recovery Clocks:                   %d\n"
           "16-Bit I/O Recovery Enable:                  %d\n"
           "16-Bit I/O Recovery Clocks:                  %d\n",
           (int) *((u8*) &iort),
           (int) iort.DMAAC,
           (int) iort.IOREC8_ENABLE,
           (int) (iort.IOREC8_CLOCKS == 0 ? 8 : iort.IOREC8_CLOCKS),
           (int) iort.IOREC16_ENABLE,
           (int) (iort.IOREC16_CLOCKS == 0 ? 4 : iort.IOREC16_CLOCKS));
}

int main(int argc, char *argv[]) {
    u16 bus;
    u16 slot;
    u32 test = 0;
    u8  found = 0;

    int iorec8_new = 0;
    int iorec16_new = 0;

    REG_IORT iort;

    printf("IVXWAIT Version %s\n", VERSION);
    printf("(C)2022 Eric \"oerg866\" Voirin\n");
    printf("Message me on Discord: EricV#9999\n");
    printf("----------------------------------------\n");


    if (argc < 3) {
       printf("IVXWAIT Sets the ISA I/O recovery / wait state timer.\n");
       printf("It can be useful to debug malfunctioning ISA devices\n");
       printf("(e.g. OPL2/3 synthesizer chip).\n");
       printf("Usage: IVXWAIT <8 bit cycles> <16 bit cycles>\n");
       printf("Cycle values can be 1 to 8 for 8-bit, 1 to 4 for 16-bit.\n");
       printf("Set to 0 to disable wait states for that bus width.\n");
       printf("Set to -1 to leave value unchagned\n");
       return -1;
    }

    printf("\n\n");

    iorec8_new = atoi(argv[1]);
    iorec16_new = atoi(argv[2]);

    if (iorec8_new > 8) {
       printf("Invalid value for 8-Bit I/O recovery.\n");
       return -1;
    }

    if (iorec16_new > 4) {
       printf("Invalid value for 16-Bit I/O recovery.\n");
       return -1;
    }

    /* Test if we can scan PCI bus. Code stolen from Linux kernel :P */

    outportb(0xCFB, 0x01);
    test = inportl(0xCF8);
    outportl(0xCF8, 0x80000000UL);

    test = inportl(0xCF8);

    if (test != 0x80000000UL) {
       printf("ERROR while testing PCI configuration space access!\n");
       printf("Expected 0x80000000, got 0x%08lx\n", test);
       return -1;
    }

    /* Enumerate PCI buses and find supported PCI device.
       Brute force method described in osdev wiki... */

    printf("Enumerating PCI devices...\n");

    for (bus = 0; bus <= BUS_MAX; ++bus) {
        for (slot = 0; slot <= SLOT_MAX; ++slot) {
            if (pci_is_vx_device(bus, slot)) {
                found = 1;
                break;
            }
        }
        if (found) break;
    }

    if ((bus > BUS_MAX) && (slot > SLOT_MAX)) {
        printf("ERROR - Could not find I430VX ISA bridge device!\n");
        return -1;
    }


    printf("Found I430VX ISA Bridge Device.\n");

    printf("IORT - ISA I/O RECOVERY TIMER REGISTER (0x4C)\n");

    *((u8*) &iort) = pci_read_8(bus, slot, 0, 0x4C);

    printf("Current values:\n");
    print_iort_info(iort);

    /* Setup new iorec for 8 bit */

    if (iorec8_new > -1) {
        iort.IOREC8_ENABLE = iorec8_new == 0 ? 0 : 1;
        iort.IOREC8_CLOCKS = iorec8_new == 8 ? 0 : iorec8_new;
    } else {
        printf("Leaving 8-Bit I/O recovery unchanged.\n");
    }

    /* setup new iorec for 16 bit */

    if (iorec16_new > -1) {
        iort.IOREC16_ENABLE = iorec16_new == 0 ? 0 : 1;
        iort.IOREC16_CLOCKS = iorec16_new == 4 ? 0 : iorec16_new;
    } else {
        printf("Leaving 16-Bit I/O recovery unchanged.\n");
    }

    pci_write_8(bus, slot, 0, 0x4C, *((u8*) &iort));
    printf("\n");
    printf("New values written. The register now contains:\n");

    print_iort_info(iort);
    return 0;
}

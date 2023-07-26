/* ISAWAIT

   Actual ISAWAIT Functionality. */

#include "ISAWAIT.H"

#include <stdio.h>

#include "TYPES.H"
#include "PCI.H"

typedef struct isawait_dev_s;

typedef void (*func_info) (struct isawait_dev_s *dev);
typedef void (*func_ir8)  (struct isawait_dev_s *dev, int value);
typedef void (*func_ir16) (struct isawait_dev_s *dev, int value);


typedef struct isawait_dev_s{
    u16       ven;          /* PCI Vendor ID of device */
    u16       dev;          /* PCI Device ID of device */
    u8        variant;      /* Variant of the device, to handle different regs */
    char      *name;        /* Name string of device */
    func_info print_info;   /* Ptr to function to print current register info */
    func_ir8  ir8_set;      /* Ptr to function to set 8 bit IO recovery time */
    func_ir16 ir16_set;     /* Ptr to fucntion to set 16 bit IO recovery time */
    u8        bus;          /* Bus number of the device */
    u8        slot;         /* Slot number of the device */
} isawait_dev;


/* INTEL PIIX, PIIX3, PIIX4(E) */

static void info_piix(isawait_dev *dev) {
    PIIX_IORT iort;
    iort.raw = pci_read_8(dev->bus, dev->slot, 0, 0x4C);
    printf("Raw: 0x%02x | 8-Bit %s, %d clocks | 16-Bit %s, %d clocks\n",
       (int) iort.raw,
       iort.bits.IOREC8_ENABLE ? "ON" : "OFF",
       iort.bits.IOREC8_CLOCKS == 0 ? 8 : (int) iort.bits.IOREC8_CLOCKS,
       iort.bits.IOREC16_ENABLE ? "ON" : "OFF",
       iort.bits.IOREC16_CLOCKS == 0 ? 4 : (int) iort.bits.IOREC16_CLOCKS);
}

static void ir8_piix(isawait_dev *dev, int value) {
    PIIX_IORT iort;
    iort.raw = pci_read_8(dev->bus, dev->slot, 0, 0x4C);
    iort.bits.IOREC8_ENABLE = value == 0 ? 0 : 1;
    iort.bits.IOREC8_CLOCKS = value == 8 ? 0 : value;
    pci_write_8(dev->bus, dev->slot, 0, 0x4C, iort.raw);
}

static void ir16_piix(isawait_dev *dev, int value) {
    PIIX_IORT iort;
    iort.raw = pci_read_8(dev->bus, dev->slot, 0, 0x4C);
    iort.bits.IOREC16_ENABLE = value == 0 ? 0 : 1;
    iort.bits.IOREC16_CLOCKS = value == 8 ? 0 : value;
    pci_write_8(dev->bus, dev->slot, 0, 0x4C, iort.raw);
}

/* SiS 5113 (0x51), 5597, 5598 (0x46) */

static const u8 sis_iorec_16[] = { 5, 4, 3, 2 };
static const u8 sis_iorec_8[] = { 8, 5, 4, 3 };

static void info_sis(isawait_dev *dev) {
    SIS reg;
    reg.raw = pci_read_8(dev->bus, dev->slot, 0, dev->variant);
    printf("Raw: 0x%02x | 8-Bit %d clocks | 16-Bit %d clocks\n",
       (int) reg.raw,
       (int) sis_iorec_8[reg.bits.IOREC8],
       (int) sis_iorec_16[reg.bits.IOREC16]);
}

static void ir8_sis(isawait_dev *dev, int value) {
    SIS reg;
    int i;
    reg.raw = pci_read_8(dev->bus, dev->slot, 0, dev->variant);

    for (i = 0; i < 4; ++i)
        if (sis_iorec_8[i] == value) {
            reg.bits.IOREC8 = i;
            pci_write_8(dev->bus, dev->slot, 0, dev->variant, reg.raw);
            return;
        }

    printf("SIS ERROR: Ignoring unsupported value.\n");
}

static void ir16_sis(isawait_dev *dev, int value) {
    SIS reg;
    int i;
    reg.raw = pci_read_8(dev->bus, dev->slot, 0, dev->variant);

    for (i = 0; i < 4; ++i)
        if (sis_iorec_16[i] == value) {
            reg.bits.IOREC16 = i;
            pci_write_8(dev->bus, dev->slot, 0, dev->variant, reg.raw);
            return;
        }

    printf("SIS ERROR: Ignoring unsupported value.");
}

static isawait_dev isawait_devs[] = {
    { 0x8086, 0x122E, 0x00, "Intel PIIX",     info_piix, ir8_piix, ir16_piix },
    { 0x8086, 0x7000, 0x00, "Intel PIIX3",    info_piix, ir8_piix, ir16_piix },
    { 0x8086, 0x7110, 0x00, "Intel PIIX4(E)", info_piix, ir8_piix, ir16_piix },
    { 0x1039, 0x5113, 0x51, "SiS 5113",       info_sis,  ir8_sis,  ir16_sis },
    { 0x1039, 0x0008, 0x46, "SiS 559x",       info_sis,  ir8_sis,  ir16_sis },
};

static const int isawait_dev_count = sizeof(isawait_devs) / sizeof(isawait_devs[0]);

static isawait_dev *isawait_get_device(u8 bus, u8 slot)
/* Checks current bus/slot for whether a supported device is on it.
   Returns pointer to isawait_dev if so,
   returns NULL if no or unsupported device. */
{
    int i;
    u16 ven = pci_get_vendor(bus, slot, 0);
    u16 dev = pci_get_device(bus, slot, 0);

    if (ven == 0xFFFF) return NULL; /* No device */

    for (i = 0; i < isawait_dev_count; ++i) {
        if (isawait_devs[i].ven == ven && isawait_devs[i].dev == dev)
           return &isawait_devs[i];
    }
    return NULL; /* Unsupported device */
}

int isawait_set(int iorec8, int iorec16) {
    u16 bus, slot;
    u8 found = 0;
    isawait_dev *dev;

    /* Find device */

    for (bus = 0; bus <= PCI_BUS_MAX; ++bus) {
        for (slot = 0; slot <= PCI_SLOT_MAX; ++slot) {
            dev = isawait_get_device(bus, slot);
            if (dev) {
                found = 1;
                break;
            }
        }
          if (found) break;
    }

    if (!found) {
        printf("ERROR - Could not find I430VX ISA bridge device!\n");
        return -1;
    }

    dev->bus = bus;
    dev->slot = slot;

    printf("Found supported Device, Vendor 0x%04x, Device %04x (%s)\n",
           dev->ven,
           dev->dev,
           dev->name);

    printf("Current values:\n");
    dev->print_info(dev);

    /* Setup new iorec for 8 bit */

    if (iorec8 > -1) {
       dev->ir8_set(dev, iorec8);
    } else {
       printf("Leaving 8-Bit I/O recovery unchanged.\n");
    }

    /* setup new iorec for 16 bit */

    if (iorec16 > -1) {
       dev->ir16_set(dev, iorec16);
    } else {
       printf("Leaving 16-Bit I/O recovery unchanged.\n");
    }

    printf("\n");
    printf("New values written. The register now contains:\n");

    dev->print_info(dev);

    return 1;
}

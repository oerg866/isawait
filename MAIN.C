/*
 * ISAWAIT
 *
 * (C) 2022-2023 Eric Voirin (oerg866@googlemail.com)
 *
 * Tool to set 8 and 16-bit I/O Recovery Time Clock Cycles
 * on supported chipsets
 *
 * Refer to README.MD
 */

#include <stdio.h>
#include <stdlib.h>
#include <dos.h>

#include "TYPES.H"

#include "PCI.H"
#include "ISAWAIT.H"

#define VERSION "1.0"


int main(int argc, char *argv[]) {
    int iorec8_new = 0;
    int iorec16_new = 0;


    printf("ISAWAIT Version %s\n", VERSION);
    printf("  (C)2022-2023 Eric Voirin (oerg866@googlemail.com)\n");
    printf("  Contact via Discord: oerg866 \n");
    printf("----------------------------------------\n");


    if (argc < 3) {
       printf("ISAWAIT Sets the ISA I/O recovery / wait state timer.\n");
       printf("It can be useful to debug malfunctioning ISA devices\n");
       printf("(e.g. OPL2/3 synthesizer chip).\n");
       printf("Usage: ISAWAIT <8 bit cycles> <16 bit cycles>\n");
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


    if (!pci_test()) {
       printf("ERROR enumerating PCI bus. Quitting...\n");
       return -1;
    }

    return isawait_set(iorec8_new, iorec16_new);

}


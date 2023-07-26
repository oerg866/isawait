/* ISAWAIT main functionality header file */

#ifndef ISAWAIT_H
#define ISAWAIT_H


#include "TYPES.H"

typedef struct
/* Describes "ISA I/O RECOVERY TIMER REGISTER" register on PIIX(3) */
{
    u8 IOREC16_CLOCKS : 2;
    u8 IOREC16_ENABLE : 1;
    u8 IOREC8_CLOCKS: 3;
    u8 IOREC8_ENABLE: 1;
    u8 DMAAC : 1;
} PIIX_IORT_BITS;

typedef union {
    u8 raw;
    PIIX_IORT_BITS bits;
} PIIX_IORT;


typedef struct
/* describes Register 51h in SiS 5513 chip */
/* describes Register 46h in SiS 559x */
{
    u8 RESERVED : 4;
    u8 IOREC8 : 2;
    u8 IOREC16 : 2;
} SIS_BITS;

typedef union {
    u8 raw;
    SIS_BITS bits;
} SIS;

#endif
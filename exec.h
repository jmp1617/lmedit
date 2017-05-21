/*
** R2K Suite
**
** exec.h
**    load/object module definitions
**
*/
#ifndef _COMMON_EXEC_H
#define _COMMON_EXEC_H

/*
** pull in standard types
*/

#include <sys/types.h>
#include <stdint.h>

/*
** DEFINITIONS
*/

/*
** magic/version definitions
**
** version number encoding:
**
**    2yyy/mm/dd    yyyyyyy mmmm ddddd
**
** last representable version date: 2127/12/31
*/

    /* common magic number */
#define    HDR_MAGIC    0xface

    /* module version numbers */
    /* version 1:    0000111 1001 00010    2007/09/02 */
#define    HDR_VERSION_1    0x0f22

    /* current version number */
#define    HDR_VERSION    HDR_VERSION_1

/*
** memory allocation information
**
** based on Gerry Kane's "MIPS R2000 RISC Architecture", Appendix D
**
** basic layout:
**
**    0x00000000 - 0x7fffffff        2GB for program
**    0x80000000 - 0xffffffff        2GB reserved for kernel
** 
** within the program's space:
**
**    0x00000000 - 0x003fffff        4MB reserved
**    0x00400000 - 0x0fffffff        252MB text
**    0x10000000 - 0x7fffffff        1.75GB data
**
** within the data area:
**
**    rdata begins at 0x10000000
**    data, sdata, sbss, bss, and heap immediately follow rdata
**        heap is initially of size 0, and grows toward high memory
**        each data area begins on a 2^3 boundary (.align 3)
**
**    stack ends at 0x7fffefff, and grows toward zero
**
**    0x7ffff000 - 0x7fffffff    is reserved by convention (not by hardware)
*/

    /* address ranges */
#define    RESERVED_LO_BEGIN 0x00000000
#define    RESERVED_LO_END   0x003fffff

#define    TEXT_BEGIN        0x00400000
#define    TEXT_END          0x0fffffff

#define    DATA_BEGIN        0x10000000
#define    DATA_END          0x7fffefff

#define    STACK_BEGIN       0x7fffd000
#define    STACK_END         DATA_END

#define    RESERVED_HI_BEGIN 0x7ffff000
#define    RESERVED_HI_END   0xffffffff

    /* initially, use an 8KB stack */
#define    STACK_SIZE    0x2000

/*
** DATA TYPES
*/

/*
** module header
*/

#define EH_IX_TEXT  0
#define EH_IX_RDATA 1
#define EH_IX_DATA  2
#define EH_IX_SDATA 3
#define EH_IX_SBSS  4
#define EH_IX_BSS   5
#define EH_IX_REL   6
#define EH_IX_REF   7
#define EH_IX_SYM   8
#define EH_IX_STR   9
#define N_EH        10

typedef
    struct exec {
        uint16_t magic;         /* magic number */
        uint16_t version;       /* object module format version number */
        uint32_t flags;         /* flags describing module contents */
        uint32_t entry;         /* entry point */
        uint32_t data[N_EH];    /* section sizes (bytes, or entries) */
    }
        exec_t;

    /* aliases for fields within the data array */
#define sz_text     data[EH_IX_TEXT]
#defin:q
        sz_rdata    data[EH_IX_RDATA]
#define sz_data     data[EH_IX_DATA]
#define sz_sdata    data[EH_IX_SDATA]
#define sz_sbss     data[EH_IX_SBSS]
#define sz_bss      data[EH_IX_BSS]
#define n_reloc     data[EH_IX_REL]
#define n_refs      data[EH_IX_REF]
#define n_syms      data[EH_IX_SYM]
#define sz_strings  data[EH_IX_STR]

/*
** symbol table entry
*/

typedef
    struct syment {
        uint32_t flags;      /* flag word */
        uint32_t value;      /* value associated with this symbol */
        uint32_t sym;        /* symbol name's index into string table */
    }
        syment_t;

/*
** relocation table entry
*/

typedef
    struct relent {
        uint32_t addr;       /* location to be relocated */
        uint8_t  section;    /* section number */
        uint8_t  type;       /* how to do the relocation */
    }
        relent_t;

    /*
    ** relocation types
    */

    /* use lower 16 bits to update 16-bit immediate field */
#define    REL_IMM     0x01

    /* split 32-bit value across two 16-bit immediate fields */
#define    REL_IMM_2   0x02

    /* 32-bit word */
#define    REL_WORD    0x03

    /* 26-bit jump target */
#define    REL_JUMP    0x04

/*
** reference table entry
*/

typedef
    struct refent {
        uint32_t addr;    /* location of the reference (within its section) */
        uint32_t sym;     /* symbol being referenced */
        uint8_t  section; /* section number */
        uint8_t  type;    /* how to fill in the reference */
    }
        refent_t;

#endif

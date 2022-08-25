/***************************************************************************
 *   Copyright (C) 2012 by Luigi Rucco and Terraneo Federico               *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   As a special exception, if other files instantiate templates or use   *
 *   macros or inline functions from this file, or you compile this file   *
 *   and link it with other works to produce a work based on this file,    *
 *   this file does not by itself cause the resulting work to be covered   *
 *   by the GNU General Public License. However the source code for this   *
 *   file must still be made available in accordance with the GNU General  *
 *   Public License. This exception does not invalidate any other reasons  *
 *   why a work based on this file might be covered by the GNU General     *
 *   Public License.                                                       *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, see <http://www.gnu.org/licenses/>   *
 ***************************************************************************/

#ifndef ELF_TYPES_H
#define	ELF_TYPES_H

#include <inttypes.h>
#include "config/miosix_settings.h"

#ifdef WITH_PROCESSES

namespace miosix {

// elf-specific types
typedef uint32_t Elf32_Word;
typedef int32_t  Elf32_Sword;
typedef uint16_t Elf32_Half;
typedef uint32_t Elf32_Off;
typedef uint32_t Elf32_Addr;

// Size of e_ident in the elf header
const int EI_NIDENT=16;

/*
 * Elf header
 */
struct Elf32_Ehdr
{
    unsigned char e_ident[EI_NIDENT]; // Ident bytes
    Elf32_Half e_type;                // File type, any of the ET_* constants
    Elf32_Half e_machine;             // Target machine
    Elf32_Word e_version;             // File version
    Elf32_Addr e_entry;               // Start address
    Elf32_Off e_phoff;                // Phdr file offset
    Elf32_Off e_shoff;                // Shdr file offset
    Elf32_Word e_flags;               // File flags
    Elf32_Half e_ehsize;              // Sizeof ehdr
    Elf32_Half e_phentsize;           // Sizeof phdr
    Elf32_Half e_phnum;               // Number phdrs
    Elf32_Half e_shentsize;           // Sizeof shdr
    Elf32_Half e_shnum;               // Number shdrs
    Elf32_Half e_shstrndx;            // Shdr string index
} __attribute__((packed));

// Values for e_type
const Elf32_Half ET_NONE = 0; // Unknown type
const Elf32_Half ET_REL  = 1; // Relocatable
const Elf32_Half ET_EXEC = 2; // Executable
const Elf32_Half ET_DYN  = 3; // Shared object
const Elf32_Half ET_CORE = 4; // Core file

// Values for e_version
const Elf32_Word EV_CURRENT = 1;

// Values for e_machine
const Elf32_Half EM_ARM  = 0x28;

// Values for e_flags
const Elf32_Word EF_ARM_EABIMASK   = 0xff000000;
const Elf32_Word EF_ARM_EABI_VER5  = 0x05000000;
const Elf32_Word EF_ARM_VFP_FLOAT  = 0x400;
const Elf32_Word EF_ARM_SOFT_FLOAT = 0x200;

/*
 * Elf program header
 */
struct Elf32_Phdr
{
    Elf32_Word p_type;   // Program header type, any of the PH_* constants
    Elf32_Off  p_offset; // Segment start offset in file
    Elf32_Addr p_vaddr;  // Segment virtual address
    Elf32_Addr p_paddr;  // Segment physical address
    Elf32_Word p_filesz; // Segment size in file
    Elf32_Word p_memsz;  // Segment size in memory
    Elf32_Word p_flags;  // Segment flasgs, any of the PF_* constants
    Elf32_Word p_align;  // Segment alignment requirements
} __attribute__((packed));

// Values for p_type
const Elf32_Word PT_NULL    = 0; // Unused array entry
const Elf32_Word PT_LOAD    = 1; // Loadable segment
const Elf32_Word PT_DYNAMIC = 2; // Segment is the dynamic section
const Elf32_Word PT_INTERP  = 3; // Shared library interpreter
const Elf32_Word PT_NOTE    = 4; // Auxiliary information

// Values for p_flags
const Elf32_Word PF_X = 0x1; // Execute
const Elf32_Word PF_W = 0x2; // Write
const Elf32_Word PF_R = 0x4; // Read

/*
 * Entries of the DYNAMIC segment
 */
struct Elf32_Dyn
{
    Elf32_Sword d_tag;    // Type of entry
    union {
        Elf32_Word d_val; // Value of entry, if number
        Elf32_Addr d_ptr; // Value of entry, if offset into the file
    } d_un;
} __attribute__((packed));

// Values for d_tag
const int DT_NULL         = 0;
const int DT_NEEDED       = 1;
const int DT_PLTRELSZ     = 2;
const int DT_PLTGOT       = 3;
const int DT_HASH         = 4;
const int DT_STRTAB       = 5;
const int DT_SYMTAB       = 6;
const int DT_RELA         = 7;
const int DT_RELASZ       = 8;
const int DT_RELAENT      = 9;
const int DT_STRSZ        = 10;
const int DT_SYMENT       = 11;
const int DT_INIT         = 12;
const int DT_FINI         = 13;
const int DT_SONAME       = 14;
const int DT_RPATH        = 15;
const int DT_SYMBOLIC     = 16;
const int DT_REL          = 17;
const int DT_RELSZ        = 18;
const int DT_RELENT       = 19;
const int DT_PLTREL       = 20;
const int DT_DEBUG        = 21;
const int DT_TEXTREL      = 22;
const int DT_JMPREL       = 23;
const int DT_BINDNOW      = 24;
const int DT_MX_RAMSIZE   = 0x10000000; //Miosix specific, RAM size
const int DT_MX_STACKSIZE = 0x10000001; //Miosix specific, STACK size
const int DT_MX_ABI       = 0x736f694d; //Miosix specific, ABI version
const unsigned int DV_MX_ABI_V0 = 0x00007869; //Miosix specific, ABI version 0
const unsigned int DV_MX_ABI_V1 = 0x01007869; //Miosix specific, ABI version 1

/*
 * Relocation entries
 */
struct Elf32_Rel
{
    Elf32_Addr r_offset;
    Elf32_Word r_info;
} __attribute__((packed));

// To extract the two fields of r_info
#define ELF32_R_SYM(i) ((i)>>8)
#define ELF32_R_TYPE(i) ((unsigned char)(i))


// Possible values for ELF32_R_TYPE(r_info)
const unsigned char R_ARM_NONE     = 0;
const unsigned char R_ARM_ABS32    = 2;
const unsigned char R_ARM_RELATIVE = 23;

} //namespace miosix

#endif //WITH_PROCESSES

#endif //ELF_TYPES_H

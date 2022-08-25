/***************************************************************************
 *   Copyright (C) 2012 by Terraneo Federico                               *
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

#include "elf_program.h"
#include "process_pool.h"
#include <stdexcept>
#include <cstring>
#include <cstdio>

using namespace std;

#ifdef WITH_PROCESSES

namespace miosix {

///\internal Enable/disable debugging of program loading
//#define DBG iprintf
#define DBG(x,...) do {} while(0)
    
///By convention, in an elf file for Miosix, the data segment starts @ this addr
static const unsigned int DATA_BASE=0x40000000;

//
// class ElfProgram
//

ElfProgram::ElfProgram(const unsigned int *elf, unsigned int size)
    : elf(elf), size(size)
{
    //Trying to follow the "full recognition before processing" approach,
    //(http://www.cs.dartmouth.edu/~sergey/langsec/occupy/FullRecognition.jpg)
    //all of the elf fields that will later be used are checked in advance.
    //Unused fields are unchecked, so when using new fields, add new checks
    if(validateHeader()==false) throw runtime_error("Bad file");
}

bool ElfProgram::validateHeader()
{
    //Validate ELF header
    //Note: this code assumes a little endian elf and a little endian ARM CPU
    if(isUnaligned8(getElfBase()))
        throw runtime_error("Elf file load address alignment error");
    if(size<sizeof(Elf32_Ehdr)) return false;
    const Elf32_Ehdr *ehdr=getElfHeader();
    static const char magic[EI_NIDENT]={0x7f,'E','L','F',1,1,1};
    if(memcmp(ehdr->e_ident,magic,EI_NIDENT))
        throw runtime_error("Unrecognized format");
    if(ehdr->e_type!=ET_EXEC) throw runtime_error("Not an executable");
    if(ehdr->e_machine!=EM_ARM) throw runtime_error("Wrong CPU arch");
    if(ehdr->e_version!=EV_CURRENT) return false;
    if(ehdr->e_entry>=size) return false;
    if(ehdr->e_phoff>=size-sizeof(Elf32_Phdr)) return false;
    if(isUnaligned4(ehdr->e_phoff)) return false;
    // Old GCC 4.7.3 used to set bit 0x2 (EF_ARM_HASENTRY) but there's no trace
    // of this requirement in the current ELF spec for ARM.
    if((ehdr->e_flags & EF_ARM_EABIMASK) != EF_ARM_EABI_VER5) return false;
    #if !defined(__FPU_USED) || __FPU_USED==0
    if(ehdr->e_flags & EF_ARM_VFP_FLOAT) throw runtime_error("FPU required");
    #endif
    if(ehdr->e_ehsize!=sizeof(Elf32_Ehdr)) return false;
    if(ehdr->e_phentsize!=sizeof(Elf32_Phdr)) return false;
    //This to avoid that the next condition could pass due to 32bit wraparound
    //20 is an arbitrary number, could be increased if required
    if(ehdr->e_phnum>20) throw runtime_error("Too many segments");
    if(ehdr->e_phoff+(ehdr->e_phnum*sizeof(Elf32_Phdr))>size) return false;
    
    //Validate program header table
    bool codeSegmentPresent=false;
    bool dataSegmentPresent=false;
    bool dynamicSegmentPresent=false;
    int dataSegmentSize=0;
    const Elf32_Phdr *phdr=getProgramHeaderTable();
    for(int i=0;i<getNumOfProgramHeaderEntries();i++,phdr++)
    {
        //The third condition does not imply the other due to 32bit wraparound
        if(phdr->p_offset>=size) return false;
        if(phdr->p_filesz>=size) return false;
        if(phdr->p_offset+phdr->p_filesz>size) return false;
        switch(phdr->p_align)
        {
            case 1: break;
            case 4:
                if(isUnaligned4(phdr->p_offset)) return false;
                break;
            case 8:
                if(isUnaligned8(phdr->p_offset)) return false;
                break;
            default:
                throw runtime_error("Unsupported segment alignment");
        }
        
        switch(phdr->p_type)
        {
            case PT_LOAD:
                if(phdr->p_flags & ~(PF_R | PF_W | PF_X)) return false;
                if(!(phdr->p_flags & PF_R)) return false;
                if((phdr->p_flags & PF_W) && (phdr->p_flags & PF_X))
                    throw runtime_error("File violates W^X");
                if(phdr->p_flags & PF_X)
                {
                    if(codeSegmentPresent) return false; //Can't apper twice
                    codeSegmentPresent=true;
                    if(ehdr->e_entry<phdr->p_offset ||
                       ehdr->e_entry>phdr->p_offset+phdr->p_filesz ||
                       phdr->p_filesz!=phdr->p_memsz) return false;
                }
                if((phdr->p_flags & PF_W) && !(phdr->p_flags & PF_X))
                {
                    if(dataSegmentPresent) return false; //Two data segments?
                    dataSegmentPresent=true;
                    if(phdr->p_memsz<phdr->p_filesz) return false;
                    unsigned int maxSize=MAX_PROCESS_IMAGE_SIZE-
                        MIN_PROCESS_STACK_SIZE;
                    if(phdr->p_memsz>=maxSize)
                        throw runtime_error("Data segment too big");
                    dataSegmentSize=phdr->p_memsz;
                }
                break;
            case PT_DYNAMIC:
                if(dynamicSegmentPresent) return false; //Two dynamic segments?
                dynamicSegmentPresent=true;
                //DYNAMIC segment *must* come after data segment
                if(dataSegmentPresent==false) return false;
                if(phdr->p_align<4) return false;
                if(validateDynamicSegment(phdr,dataSegmentSize)==false)
                    return false;
                break;
            default:
                //Ignoring other segments
                break;
        }
    }
    if(codeSegmentPresent==false) return false; //Can't not have code segment
    return true;
}

bool ElfProgram::validateDynamicSegment(const Elf32_Phdr *dynamic,
        unsigned int dataSegmentSize)
{
    unsigned int base=getElfBase();
    const Elf32_Dyn *dyn=reinterpret_cast<const Elf32_Dyn*>(base+dynamic->p_offset);
    const int dynSize=dynamic->p_memsz/sizeof(Elf32_Dyn);
    Elf32_Addr dtRel=0;
    Elf32_Word dtRelsz=0;
    unsigned int hasRelocs=0;
    bool miosixTagFound=false;
    unsigned int ramSize=0;
    unsigned int stackSize=0;
    for(int i=0;i<dynSize;i++,dyn++)
    {
        switch(dyn->d_tag)
        {
            case DT_REL:
                hasRelocs |= 0x1;
                dtRel=dyn->d_un.d_ptr;
                break;
            case DT_RELSZ:
                hasRelocs |= 0x2;
                dtRelsz=dyn->d_un.d_val;
                break;
            case DT_RELENT:
                hasRelocs |= 0x4;
                if(dyn->d_un.d_val!=sizeof(Elf32_Rel)) return false;
                break;  
            case DT_MX_ABI:
                if(dyn->d_un.d_val==DV_MX_ABI_V1) miosixTagFound=true;
                else throw runtime_error("Unknown/unsupported DT_MX_ABI");
                break;
            case DT_MX_RAMSIZE:
                ramSize=dyn->d_un.d_val;
                break;
            case DT_MX_STACKSIZE:
                stackSize=dyn->d_un.d_val;
                break;
            case DT_RELA:
            case DT_RELASZ:
            case DT_RELAENT:
                throw runtime_error("RELA relocations unsupported");
            default:
                //Ignore other entries
                break;
        }
    }
    if(miosixTagFound==false) throw runtime_error("Not a Miosix executable");
    if(stackSize<MIN_PROCESS_STACK_SIZE)
        throw runtime_error("Requested stack is too small");
    if(ramSize>MAX_PROCESS_IMAGE_SIZE)
        throw runtime_error("Requested image size is too large");
    if((stackSize & 0x3) ||
       (ramSize & 0x3) ||
       (ramSize < ProcessPool::blockSize) ||
       (stackSize>MAX_PROCESS_IMAGE_SIZE) ||
       (dataSegmentSize>MAX_PROCESS_IMAGE_SIZE) ||
       (dataSegmentSize+stackSize>ramSize))
        throw runtime_error("Invalid stack or RAM size");
    
    if(hasRelocs!=0 && hasRelocs!=0x7) return false;
    if(hasRelocs)
    {
        //The third condition does not imply the other due to 32bit wraparound
        if(dtRel>=size) return false;
        if(dtRelsz>=size) return false;
        if(dtRel+dtRelsz>size) return false;
        if(isUnaligned4(dtRel)) return false;
        
        const Elf32_Rel *rel=reinterpret_cast<const Elf32_Rel*>(base+dtRel);
        const int relSize=dtRelsz/sizeof(Elf32_Rel);
        for(int i=0;i<relSize;i++,rel++)
        {
            switch(ELF32_R_TYPE(rel->r_info))
            {
                case R_ARM_NONE:
                    break;
                case R_ARM_RELATIVE:
                    if(rel->r_offset<DATA_BASE) return false;
                    if(rel->r_offset>DATA_BASE+dataSegmentSize-4) return false;
                    if(rel->r_offset & 0x3) return false;
                    break;
                default:
                    throw runtime_error("Unexpected relocation type");
            }
        }
    }
    return true;
}

//
// class ProcessImage
//

void ProcessImage::load(const ElfProgram& program)
{
    if(image) ProcessPool::instance().deallocate(image);
    const unsigned int base=program.getElfBase();
    const Elf32_Phdr *phdr=program.getProgramHeaderTable();
    const Elf32_Phdr *dataSegment=0;
    Elf32_Addr dtRel=0;
    Elf32_Word dtRelsz=0;
    bool hasRelocs=false;
    for(int i=0;i<program.getNumOfProgramHeaderEntries();i++,phdr++)
    {
        switch(phdr->p_type)
        {
            case PT_LOAD:
                if((phdr->p_flags & PF_W) && !(phdr->p_flags & PF_X))
                    dataSegment=phdr;
                break;
            case PT_DYNAMIC:
            {
                const Elf32_Dyn *dyn=reinterpret_cast<const Elf32_Dyn*>
                    (base+phdr->p_offset);
                const int dynSize=phdr->p_memsz/sizeof(Elf32_Dyn);
                for(int i=0;i<dynSize;i++,dyn++)
                {
                    switch(dyn->d_tag)
                    {
                        case DT_REL:
                            hasRelocs=true;
                            dtRel=dyn->d_un.d_ptr;
                            break;
                        case DT_RELSZ:
                            hasRelocs=true;
                            dtRelsz=dyn->d_un.d_val;
                            break;
                        case DT_MX_RAMSIZE:
                            size=dyn->d_un.d_val;
                            image=ProcessPool::instance()
                                    .allocate(dyn->d_un.d_val);
                        default:
                            break;
                    }
                }
                break;
            }
            default:
                //Ignoring other segments
                break;
        }
    }
    const char *dataSegmentInFile=
        reinterpret_cast<const char*>(base+dataSegment->p_offset);
    char *dataSegmentInMem=reinterpret_cast<char*>(image);
    memcpy(dataSegmentInMem,dataSegmentInFile,dataSegment->p_filesz);
    dataSegmentInMem+=dataSegment->p_filesz;
    memset(dataSegmentInMem,0,dataSegment->p_memsz-dataSegment->p_filesz);
    if(hasRelocs)
    {
        const Elf32_Rel *rel=reinterpret_cast<const Elf32_Rel*>(base+dtRel);
        const int relSize=dtRelsz/sizeof(Elf32_Rel);
        const unsigned int ramBase=reinterpret_cast<unsigned int>(image);
        DBG("Relocations -- start (code base @0x%x, data base @ 0x%x)\n",base,ramBase);
        for(int i=0;i<relSize;i++,rel++)
        {
            unsigned int offset=(rel->r_offset-DATA_BASE)/4;
            switch(ELF32_R_TYPE(rel->r_info))
            {
                case R_ARM_RELATIVE:
                    if(image[offset]>=DATA_BASE)
                    {
                        DBG("R_ARM_RELATIVE offset 0x%x from 0x%x to 0x%x\n",
                            offset*4,image[offset],image[offset]+ramBase-DATA_BASE);
                        image[offset]+=ramBase-DATA_BASE;
                    } else {
                        DBG("R_ARM_RELATIVE offset 0x%x from 0x%x to 0x%x\n",
                            offset*4,image[offset],image[offset]+base);
                        image[offset]+=base;
                    }
                    break;
                default:
                    break;
            }
        }
        DBG("Relocations -- end\n");
    }
}

ProcessImage::~ProcessImage()
{
    if(image) ProcessPool::instance().deallocate(image);
}

} //namespace miosix

#endif //WITH_PROCESSES

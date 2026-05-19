/***************************************************************************
 *   Copyright (C) 2013 by Terraneo Federico                               *
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

//Makes memrchr available in newer GCCs
#define _GNU_SOURCE
#include <string.h>

#include "stringpart.h"
#include <cassert>

using namespace std;

namespace miosix {

//
// class StringPart
//

StringPart::StringPart(string& str, size_t idx, size_t off)
        : str(&str), index(idx), offset(off), saved('\0'), owner(false),
          type(CPPSTR)
{
    if(index==string::npos || index>=str.length()) index=str.length();
    else {
        saved=str[index];
        str[index]='\0';
    }
    offset=min(offset,index);
}

StringPart::StringPart(char* s, size_t idx, size_t off)
        : cstr(s), index(idx), offset(off), saved('\0'), owner(false),
          type(CSTR)
{
    assert(cstr); //Passed pointer can't be null
    size_t len=strlen(cstr);
    if(index==string::npos || index>=len) index=len;
    else {
        saved=cstr[index];
        cstr[index]='\0';
    }
    offset=min(offset,index);
}

StringPart::StringPart(const char* s)
        : ccstr(s), offset(0), saved('\0'), owner(false), type(CCSTR)
{
    assert(ccstr); //Passed pointer can't be null
    index=strlen(s);
}

StringPart::StringPart(StringPart& rhs, size_t idx, size_t off)
        : saved('\0'), owner(false), type(rhs.type)
{
    switch(type)
    {
        case CSTR:
            this->cstr=rhs.cstr;
            break;
        case CCSTR:
            type=CSTR; //To make a substring of a CCSTR we need to make a copy
            if(rhs.empty()==false) assign(rhs); else cstr=&saved;
            break;
        case CPPSTR:
            this->str=rhs.str;
            break;
    }
    if(idx!=string::npos && idx<rhs.length())
    {
        index=rhs.offset+idx;//Make index relative to beginning of original str
        if(type==CPPSTR)
        {
            saved=(*str)[index];
            (*str)[index]='\0';
        } else {
            saved=cstr[index];
            cstr[index]='\0';
        }
    } else index=rhs.index;
    offset=min(rhs.offset+off,index); //Works for CCSTR as offset is always zero
}

StringPart::StringPart(const StringPart& rhs)
        : cstr(&saved), index(0), offset(0), saved('\0'), owner(false),
          type(CSTR)
{
    if(rhs.empty()==false) assign(rhs);
}

StringPart& StringPart::operator= (const StringPart& rhs)
{
    if(this==&rhs) return *this; //Self assignment
    //Don't forget that we may own a pointer to someone else's string,
    //so always clear() to detach from a string on assignment!
    clear();
    if(rhs.empty()==false) assign(rhs);
    return *this;
}

bool StringPart::startsWith(const StringPart& rhs) const
{
    if(this->length()<rhs.length()) return false;
    return memcmp(this->c_str(),rhs.c_str(),rhs.length())==0;
}

size_t StringPart::findLastOf(char c) const
{
    const char *begin=c_str();
    //Not strrchr() to take advantage of knowing the string length
    void *index=memrchr(begin,c,length());
    if(index==0) return std::string::npos;
    return reinterpret_cast<char*>(index)-begin;
}

const char *StringPart::c_str() const
{
    switch(type)
    {
        case CSTR: return cstr+offset;
        case CCSTR: return ccstr; //Offset always 0
        default: return str->c_str()+offset;
    }
}

char StringPart::operator[] (size_t index) const
{
    switch(type)
    {
        case CSTR: return cstr[offset+index];
        case CCSTR: return ccstr[index]; //Offset always 0
        default: return (*str)[offset+index];
    }
}

void StringPart::clear()
{
    if(type==CSTR)
    {
        cstr[index]=saved;//Worst case we'll overwrite terminating \0 with an \0
        if(owner) delete[] cstr;
    } else if(type==CPPSTR) {
        if(index!=str->length()) (*str)[index]=saved;
        if(owner) delete str;
    } //For CCSTR there's nothing to do
    cstr=&saved; //Reusing saved as an empty string
    saved='\0';
    index=offset=0;
    owner=false;
    type=CSTR;
}

void StringPart::assign(const StringPart& rhs)
{
    cstr=new char[rhs.length()+1];
    strcpy(cstr,rhs.c_str());
    index=rhs.length();
    offset=0;
    owner=true;
}

} //namespace miosix

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

#include "unicode.h"

using namespace std;

 #define PUT(x) do \
{ \
    if(length>=dstSize) return make_pair(INSUFFICIENT_SPACE,length); \
    *dst++=x; length++; \
} while(0)

namespace miosix {

pair<Unicode::error,int> Unicode::putUtf8(char *dst, char32_t c, int dstSize)
{
    //Reserved space for surrogate pairs in utf16 are invalid code points
    if(c>=0xd800 && c<= 0xdfff) return make_pair(INVALID_STRING,0);
    //Unicode is limited in the range 0-0x10ffff
    if(c>0x10ffff) return make_pair(INVALID_STRING,0);
    int length=0;

    if(c<0x80)
    {
        PUT(c);
        return make_pair(OK,length);
    }
    
    if(c<0x800)
    {
        PUT(c>>6 | 0xc0);
    } else if(c<0x10000) {
        PUT(c>>12 | 0xe0);
        PUT(((c>>6) & 0x3f) | 0x80);
    } else {
        PUT(c>>18 | 0xf0);
        PUT(((c>>12) & 0x3f) | 0x80);
        PUT(((c>>6) & 0x3f) | 0x80);
    }
    PUT((c & 0x3f) | 0x80);
    return make_pair(OK,length);
}

pair<Unicode::error,int> Unicode::utf8toutf16(char16_t *dst, int dstSize,
        const char *src)
{
    int length=0;
    
    for(;;)
    {
        char32_t c=nextUtf8(src);
        if(c==0) break;
        if(c==invalid) return make_pair(INVALID_STRING,length);
        
        if(c>0xffff)
        {
            const char32_t leadOffset=0xd800-(0x10000>>10);
            PUT(leadOffset+(c>>10));
            PUT(0xdc00+(c & 0x3ff));
        } else PUT(c);
    }
    
    PUT(0); //Terminate string
    return make_pair(OK,length-1);
}

pair<Unicode::error,int> Unicode::utf16toutf8(char *dst, int dstSize,
        const char16_t *src)
{
    //Note: explicit cast to be double sure that no sign extension happens
    const unsigned short *srcu=reinterpret_cast<const unsigned short*>(src);
    int length=0;
    
    while(char32_t c=*srcu++)
    {
        //Common case first: ASCII
        if(c<0x80)
        {
            PUT(c);
            continue;
        }
        
        //If not ASCII, pass through utf32        
        if(c>=0xd800 && c<=0xdbff)
        {
            char32_t next=*srcu++;
            //Unpaired lead surrogate (this includes the case next==0)
            if(next<0xdc00 || next>0xdfff) return make_pair(INVALID_STRING,length);
            
            const char32_t surrogateOffset=0x10000-(0xd800<<10)-0xdc00;
            c=(c<<10)+next+surrogateOffset;
        } else if(c>=0xdc00 && c<=0xdfff) {
            //Unpaired trail surrogate
            return make_pair(INVALID_STRING,length);
        }
        
        pair<error,int> result=putUtf8(dst,c,dstSize-length);
        dst+=result.second;
        length+=result.second;
        if(result.first!=OK) return make_pair(result.first,length);
    }
    
    PUT(0); //Terminate string
    return make_pair(OK,length-1);
}

pair<bool,int> Unicode::validateUtf8(const char* str)
{
    const char *iter=str;
    for(;;)
    {
        char32_t codePoint=nextUtf8(iter);
        if(codePoint==0) return make_pair(true,iter-str);
        if(codePoint==invalid) return make_pair(false,iter-str);
    }
}

} //namespace miosix

/*
#include <iostream>
#include <fstream>
#include <cassert>

using namespace std;
using namespace miosix;

int main(int argc, char *argv[])
{
    ifstream in(argv[1]);
    in.seekg(0,ios::end);
    const int size=in.tellg();
    in.seekg(0,ios::beg);
    ofstream out(argv[2]);
    if(argv[3][0]=='u')
    {
        char *c=new char[size+1];
        in.read(c,size);
        c[size]='\0';
        char16_t *cc=new char16_t[512];
        pair<Unicode::error,int> result=Unicode::utf8toutf16(cc,512,c);
        assert(result.first==Unicode::OK);
        cout<<"Target string len "<<result.second<<endl;
        out.write((char*)cc,result.second*2);
    } else {
        char16_t *c=new char16_t[size/2+1];
        in.read((char*)c,size);
        c[size/2]=0;
        char *cc=new char[1024];
        pair<Unicode::error,int> result=Unicode::utf16toutf8(cc,1024,c);
        assert(result.first==Unicode::OK);
        cout<<"Target string len "<<result.second<<endl;
        out.write(cc,result.second);
    }
} 
*/

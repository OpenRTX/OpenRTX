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

#include <stdint.h>
#include <utility>

#ifndef UNICODE_H
#define UNICODE_H

#if __cplusplus <= 199711L
//These are builtin types in C++11, add them if compiling in C++03 mode
typedef uint16_t char16_t;
typedef uint32_t char32_t;
#endif // !c++11

namespace miosix {

/**
 * Result codes for unicode related conversion stuff
 */
class Unicode
{
public:
    /**
     * Possible errors for unicode string conversion
     */
    enum error
    {
        OK,                 ///< The string conversion completed successfully
        INSUFFICIENT_SPACE, ///< The source string is too long to fit
        INVALID_STRING      ///< The source string is an illegal unicode string
    };
    
    /// Represents an invalid code point
    static const char32_t invalid=0xffffffff;
    
    /**
     * Peek an unicode code point out of an iterator into an utf8 string
     * \param it an iterator into an utf8 encoded string
     * \param end iterator one past the last character of the string
     * \return an unicode code point, or Unicode::invalid if the string
     * contains an invalid code point. Returns 0 if the end of string is found,
     * and it is not in the middle of a character
     */
    template<typename Iter>
    static char32_t nextUtf8(Iter& it, Iter end)
    {
        return nextUtf8(it,end,true);
    }
    
    /**
     * Peek an unicode code point out of an iterator into an utf8 string
     * \param it an iterator into an utf8 encoded string, the string is assumed
     * to be nul-terminated
     * \return an unicode code point, or Unicode::invalid if the string
     * contains an invalid code point. Returns 0 if the end of string is found,
     * and it is not in the middle of a character
     */
    template<typename Iter>
    static char32_t nextUtf8(Iter& it)
    {
        return nextUtf8(it,it,false);
    }
    
    /**
     * Put an unicode code point into a character array, converting it to utf8.
     * \param dst pointer to the buffer where the character is to be written
     * \param c an unicode code point (utf32 char)
     * \param dstSize number of bytes available in dst
     * \return an error code and the number of bytes of dst that were used up to
     * write src to dst
     */
    static std::pair<error,int> putUtf8(char *dst, char32_t c, int dstSize);
    
    /**
     * Convert an utf8 string in an utf16 one
     * \param dst an utf16 string in system-dependent endianness (i.e: little
     * endian in a little endian machine and big endian in a big endian one)
     * \param dstSize size in units of char16_t of dst, to prevent overflow
     * \param src a nul-terminated utf8 string
     * \return an error code and the length (in units of char16_t) of the
     * string written to dst
     */
    static std::pair<error,int> utf8toutf16(char16_t *dst, int dstSize,
        const char *src);
    
    /**
     * Convert an utf16 string in an utf8 one
     * \param dst an utf8 string
     * \param dstSize size in bytes of dst, to prevent overflow
     * \param src a nul-terminated utf16 string in system-dependent endianness
     * (i.e: little endian in a little endian machine and big endian in a big
     * endian one)
     * \return an error code and the length of the string written to dst
     */
    static std::pair<error,int> utf16toutf8(char *dst, int dstSize,
        const char16_t *src);
    
    /**
     * \param str an utf8 encoded string
     * \return a pair with a bool that is true if the string is valid, and the
     * string length in bytes, not code points
     */
    static std::pair<bool,int> validateUtf8(const char *str);

private:
    /**
     * Common implementation of nextUtf8
     * \param it an iterator into an utf8 encoded string
     * \param end iterator one past the last character of the string
     * \param checkEnd true if there is the need to check for end of string
     * considering end. If false, a nul in the char stream is the only end
     * condition.
     * \return an unicode code point, or Unicode::invalid if the string
     * contains an invalid code point. Returns 0 if the end of string is found,
     * and it is not in the middle of a character
     */
    template<typename Iter>
    static char32_t nextUtf8(Iter& it, Iter end, bool checkEnd);
};

template<typename Iter>
char32_t Unicode::nextUtf8(Iter& it, Iter end, bool checkEnd)
{
    //End of string at the beginning, return 0
    if(checkEnd && it==end) return 0;
    
    //Note: cast to unsigned char to prevent sign extension if *it > 0x7f
    char32_t c=static_cast<unsigned char>(*it++);
    
    //Common case first: ASCII
    if(c<0x80) return c;
    
    //If not ASCII, decode to utf32        
    int additionalBytes;
    if((c & 0xe0)==0xc0)      { c &= 0x1f; additionalBytes=1; } //110xxxxx
    else if((c & 0xf0)==0xe0) { c &= 0x0f; additionalBytes=2; } //1110xxxx
    else if((c & 0xf8)==0xf0) { c &= 0x07; additionalBytes=3; } //11110xxx
    else return invalid;
    for(int i=0;i<additionalBytes;i++)
    {
        //End of string in the middle of a char, return invalid
        if(checkEnd && it==end) return invalid;
        char32_t next=static_cast<unsigned char>(*it++);
        //This includes the case next==0
        if((next & 0xc0)!=0x80) return invalid;
        c<<=6;
        c |= next & 0x3f;
    }
    //Detect overlong encodings as errors to prevent vulnerabilities
    switch(additionalBytes)
    {
        case 1:
            if(c<0x80) return invalid;
            break;
        case 2:
            if(c<0x800) return invalid;
            break;
        case 3:
            if(c<0x10000) return invalid;
            break;
    }
    
    //Reserved space for surrogate pairs in utf16 are invalid code points
    if(c>=0xd800 && c<= 0xdfff) return invalid;
    //Unicode is limited in the range 0-0x10ffff
    if(c>0x10ffff) return invalid;
    return c;
}

} //namespace miosix

#endif //UNICODE_H


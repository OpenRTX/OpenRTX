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

#ifndef STRINGPART_H
#define	STRINGPART_H

#include <string>
#include <cstring>

namespace miosix {

/**
 * \internal
 * This class is used to take a substring of a string containing a file path
 * without creating a copy, and therefore requiring additional memory
 * allocation.
 * 
 * When parsing a path like "/home/test/directory/file" it is often necessary
 * to create substrings at the path component boundaries, such as "/home/test".
 * In this case, it is possible to temporarily make a substring by replacing a
 * '/' with a '\0'. The std::string will not 'forget' its orginal size, and
 * when the '\0' will be converted back to a '/', the string will look identical
 * to the previous one.
 * 
 * This is an optimization made for filesystem mountpoint lookups, and is not
 * believed to be useful outside of that purpose. Given that in Miosix, the
 * mountpoints are stored in a map, this class supports operator< to correctly
 * lookup mountpoints in the map.
 */
class StringPart
{
public:
    /**
     * Default constructor
     */
    StringPart() : cstr(&saved), index(0), offset(0), saved('\0'),
            owner(false), type(CSTR)
    {
        //We need an empty C string, that is, a pointer to a char that is \0,
        //so we make cstr point to saved, and set saved to \0.
    }
    
    /**
     * Constructor from C++ string
     * \param str original string. A pointer to the string is taken, the string
     * is NOT copied. Therefore, the caller is responsible to guarantee the
     * string won't be deallocated while this class is alive. Note that the
     * original string is modified by inserting a '\0' at the index position,
     * if index is given. The string will be restored to the exact original
     * content only when this class is destroyed.
     * \param idx if this parameter is given, this class becomes an in-place
     * substring of the original string. Otherwise, this class will store the
     * entire string passed. In this case, the original string will not be
     * modified.
     * \param off if this parameter is given, the first off characters of the
     * string are skipped. Note that idx calculations take place <b>before</b>
     * offset computation, so idx is relative to the original string.
     */
    explicit StringPart(std::string& str, size_t idx=std::string::npos,
               size_t off=0);
    
    /**
     * Constructor from C string
     * \param s original string. A pointer to the string is taken, the string
     * is NOT copied. Therefore, the caller is responsible to guarantee the
     * string won't be deallocated while this class is alive. Note that the
     * original string is modified by inserting a '\0' at the index position,
     * if index is given. The string will be restored to the exact original
     * content only when this class is destroyed.
     * \param idx if this parameter is given, this class becomes an in-place
     * substring of the original string. Otherwise, this class will store the
     * entire string passed. In this case, the original string will not be
     * modified.
     * \param off if this parameter is given, the first off characters of the
     * string are skipped. Note that idx calculations take place <b>before</b>
     * offset computation, so idx is relative to the original string.
     */
    explicit StringPart(char *s, size_t idx=std::string::npos, size_t off=0);
    
    /**
     * Constructor from const C string
     * \param s original string. A pointer to the string is taken, the string
     * is NOT copied. Therefore, the caller is responsible to guarantee the
     * string won't be deallocated while this class is alive. Note that this
     * constructor misses the idx and off parameters as it's not possible to
     * make an in-place substring of a string if it's const.
     */
    explicit StringPart(const char *s);
    
    /**
     * Substring constructor. Given a StringPart, produce another StringPart
     * holding a substring of the original StringPart. Unlike the normal copy
     * constructor that does deep copy, this one does shallow copy, and
     * therefore the newly created object will share the same string pointer
     * as rhs. Useful for making substrings of a substring without memory
     * allocation.
     * \param rhs a StringPart
     */
    StringPart(StringPart& rhs, size_t idx, size_t off=0);
    
    /**
     * Copy constructor. Note that deep copying is used, so that the newly
     * created StringPart is a self-contained string. It has been done like
     * that to be able to store the paths of mounted filesystems in an std::map
     * \param rhs a StringPart
     */
    StringPart(const StringPart& rhs);
    
    /**
     * Operator = Note that deep copying is used, so that the assigned
     * StringPart becomes a self-contained string. It has been done like
     * that to be able to store the paths of mounted filesystems in an std::map
     * \param rhs a StringPart
     */
    StringPart& operator= (const StringPart& rhs);
    
    /**
     * Compare two StringParts for inequality
     * \param rhs second StringPart to compare to
     * \return true if *this < rhs
     */
    bool operator<(const StringPart& rhs) const
    {
        return strcmp(this->c_str(),rhs.c_str())<0;
    }
    
    /**
     * \param rhs a StringPart
     * \return true if this starts with rhs
     */
    bool startsWith(const StringPart& rhs) const;
    
    /**
     * \param c char to find in the string, starting from the end
     * \return the index of the last occurrence of c, or string::npos
     */
    size_t findLastOf(char c) const;
    
    /**
     * \return the StringPart length, which is the same as strlen(this->c_str())
     */
    size_t length() const { return index-offset; }
    
    /**
     * \return the StringPart as a C string 
     */
    const char *c_str() const;
    
    /**
     * \param index index into the string
     * \return the equivalent of this->c_str()[index]
     */
    char operator[] (size_t index) const;
    
    /**
     * \return true if the string is empty
     */
    bool empty() const { return length()==0; }
    
    /**
     * Make this an empty string
     */
    void clear();
    
    /**
     * Destructor
     */
    ~StringPart() { clear(); }
    
private:
    /**
     * To implement copy constructor and operator=. *this must be empty.
     * \param rhs other StringPart that is assigned to *this.
     */
    void assign(const StringPart& rhs);
    
    union {
        std::string *str;  ///< Pointer to underlying C++ string
        char *cstr;        ///< Pointer to underlying C string
        const char *ccstr; ///< Pointer to underlying const C string
    };
    size_t index;        ///< Index into the character substituted by '\0'
    size_t offset;       ///< Offset to skip the first part of a string
    char saved;          ///< Char that was replaced by '\0'
    bool owner;          ///< True if this class owns str
    char type;           ///< either CPPSTR or CSTR. Using char to reduce size
    enum { CPPSTR, CSTR, CCSTR }; ///< Possible values fot type
};

} //namespace miosix

#endif //STRINGPART_H

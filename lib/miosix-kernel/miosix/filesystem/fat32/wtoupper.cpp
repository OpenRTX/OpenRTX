
#include "ff.h"
#include "config/miosix_settings.h"

/*
 * This is an alternative version of ff_wtoupper(), designed to be both smaller,
 * faster and to better conform to the unicode specification.
 * 
 * Code size using gcc 4.7.3, with
 * arm-miosix-eabi-g++ -mcpu=cortex-m3 -mthumb -O2 -fno-exceptions -fno-rtti \
 *                     -c wtoupper.cpp
 * is:
 * - ChaN's ff_wtoupper():         1000bytes
 * - fede.tft's enhanced version:   236bytes
 *
 * The design of this function is a bit tricky, as the usual way of making a
 * look up table is not optimized enough. It is old wisdom that a lut is
 * both faster and more space-efficient than a sequence of if, but unicode
 * conversion is somewhat peculiar. First, the input set is made of 0x10ffff
 * possible values, so the usual design that makes lut access O(1) would
 * require more than 2MB and is therefore out of question. However, the number
 * of characters that have an uppercase form is just around 1000, so the next
 * straightforward implementation would be to make a table of lowercase and
 * a table of upperacse characters. A character is checked against each entry
 * of the lowercase table, and if it matches the corresponding entry in the
 * upperacse table is returned, while if it matches none then the character
 * is either already uppercase, or does not have an uppercase form.
 * 
 * This works, but requires roughly 4KB of tables, and is not very fast as it
 * requires a for loop and a thousand comparisons per converted character.
 * The next thing to notice is that many characters to convert are in
 * contiguous ranges, which can be dealt with using an if statement per range.
 * There is a tradeoff, however, as the range needs to contain more than a
 * certain number of elements to be faster and/or result in less code size than
 * the lut approach. For this reason, it was selected to use an if for every
 * range of 7 or more code points, and use a final round with a look up table
 * to deal with too small ranges.
 * 
 * For what concerns unicode conformance, the result has been checked against
 * the file UnicodeData.txt downloaded from unicode's website, and the following
 * functional modifications have been done with respect to the original
 * ff_wtoupper():
 * - Code points 0x00a1, 0x00a2, 0x003, 0x00a5, 0x00ac, 0x00af are no longer
 *   converted. This is because they do not have an uppercase form
 * - Code point 0x00b5 is converted to 0x039c
 * - Code point 0x0131 is converted to 0x0049 and not 0x130
 * 
 * In addition, according to UnicodeData.txt there are many character that
 * were missing from the original implementation of ff_wtoupper(), but this was
 * not fixed, as it would lead to significantly larger tables.
 */

#ifdef WITH_FILESYSTEM

static const unsigned short lowerCase[]=
{
    0x00b5, 0x00ff, 0x0131, 0x0133, 0x0135, 0x0137, 0x017a, 0x017c,
    0x017e, 0x0192, 0x045e, 0x045f,
};

static const unsigned short upperCase[]=
{
    0x039c, 0x0178, 0x0049, 0x0132, 0x0134, 0x0136, 0x0179, 0x017b,
    0x017d, 0x0191, 0x040e, 0x040f,
};

static const int tabSize=sizeof(lowerCase)/sizeof(lowerCase[0]);

unsigned short ff_wtoupper(unsigned short c)
{
    if(c>='a' && c<='z') return c-('a'-'A');               //26 code points

    if(c<0x80) return c;//Speed hack: there are no other lowercase char in ASCII

    if(c>=0x00e0 && c<=0x00f6) return c-(0x00e0-0x00c0);   //23 code points

    if(c>=0x00f8 && c<=0x00fe) return c-(0x00f8-0x00d8);   // 7 code points

    if(c>=0x0101 && c<=0x012f && (c & 1)) return c-1;      //24 code points

    if(c>=0x013a && c<=0x0148 && ((c & 1)==0)) return c-1; // 8 code points

    if(c>=0x014b && c<=0x0177 && (c & 1)) return c-1;      //23 code points

    if(c>=0x03b1 && c<=0x03c1) return c-(0x03b1-0x0391);   //17 code points

    if(c>=0x03c3 && c<=0x03ca) return c-(0x03c3-0x03a3);   // 8 code points

    if(c>=0x0430 && c<=0x044f) return c-(0x0430-0x0410);   //32 code points

    if(c>=0x0451 && c<=0x045c) return c-(0x0451-0x0401);   //12 code points

    if(c>=0x2170 && c<=0x217f) return c-(0x2170-0x2160);   //16 code points

    if(c>=0xff41 && c<=0xff5a) return c-(0xff41-0xff21);   //26 code points

    for(int i=0;i<tabSize;i++) if(lowerCase[i]==c) return upperCase[i];
    return c;
}

#endif //WITH_FILESYSTEM

/*
//Print all characters that have an uppercase

#include <iostream>
#include <iomanip>

using namespace std;

int main()
{
    for(unisgned short i=0;i<0x10000;i++)
    {
        unisgned short up=ff_wtoupper(i);
        if(up==i) continue;
        cout<<hex<<uppercase<<setw(4)<<setfill('0')<<i
            <<" "<<setw(4)<<setfill('0')<<up<<endl;
    }
}

//Crawl UnicodeData.txt making a table of code points and their uppercase

#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>

using namespace std;

enum {
    codePoint=0,
    upperCase=12,
    lowerCase=13
};

int main()
{
    ifstream in("UnicodeData.txt");
    string line;
    int lineno=0;
    while(getline(in,line))
    {
        lineno++;
        stringstream ss(line);
        vector<string> fields;
        string field;
        while(getline(ss,field,';')) fields.push_back(field);
        if(fields.at(upperCase).empty()==false &&
           fields.at(codePoint)!=fields.at(upperCase))
            cout<<fields.at(codePoint)<<" "<<fields.at(upperCase)<<endl;
    }
}
*/

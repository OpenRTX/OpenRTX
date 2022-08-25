
#include "crc16.h"

namespace miosix {

/**
 * 16 bit ccitt crc calculation
 * \param crc the firs time the function is called, pass 0xffff, all the other
 * times, pass the previous value. The value returned after the last call is
 * the desired crc
 * \param data a byte of the message
 */
static inline void crc16Update(unsigned short& crc, unsigned char data)
{
    unsigned short x=((crc>>8) ^ data) & 0xff;
    x^=x>>4;
    crc=(crc<<8) ^ (x<<12) ^ (x<<5) ^ x;
}

unsigned short crc16(const void *message, unsigned int length)
{
    const unsigned char *m=reinterpret_cast<const unsigned char*>(message);
    unsigned short result=0xffff;
    for(unsigned int i=0;i<length;i++) crc16Update(result,m[i]);
    return result;
}

} //namespace miosix

//Testcase
/*#include <iostream>
#include <boost/crc.hpp>

using namespace std;

int main()
{
    unsigned char test[]="Some test string";
    cout<<"0x"<<hex<<miosix::crc16(test,sizeof(test))<<endl;

    boost::crc_ccitt_type crc;
	crc.process_bytes(test,sizeof(test));
    cout<<"0x"<<hex<<crc.checksum()<<endl;
}*/

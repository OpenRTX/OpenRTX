
#include <stdio.h>
#include <stdarg.h>
#include "miosix.h"
#include "interfaces/delays.h"
#include "lcd44780.h"

namespace miosix {

Lcd44780::Lcd44780(miosix::GpioPin rs, miosix::GpioPin e, miosix::GpioPin d4,
             miosix::GpioPin d5, miosix::GpioPin d6, miosix::GpioPin d7,
             int row, int col) : rs(rs), e(e), d4(d4), d5(d5), d6(d6), d7(d7),
             row(row), col(col)
{
    rs.mode(Mode::OUTPUT);
    e.mode(Mode::OUTPUT);
    d4.mode(Mode::OUTPUT);
    d5.mode(Mode::OUTPUT);
    d6.mode(Mode::OUTPUT);
    d7.mode(Mode::OUTPUT);
    e.low();
    Thread::sleep(50); //Powerup delay
    init();
    clear();
}

void Lcd44780::go(int x, int y)
{
    if(x<0 || x>=col || y<0 || y>=row) return;

    // 4x20 is implemented as 2x40.
    if(y>1) x += col;

    comd(0x80 | ((y & 1) ? 0x40 : 0) | x); //Move cursor
}

int Lcd44780::iprintf(const char* fmt, ...)
{
    va_list arg;
    char line[40];
    va_start(arg,fmt);
    int len=vsniprintf(line,sizeof(line),fmt,arg);
    va_end(arg);
    for(int i=0;i<len;i++) data(line[i]);
    return len;
}

int Lcd44780::printf(const char* fmt, ...)
{
    va_list arg;
    char line[40];
    va_start(arg,fmt);
    int len=vsnprintf(line,sizeof(line),fmt,arg);
    va_end(arg);
    for(int i=0;i<len;i++) data(line[i]);
    return len;
}

void Lcd44780::clear()
{
    comd(1);
    Thread::sleep(2); //Some displays require this delay
}

void Lcd44780::off()
{
    comd(8);
}

void Lcd44780::on()
{
    comd(12 | cursorState);
}

void Lcd44780::cursor(Cursor cursor)
{
    cursorState=cursor & 0x3;
    comd(12 | cursorState);
}

void Lcd44780::setCgram(int charCode, const unsigned char font[8])
{
    comd(64 | (charCode & 0x7)<<3);
    for(int i=0;i<8;i++) data(font[i]);
    comd(0x80);
}

void Lcd44780::init()
{
    rs.low();
    half(0x20);
    rs.high();
    delayUs(50);
    if(row==1) comd(32); else comd(40);
    Thread::sleep(5);  //Initialization delay
    comd(12 | cursorState); //Display ON, cursor OFF, blink OFF
    comd(6);  //Auto increment ON, shift OFF
}

void Lcd44780::half(unsigned char byte)
{
    if(byte & (1<<7)) d7.high(); else d7.low(); //Not much fast, but works
    if(byte & (1<<6)) d6.high(); else d6.low();
    if(byte & (1<<5)) d5.high(); else d5.low();
    if(byte & (1<<4)) d4.high(); else d4.low();
    delayUs(1);
    e.high();
    delayUs(1);
    e.low();
}

void Lcd44780::data(unsigned char byte)
{
    half(byte);
    byte<<=4;
    half(byte);
    delayUs(50);
}

void Lcd44780::comd(unsigned char byte)
{
    rs.low();
    half(byte);
    byte<<=4;
    half(byte);
    delayUs(50);
    rs.high();
}

} //namespace miosix

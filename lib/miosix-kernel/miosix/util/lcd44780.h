
#include "interfaces/gpio.h"

#ifndef LCD44780_H
#define LCD44780_H

namespace miosix {

/**
 * Class to control an HD44780 compatible LCD
 * 
 */
class Lcd44780
{
public:
    /**
     * Constructor, initializes the display
     * \param rs a Gpio class specifying the GPIO connected to the rs signal
     * \param e  a Gpio class specifying the GPIO connected to the e  signal
     * \param d4 a Gpio class specifying the GPIO connected to the d4 signal
     * \param d5 a Gpio class specifying the GPIO connected to the d5 signal
     * \param d6 a Gpio class specifying the GPIO connected to the d6 signal
     * \param d7 a Gpio class specifying the GPIO connected to the d7 signal
     * \param row number of rows of the display
     * \param col number of columns of the display
     */
    Lcd44780(GpioPin rs, GpioPin e, GpioPin d4, GpioPin d5, GpioPin d6,
             GpioPin d7, int row, int col);
    
    /**
     * Set the display cursor
     * \param x a value from 0 to col-1
     * \param y a value from 0 to row-1
     */
    void go(int x, int y);
    
    /**
     * Print to the display
     */
    int printf(const char *fmt, ...);
    
    /**
     * Clear the screen 
     */
    void clear();
    
    /**
     * \return the number of rows of the display 
     */
    int getRow() const { return row; }
    
    /**
     * \return the number of columns of the display 
     */
    int getCol() const { return col; }
        
private:
    
    void init();
    
    void half(unsigned char byte);

    void data(unsigned char byte);

    void comd(unsigned char byte);
    
    GpioPin rs;
    GpioPin e;
    GpioPin d4;
    GpioPin d5;
    GpioPin d6;
    GpioPin d7;
    const int row;
    const int col;
};

} //namespace miosix

#endif //LCD44780_H

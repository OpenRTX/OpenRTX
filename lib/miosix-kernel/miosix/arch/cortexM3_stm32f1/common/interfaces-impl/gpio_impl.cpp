
#include "gpio_impl.h"

namespace miosix {

void GpioPin::mode(Mode::Mode_ m)
{
    if(n<8)
    {
        p->CRL &= ~(0xf<<(n*4));
        p->CRL |= m<<(n*4);
    } else {
        p->CRH &= ~(0xf<<((n-8)*4));
        p->CRH |= m<<((n-8)*4);
    }
}

} //namespace miosix

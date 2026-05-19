/***************************************************************************
 *   Copyright (C) 2016 by Lorenzo Pinosa                                  *
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

#include "../filesystem/devfs/devfs.h"
#include "queue.h"
#include <vector>
#include "kernel.h"

using namespace std;

namespace miosix {

class IRQDisplayPrint : public Device
{
public:
    IRQDisplayPrint();
    ~IRQDisplayPrint();
    
    void IRQwrite(const char *str);
    ssize_t writeBlock(const void *buffer, size_t size, off_t where);

    void printIRQ();
private:
    Queue<string, 20> input_queue;
    vector<string> print_lines;

    int right_margin;
    int bottom_margin;
    bool carriage_return_enabled;

    bool carriage_return_fix(string str);
    void process_string(string str);
    void check_array_overflow();
    void internal_print();
};

} //namespace miosix

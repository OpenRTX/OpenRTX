/***************************************************************************
 *   Copyright (C) 2021 by Federico Amedeo Izzo IU2NUO,                    *
 *                         Niccolò Izzo IU2KIN                             *
 *                         Frederik Saraci IU2NRO                          *    
 *                         Silvano Seva IU2KWO                             *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 3 of the License, or     *
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

#ifndef UI_WIDGET_H
#define UI_WIDGET_H


#ifndef __cplusplus
#error This header is C++ only!
#endif

/**
 * Class that represent a reusable component of the UI
 * Example: ListWidget, TextInputWidget,...
 */
class Widget
{
public:
    
    /**
     * Constructor
     * 
     * @param origin: position starting point for graphic elements of this widget
     * @param site: size available for graphic elements of this widget
     */
    Widget(point_t origin, point_t size) : origin(origin), size(size) {}
    
    /**
     * Base class constructor
     */
    virtual ~Widget(){}
    
    /**
     * Base class draw method
     */
    virtual void draw() = 0;
    
    /**
     * Base class event method
     */
    virtual void event(event_t event) = 0;

private:
    point_t origin, size;
};

#endif /* UI_WIDGET_H */

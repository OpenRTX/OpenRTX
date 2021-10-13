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

#ifndef UI_TOOLKIT_H
#define UI_TOOLKIT_H

#ifndef __cplusplus
#error This header is C++ only!
#endif

#include <array>
#include "ui_widget.h"

/**
 * Class that manages the execution and switch of UI elements
 */
class UIManager
{
public:
    /**
     * Constructor.
     */
    UIManager();

    /**
     * Destructor.
     */
    ~UIManager();

    /**
     * Replaces the current active UI Element with another one
     * This methods disables the current UI Element and enables the new one.
     *
     * @param index: index of the new UI Element to activate, replacing the current one
     */
    void switchElement(const unsigned int elem);

    /**
     * Returns a reference to the array of available elements
     * To be used by MenuElement to get list of callable applications
     */
    const std::array<Element>& getElements();

    /**
     * Renders the active view of the active element
     */
    void draw();
    
    /**
     * Passes an event to the active element
     *
     * @param event: The event to be passed
     */
    void event(event_t event);
    
private:
    
    /**
     * Used to create a preset list of Elements, containing the functionality
     * built-in OpenRTX
     */
    void createElementList();

    std::array<Element> elements;    ///< Array of the available Elements
    unsigned int active;             ///< Index of the active Element
    unsigned int last;               ///< Index of the previous active Element
};


/**
 * Class that corresponds to a self-contained UI functionality
 * Example: Main menu, Main screen, GPS menu...
 * It is composed by one or more views which contain zero or more Widgets
 */
class Element
{
public:
    
    /**
     * Constructor.
     */

    Element();
    
    /**
     * Destructor.
     */
    virtual ~Element(){}
    
    /**
     * Show the element on screen, resetting its state if needed
     */
    void enable();
    
    /**
     * Remove the element from the screen
     */
    void disable();
    
    /**
     * Render the active view
     */
    void draw();
    
    /**
     * Process an event and pass it to the active View
     *
     * @param event: The event to be passed
     */
    void event(event_t event);

private:
    std::array<View> viewList; ///< Array of the views composing the Element
    unsigned int activeView;   ///< Index of the currently active view
};

/**
 * Class that consists in a group of widgets shown at the same time on the screen
 * Tipically used to create a single page of a UI Element
 * It contains zero or more Widgets
 */
class View
{
public:
    
    /**
     * Constructor.
     */
    View(std::string title, 
         point_t origin = {0, 0}, 
         point_t size = {SCREEN_WIDTH, SCREEN_HEIGHT}) : title(title), 
                                                         origin(origin), 
                                                         size(size);
    
    /**
     * Destructor.
     */
    virtual ~View(){}
    
    /**
     * Add a widget to the View
     */
    void addWidget(Widget widget);
    
    /**
     * Renders this view and the widgets it contains
     */
    void draw();
    
    /**
     * Passes an event to the widgets it contains
     *
     * @param event: The event to be passed
     */
    void event(event_t event);

private:
    std::string title;              ///< Title of the View
    point_t origin, size;           ///< Origin and size of the view
    std::array<Widget> widgetList;  ///< Array of the widgets contained
};

#endif /* UI_TOOLKIT_H */

/***************************************************************************
 *   Copyright (C) 2020 - 2024 by Federico Amedeo Izzo IU2NUO,             *
 *                                Niccolò Izzo IU2KIN,                     *
 *                                Silvano Seva IU2KWO                      *
 *                                Morgan Diepart ON4MOD                    *
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
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, see <http://www.gnu.org/licenses/>   *
 ***************************************************************************/

#ifndef USER_FUNCTIONS_H
#define USER_FUNCTIONS_H

#include <hwconfig.h>
#include <stdint.h>
#include <stddef.h>
#include <errno.h>

/**
 * @file
 * @brief Defines user functions. User functions will be called by a specific thread either periodically or when triggered.
 * 
 * The user functions module will allow user to register functions to be called periodically or when triggered.
 * User must first define the CONFIG_USER_FUNCTIONS macro with the number of user functions expected to be used
 * in the code. In subsequent calls to user_functions_* functions the id to be provided is limited to CONFIG_USER_FUNCTIONS-1. 
 */

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @enum Defines the possible schedulings for user functions
 */
typedef enum {
    USER_FUNCTION_ASYNC = 0,    /**enum Functions will only be executed when triggered */
    USER_FUNCTION_100HZ = 10,   /**enum Function will be executed at a 100 Hz frequency */
    USER_FUNCTION_50HZ  = 20,   /**enum Function will be executed at a 50 Hz frequency */
    USER_FUNCTION_20HZ  = 50,   /**enum Function will be executed at a 20 Hz frequency */
    USER_FUNCTION_10HZ  = 100,  /**enum Function will be executed at a 10 Hz frequency */
    USER_FUNCTION_1HZ   = 1000, /**enum Function will be executed at a 1 Hz frequency */
} user_functions_sched_t;

/**
 * Initializes user_functions module. 
 */
void user_functions_init();

/**
 * Clean up and terminates the user_functions module.
 */
void user_functions_terminate();

/**
 * Core function of the user_functions module. Must be called as the core of the user_functions thread.
 */
void user_functions_task();

/**
 * Add a user function to the user_functions module.
 * The function will be added and disabled by default.
 * 
 * @param id: ID associated with the user function
 * @param f: function to be called
 * @param arg: void pointer that will be passed as argument when calling f
 * @param scheduling: scheduling for the associated user function
 * 
 * @return EINVAL if id is above the max id defined by CONFIG_USER_FUNCTIONS
 * @return EADDRINUSE if id is already in use
 * @return 0 if no error
 */
error_t user_functions_add(uint8_t id, void (*f)(), void *arg, user_functions_sched_t scheduling);

/**
 * Unregisters a function from the user_functions module.
 * 
 * @param id: ID associated with the user function to remove
 * 
 * @return EINVAL if id is above the max id defined by CONFIG_USER_FUNCTIONS
 * @return 0 if no error
 */
error_t user_functions_remove(uint8_t id);

/**
 * Triggers an async user function to be executed
 * 
 * @param id: ID of the function to trigger
 * 
 * @return EINVAL if id is above the max id defined by CONFIG_USER_FUNCTIONS
 * @return 0 if no error
 */
error_t user_functions_trigger(uint8_t id);

/**
 * Enable a user function. 
 * A user function can not be executed unless it is enabled.
 * 
 * @param id: ID of the function to enable
 * 
 * @return EINVAL if id is above the max id defined by CONFIG_USER_FUNCTIONS
 * @return 0 if no error
 */
error_t user_functions_enable(uint8_t id);

/**
 * Disable a user function
 * A disabled user function is not unregistered but won't be executed until it 
 * is enabled again.
 * 
 * @param id: ID of the function to disable
 * 
 * @return EINVAL if id is above the max id defined by CONFIG_USER_FUNCTIONS
 * @return 0 if no error
 */
error_t user_functions_disable(uint8_t id);

#ifdef __cplusplus
}
#endif

#endif /* USER_FUNCTIONS_H */

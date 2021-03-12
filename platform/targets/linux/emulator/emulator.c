/***************************************************************************
 *   Copyright (C) 2020 by Federico Amedeo Izzo IU2NUO,                    *
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
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, see <http://www.gnu.org/licenses/>   *
 ***************************************************************************/


#include "emulator.h"
#include <pthread.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>

radio_state Radio_State = {12, 8.2f, 3, 4, 1, false};

int CLIMenu()
{
    int choice = 0;
    printf("Select the value to change:\n");
    printf("1 -> RSSI\n");
    printf("2 -> Vbat\n");
    printf("3 -> Mic Level\n");
    printf("4 -> Volume Level\n");
    printf("5 -> Channel selector\n");
    printf("6 -> Toggle PTT\n");
    printf("7 -> Print current state\n");
    printf("8 -> Exit\n");
    printf("> ");
    do
    {
        scanf("%d", &choice);
    } while (choice < 1 || choice > 8);
    printf("\033[1;1H\033[2J");
    return choice;
}

void updateValue(float *curr_value)
{
    printf("Current value: %f\n", *curr_value);
    printf("New value: \n");
    scanf("%f", curr_value);
}

void printState()
{
    printf("\nCurrent state\n");
    printf("RSSI   : %f\n", Radio_State.RSSI);
    printf("Battery: %f\n", Radio_State.Vbat);
    printf("Mic    : %f\n", Radio_State.micLevel);
    printf("Volume : %f\n", Radio_State.volumeLevel);
    printf("Channel: %f\n", Radio_State.chSelector);
    printf("PTT    : %s\n\n", Radio_State.PttStatus ? "true" : "false");

}

void *startCLIMenu()
{
    int choice;
    do
    {
        choice = CLIMenu();
        switch (choice)
        {
            case VAL_RSSI:
                updateValue(&Radio_State.RSSI);
                break;
            case VAL_BAT:
                updateValue(&Radio_State.Vbat);
                break;
            case VAL_MIC:
                updateValue(&Radio_State.micLevel);
                break;
            case VAL_VOL:
                updateValue(&Radio_State.volumeLevel);
                break;
            case VAL_CH:
                updateValue(&Radio_State.chSelector);
                break;
            case VAL_PTT:
                Radio_State.PttStatus = Radio_State.PttStatus ? false : true;
                break;
            case PRINT_STATE:
                printState();
                break;
            default:
                continue;
        }
    } while (choice != EXIT);
    printf("73\n");
    exit(0);
}


void emulator_start()
{
    pthread_t cli_thread;
    int err = pthread_create(&cli_thread, NULL, startCLIMenu, NULL);

    if(err)
    {
        printf("An error occurred starting the emulator thread: %d\n", err);
    }
}

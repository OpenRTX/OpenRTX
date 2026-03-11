#pragma once

#include <stdint.h>

#define portYIELD_FROM_ISR(x)

#define portSTACK_TYPE										uint32_t

#define configTICK_RATE_HZ              CONFIG_SYS_CLOCK_TICKS_PER_SEC

typedef void (*TaskFunction_t)( void * );
typedef portSTACK_TYPE										StackType_t;
typedef long												BaseType_t;
typedef unsigned long										UBaseType_t;

typedef uint32_t TickType_t;
#define portMAX_DELAY ( TickType_t )					0xffffffffUL

#define configSTACK_DEPTH_TYPE uint16_t

#define pdFALSE			( ( BaseType_t ) 0 )
#define pdTRUE			( ( BaseType_t ) 1 )

#define pdPASS			( pdTRUE )
#define pdFAIL			( pdFALSE )
#define errQUEUE_EMPTY	( ( BaseType_t ) 0 )
#define errQUEUE_FULL	( ( BaseType_t ) 0 )
#define errQUEUE_EMPTY	( ( BaseType_t ) 0 )
#define errQUEUE_FULL	( ( BaseType_t ) 0 )

/* FreeRTOS error definitions. */
#define errCOULD_NOT_ALLOCATE_REQUIRED_MEMORY	( -1 )
#define errQUEUE_BLOCKED						( -4 )
#define errQUEUE_YIELD							( -5 )


/* FreeRTOSConfig.h */
#define configMAX_PRIORITIES							(14)


#define pdMS_TO_TICKS( xTimeInMs ) ( ( TickType_t ) ( ( ( TickType_t ) ( xTimeInMs ) * ( TickType_t ) configTICK_RATE_HZ ) / ( TickType_t ) 1000 ) )

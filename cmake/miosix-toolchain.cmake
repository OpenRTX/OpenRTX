set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM_PROCESSOR cortex-m4)
set(triple arm-miosix-eabi)

set(MIOSIX_ROOT /opt/arm-miosix-eabi CACHE PATH "Miosix root directory")
#set(CMAKE_SYSROOT ${MIOSIX_ROOT})

set(CMAKE_C_COMPILER ${MIOSIX_ROOT}/bin/arm-miosix-eabi-gcc CACHE FILEPATH "Miosix C compiler")
set(CMAKE_CXX_COMPILER ${MIOSIX_ROOT}/bin/arm-miosix-eabi-g++ CACHE FILEPATH "Miosix C++ compiler")
set(CMAKE_LINKER ${MIOSIX_ROOT}/bin/arm-miosix-eabi-ld CACHE FILEPATH "Miosix linker")
set(CMAKE_AR ${MIOSIX_ROOT}/bin/arm-miosix-eabi-ar CACHE FILEPATH "Miosix archiver")
set(CMAKE_AS ${MIOSIX_ROOT}/bin/arm-miosix-eabi-as CACHE FILEPATH "Miosix assembler")
set(CMAKE_SIZE ${MIOSIX_ROOT}/bin/arm-miosix-eabi-size CACHE FILEPATH "Miosix size")
set(CMAKE_OBJCOPY ${MIOSIX_ROOT}/bin/arm-miosix-eabi-objcopy CACHE FILEPATH "Miosix objcopy")
set(CMAKE_OBJDUMP ${MIOSIX_ROOT}/bin/arm-miosix-eabi-objdump CACHE FILEPATH "Miosix objdump")
set(CMAKE_RANLIB ${MIOSIX_ROOT}/bin/arm-miosix-eabi-ranlib CACHE FILEPATH "Miosix ranlib")
set(CMAKE_STRIP ${MIOSIX_ROOT}/bin/arm-miosix-eabi-strip CACHE FILEPATH "Miosix strip")

SET(CMAKE_C_FLAGS   "-D_DEFAULT_SOURCE=1 -ffunction-sections -fdata-sections -Wall -Werror=return-type -mcpu=cortex-m4 -mthumb -mfloat-abi=hard -mfpu=fpv4-sp-d16 -Os" CACHE STRING "" FORCE)
SET(CMAKE_CXX_FLAGS "-D_DEFAULT_SOURCE=1 -ffunction-sections -fdata-sections -Wall -Werror=return-type -fno-exceptions -fno-rtti -mcpu=cortex-m4 -mthumb -mfloat-abi=hard -mfpu=fpv4-sp-d16 -D__NO_EXCEPTIONS -Os" CACHE STRING "" FORCE)

SET(CMAKE_EXE_LINKER_FLAGS "-mcpu=cortex-m4 -mthumb -mfloat-abi=hard -mfpu=fpv4-sp-d16 -ffunction-sections -fdata-sections -fno-exceptions -fno-rtti -Wl,-L../lib/miosix-kernel -Wl,--gc-sections -Wl,-Map,main.map -Wl,--warn-common -nostdlib -lstdc++ -lc -lm -lgcc -latomic" CACHE STRING "" FORCE)

set(CMAKE_C_STANDARD_COMPUTED_DEFAULT 11)
set(CMAKE_CXX_STANDARD_COMPUTED_DEFAULT 14)

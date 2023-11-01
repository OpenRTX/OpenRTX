# Define source files
set(miosix_src
    miosix/kernel/scheduler/priority/priority_scheduler.cpp
    miosix/kernel/error.cpp
    miosix/kernel/kernel.cpp
    miosix/kernel/pthread.cpp
    miosix/kernel/stage_2_boot.cpp
    miosix/kernel/sync.cpp
    miosix/kernel/timeconversion.cpp
    miosix/util/util.cpp
    miosix/stdlib_integration/libc_integration.cpp
    miosix/stdlib_integration/libstdcpp_integration.cpp
)

# Define include directories
set(miosix_inc
    miosix
    miosix/config
    miosix/arch/common
    miosix/util
)

#if(1) # Add ARM Cortex M4 specific files and include directories
    list(APPEND miosix_src
        miosix/arch/cortexM4F/portability.cpp
        miosix/arch/common/core/interrupts_cortexMx.cpp
    )

    list(APPEND miosix_inc
        miosix/arch/cortexM4F
    )
#endif()

# Create the static library
add_library(miosix STATIC ${miosix_src})

# Set include directories for the library
target_include_directories(miosix PUBLIC ${miosix_inc})

# Define compile definitions
target_compile_definitions(miosix PUBLIC
    -DDONT_USE_CMSIS_INIT=1
    -DCOMPILING_MIOSIX=1
    -D_POSIX_PRIORITY_SCHEDULING=1
    -D_ARCH_CORTEXM4=1
)

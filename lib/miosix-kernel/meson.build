
miosix_inc = ['lib/miosix-kernel/miosix',
              'lib/miosix-kernel/miosix/config',
              'lib/miosix-kernel/miosix/arch/common',
              'lib/miosix-kernel/miosix/util']

miosix_src = ['lib/miosix-kernel/miosix/kernel/scheduler/priority/priority_scheduler.cpp',
              'lib/miosix-kernel/miosix/kernel/error.cpp',
              'lib/miosix-kernel/miosix/kernel/kernel.cpp',
              'lib/miosix-kernel/miosix/kernel/pthread.cpp',
              'lib/miosix-kernel/miosix/kernel/stage_2_boot.cpp',
              'lib/miosix-kernel/miosix/kernel/sync.cpp',
              'lib/miosix-kernel/miosix/kernel/timeconversion.cpp',
              'lib/miosix-kernel/miosix/util/util.cpp',
              'lib/miosix-kernel/miosix/stdlib_integration/libc_integration.cpp',
              'lib/miosix-kernel/miosix/stdlib_integration/libstdcpp_integration.cpp']

miosix_def = {'DONT_USE_CMSIS_INIT'       : '',
              'COMPILING_MIOSIX'          : '',
              '_POSIX_PRIORITY_SCHEDULING': ''}

##
## ARM Cortex M4 with FPU
##
miosix_cm4f_inc = miosix_inc + ['lib/miosix-kernel/miosix/arch/cortexM4F']
miosix_cm4f_src = miosix_src + ['lib/miosix-kernel/miosix/arch/cortexM4F/portability.cpp',
                               'lib/miosix-kernel/miosix/arch/common/core/interrupts_cortexMx.cpp']
miosix_cm4f_def = miosix_def + {'_ARCH_CORTEXM4' : ''}

##
## ARM Cortex M4 without FPU
##
miosix_cm4_inc = miosix_inc + ['lib/miosix-kernel/miosix/arch/cortexM4']
miosix_cm4_src = miosix_src + ['lib/miosix-kernel/miosix/arch/cortexM4/portability.cpp',
                               'lib/miosix-kernel/miosix/arch/common/core/interrupts_cortexMx.cpp']
miosix_cm4_def = miosix_def + {'_ARCH_CORTEXM4' : ''}

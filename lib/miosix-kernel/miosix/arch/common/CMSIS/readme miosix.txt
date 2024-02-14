Changes with respect to upstream CMSIS:

- Removed RTOS and Documentation/RTOS directories as Miosix does not try to
  conform to the cmsis_rtos API, as it has a bigger standard to refer to, POSIX
- Removed SVD/SVDConv.exe as it is a closed source windows-only exe program.
  Development for Miosix must be fully supported from Linux, and no closed
  source binaries are allowed in the kernel tree.
- Removed assembler written startup code in Device/ST/*/Source/Templates.
  Miosix provides its own startup code, written in C++. This is because the
  interrupt handlers in Miosix need to have C++ linkage and mangled names.
  As a deliberate side effect, in Miosix it is not possible to declare an
  interrupt handler in a C source file, as Miosix is first and foremost a
  kernel written in C++.

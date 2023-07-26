.section .note.GNU-stack,""
.section .rodata

.global _vpdata_start
.global _vpdata_end

_vpdata_start:
    .incbin "../voiceprompts.vpc"

.set _vpdata_end, .

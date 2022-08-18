.section .note.GNU-stack,""
.section .rodata

.global _voiceprompts_start
.global _voiceprompts_end

_voiceprompts_start:
    .incbin "../voiceprompts.vpc"
_voiceprompts_end:

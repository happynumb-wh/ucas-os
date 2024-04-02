.section .data
.align 3
.global dtb_start, dtb_end
dtb_start:
.incbin "system.dtb"
dtb_end:

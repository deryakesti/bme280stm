/* STM32F407VGT6 minimal startup – GNU AS */

    .syntax unified
    .cpu cortex-m4
    .fpu fpv4-sp-d16
    .thumb

/* Stack size (bytes) */
    .equ    Stack_Size, 0x800

    .section .stack, "w"
    .align 3
    .space Stack_Size
stack_top:

/* Heap size */
    .equ    Heap_Size, 0x400

    .section .heap, "w"
    .align 3
heap_base:
    .space Heap_Size
heap_top:

/* Vector table */
    .section .isr_vector, "a"
    .type  g_pfnVectors, %object
g_pfnVectors:
    .word  stack_top
    .word  Reset_Handler
    .word  NMI_Handler
    .word  HardFault_Handler
    .word  MemManage_Handler
    .word  BusFault_Handler
    .word  UsageFault_Handler
    .word  0
    .word  0
    .word  0
    .word  0
    .word  SVC_Handler
    .word  DebugMon_Handler
    .word  0
    .word  PendSV_Handler
    .word  SysTick_Handler
    /* External interrupts — only what we need, rest default */
    .rept  91
    .word  Default_Handler
    .endr

    .size g_pfnVectors, . - g_pfnVectors

/* Reset handler */
    .section .text.Reset_Handler
    .weak   Reset_Handler
    .type   Reset_Handler, %function
Reset_Handler:
    /* Copy .data from Flash to SRAM */
    ldr  r0, =_sdata
    ldr  r1, =_edata
    ldr  r2, =_sidata
    movs r3, #0
    b    copy_loop_check
copy_loop:
    ldr  r4, [r2, r3]
    str  r4, [r0, r3]
    adds r3, r3, #4
copy_loop_check:
    adds r4, r0, r3
    cmp  r4, r1
    bcc  copy_loop

    /* Zero .bss */
    ldr  r2, =_sbss
    ldr  r4, =_ebss
    movs r3, #0
    b    bss_loop_check
bss_loop:
    str  r3, [r2]
    adds r2, r2, #4
bss_loop_check:
    cmp  r2, r4
    bcc  bss_loop

    bl   SystemInit
    bl   main
    b    .

    .size Reset_Handler, . - Reset_Handler

/* Weak default handlers */
    .macro weak_alias name
    .weak \name
    .thumb_set \name, Default_Handler
    .endm

    .section .text.Default_Handler, "ax"
Default_Handler:
    b  Default_Handler
    .size Default_Handler, . - Default_Handler

    weak_alias NMI_Handler
    weak_alias HardFault_Handler
    weak_alias MemManage_Handler
    weak_alias BusFault_Handler
    weak_alias UsageFault_Handler
    weak_alias SVC_Handler
    weak_alias DebugMon_Handler
    weak_alias PendSV_Handler
    weak_alias SysTick_Handler

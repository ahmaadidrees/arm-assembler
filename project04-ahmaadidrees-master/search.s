.global search_s

search_s:
    sub sp, sp, #20
    str lr, [sp]
    cmp r2, r1
    blt input_error
    sub r5, r2, r1
    lsr r5, r5, #1
    add r5, r5, r1
    lsl r6, r5, #2
    ldr r4, [r0, r6]
    cmp r4, r3
    beq if_statement
    bgt else_if_statement
    b else_statement 
if_statement:
    mov r0, r5
    beq search_exit
else_if_statement:
    sub r5, r5, #1
    mov r2, r5
    b search_recurse
else_statement:
    add r5, r5, #1
    mov r1, r5
search_recurse:
    bl search_s
    b search_exit
input_error:
    mov r0, #-1
search_exit:
    ldr lr, [sp]
    add sp, sp, #20
    bx lr

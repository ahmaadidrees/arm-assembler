@ r0 = array
@ r1 = len
@ r2 = i
@ r3 = second i
@ r4 = max
@ r6 = array[i]

.global sort_s

sort_s:
    sub sp, sp, #20
    str lr, [sp]
    mov r2, #0
begin_loop:
    cmp r2, r1
    bge end
    mov r3, #1
    mov r5, #0
    str r0, [sp, #4]
    str r1, [sp, #8]
    str r2, [sp, #12]
    lsl r2, r2, #2
    add r0, r0, r2
    sub r1, r1, r2
    ldr r4, [r0]
    bl find_max_index_s
    mov r8, r4
    mov r9, r0
    ldr r2, [sp, #12]
    ldr r1, [sp, #8]
    ldr r0, [sp, #4]
    add r9, r9, r2 
    cmp r9, r2
    beq equal
    lsl r2, r2, #2
    ldr r7, [r0, r2]
    lsl r2, r2, #2
    str r8, [r0, r2]
    lsl r9, r9, #2
    str r7, [r0, r9]
equal:
    add r2, r2, #1
    b begin_loop
end: 
   mov r0, r1
   ldr lr, [sp]
   add sp, sp, #20
   bx lr
find_max_index_s:
   cmp r3, r1
   bge max_end
   lsl r3,r3 ,#2
   ldr r6, [r0, r3]
   cmp r6, r4
   bgt update
   b find_max_loop
update:
   mov r4, r6
   mov r5, r3
find_max_loop:   
   add r3, r3, #1
   b find_max_index_s
max_end:
   mov r0, r5
   bx lr


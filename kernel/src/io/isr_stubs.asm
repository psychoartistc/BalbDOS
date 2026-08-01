bits 64

%macro EXCEPTION_NO_ERR_CODE 1
global exception_handler_%1
exception_handler_%1:
    push qword 0
    push qword %1
    jmp common_exception_wrapper
%endmacro


%macro EXCEPTION_WITH_ERR_CODE 1
global exception_handler_%1
exception_handler_%1:
                    
    push qword %1  
    jmp common_exception_wrapper
%endmacro

%assign i 0
%rep 32

    %if i == 8 || (i >= 10 && i <= 14) || i == 17 || i == 21 || i == 29 || i == 30
        EXCEPTION_WITH_ERR_CODE i
    %else
        EXCEPTION_NO_ERR_CODE i
    %endif
%assign i i+1
%endrep

extern c_exception_handler

common_exception_wrapper:

    push rbp
    push r15
    push r14
    push r13
    push r12
    push r11
    push r10
    push r9
    push r8
    push rdi
    push rsi
    push rdx
    push rcx
    push rbx
    push rax

    mov rdi, rsp
    

    call c_exception_handler


    pop rax
    pop rbx
    pop rcx
    pop rdx
    pop rsi
    pop rdi
    pop r8
    pop r9
    pop r10
    pop r11
    pop r12
    pop r13
    pop r14
    pop r15
    pop rbp


    add rsp, 16
    
    iretq

section .data
global g_exception_handlers_table

g_exception_handlers_table:
%assign i 0
%rep 32
    dq exception_handler_%[i]
%assign i i+1
%endrep



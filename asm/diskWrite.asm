extern CreateFileA
extern WriteFile
extern CloseHandle
extern DeleteFileA
extern ExitProcess

section .data align=4096
buffer1 times 65536 db 0xAA
buffer2 times 65536 db 0x55
buffer3 times 65536 db 0xFF
buffer4 times 65536 db 0x00

section .text
global diskWrite

diskWrite:
    push rbx
    push rsi
    push rdi
    push r12
    push r13
    push r14
    push r15

    mov r15, rcx        ; const char* name
    mov rbx, 8          ; 8 write cycles

.cycle_loop:
    ; HANDLE CreateFileA(LPCSTR lpFileName, ...);
    mov rcx, r15                    ; lpFileName
    mov rdx, 0x40000000             ; GENERIC_WRITE
    mov r8, 0                       ; no sharing
    mov r9, 0                       ; no security
    sub rsp, 32                    ; shadow space
    mov qword [rsp+0], 2           ; CREATE_ALWAYS
    mov qword [rsp+8], 0           ; FILE_ATTRIBUTE_NORMAL
    mov qword [rsp+16], 0          ; hTemplateFile

    call CreateFileA
    add rsp, 32

    cmp rax, -1
    je .next_cycle

    mov r12, rax                   ; store HANDLE
    mov r13, 8192                  ; 2GB worth of writes

.write_loop:
    ; Macro-like pattern for all four buffers:
    ; BOOL WriteFile(HANDLE hFile, LPCVOID lpBuffer, DWORD nNumberOfBytesToWrite, ...)

    mov rcx, r12
    lea rdx, [rel buffer1]
    mov r8d, 65536
    sub rsp, 32
    mov qword [rsp+0], rsp         ; lpNumberOfBytesWritten
    mov qword [rsp+8], 0           ; lpOverlapped

    call WriteFile
    add rsp, 32

    mov rcx, r12
    lea rdx, [rel buffer2]
    mov r8d, 65536
    sub rsp, 32
    mov qword [rsp+0], rsp         ; lpNumberOfBytesWritten
    mov qword [rsp+8], 0           ; lpOverlapped

    call WriteFile
    add rsp, 32

    mov rcx, r12
    lea rdx, [rel buffer3]
    mov r8d, 65536
    sub rsp, 32
    mov qword [rsp+0], rsp         ; lpNumberOfBytesWritten
    mov qword [rsp+8], 0           ; lpOverlapped

    call WriteFile
    add rsp, 32

    mov rcx, r12
    lea rdx, [rel buffer4]
    mov r8d, 65536
    sub rsp, 32
    mov qword [rsp+0], rsp         ; lpNumberOfBytesWritten
    mov qword [rsp+8], 0           ; lpOverlapped

    call WriteFile
    add rsp, 32

    ; Repeat with buffer2, buffer3, buffer4...
    ; You can macro-ize this or unroll

    dec r13
    jnz .write_loop

    ; Close handle
    mov rcx, r12
    call CloseHandle

    ; Delete the file
    mov rcx, r15
    call DeleteFileA

.next_cycle:
    dec rbx
    jnz .cycle_loop

    ; Cleanup
    pop r15
    pop r14
    pop r13
    pop r12
    pop rdi
    pop rsi
    pop rbx
    ret

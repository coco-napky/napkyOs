;kernel.asm
;Michael Black, 2007

;kernel.asm contains assembly functions that you can use in your kernel

	.global _putInMemory
	.global _interrupt
	.global _makeInterrupt21
	.extern _handleInterrupt21
	.global _printChar
	.global _readChar
	.global _readSector
	.global _writeSector
	.global _setKernelDataSegment
	.global _restoreDataSegment
	.global _irqInstallHandler
	.global _timer_ISR
	.global _setTimerPhase
	.global _copyToSeg
	.extern _scheduleProcess
	.data
	.extern _currentProcess
	.text

;void restoreDataSegment()
;restores the data segment
_restoreDataSegment:
        pop bx
        pop ds
        push bx
        ret


_copyToSeg:
	push bp
	mov bp, sp
	push es
	push di
	push si
	mov es, [bp+4]
	mov di, [bp+6]
	mov si, [bp+8]
	mov cx, [bp+10]
	rep
	movsb
	pop si
	pop di
	pop es
	pop bp
	ret

; void setTimerPhase(int hz) ; Set the timer frequency in Hertz
_setTimerPhase:
	push bp
	mov bp, sp
	mov dx, #0x0012 ; Default frequency of the timer is 1,193,180 Hz
	mov ax, #0x34DC
	mov bx, [bp+4]
	div bx

	mov bx, ax ; Save quotient

	mov dx, #0x43
	mov al, #0x36
	out dx, al ; Set our command byte 0x36

	mov dx, #0x40
	mov al, bl
	out dx, al ; Set low byte of divisor
	mov al, bh
	out dx, al ; Set high byte of divisor
	pop bp
	ret

;void setKernelDataSegment()
;sets the data segment to the kernel, saving the current ds on the stack
_setKernelDataSegment:
        pop bx
        push ds
        push bx
        mov ax,#0x1000
        mov ds,ax
        ret

; void irqInstallHandler(int irq_number, void (*fn)())
; Install an IRQ handler
_irqInstallHandler:
	cli
	push bp
	mov bp, sp
	push si
	push ds
	mov dx, #_timer_ISR ; Function pointer
	xor ax, ax
	mov ds, ax ; Interrupt vector is at lowest memory
	mov si, #0x8
	shl si, 2
	; ax = irq_handler * 4
	mov ax, cs
	mov [si + 2], ax
	mov [si], dx
	pop ds
	pop si
	pop bp
	sti
	ret

;void putInMemory (int segment, int address, char character)
_putInMemory:
	push bp
	mov bp,sp
	push ds
	mov ax,[bp+4]
	mov si,[bp+6]
	mov cl,[bp+8]
	mov ds,ax
	mov [si],cl
	pop ds
	pop bp
	ret

;int interrupt (int number, int AX, int BX, int CX, int DX)
_interrupt:
	push bp
	mov bp,sp
	mov ax,[bp+4]	;get the interrupt number in AL
	push ds		;use self-modifying code to call the right interrupt
	mov bx,cs
	mov ds,bx
	mov si,#intr
	mov [si+1],al	;change the 00 below to the contents of AL
	pop ds
	mov ax,[bp+6]	;get the other parameters AX, BX, CX, and DX
	mov bx,[bp+8]
	mov cx,[bp+10]
	mov dx,[bp+12]

intr:	int #0x00	;call the interrupt (00 will be changed above)

	mov ah,#0	;we only want AL returned
	pop bp
	ret

;void makeInterrupt21()
;this sets up the interrupt 0x21 vector
;when an interrupt 0x21 is called in the future,
;_interrupt21ServiceRoutine will run

_makeInterrupt21:
	;get the address of the service routine
	mov dx,#_interrupt21ServiceRoutine
	push ds
	mov ax, #0	;interrupts are in lowest memory
	mov ds,ax
	mov si,#0x84	;interrupt 0x21 vector (21 * 4 = 84)
	mov ax,cs	;have interrupt go to the current segment
	mov [si+2],ax
	mov [si],dx	;set up our vector
	pop ds
	ret

;this is called when interrupt 21 happens
;it will call your function:
;void handleInterrupt21 (int AX, int BX, int CX, int DX)
_interrupt21ServiceRoutine:
	push dx
	push cx
	push bx
	push ax
	call _handleInterrupt21
	pop ax
	pop bx
	pop cx
	pop dx

	iret

_readSector:
	push bp
	mov bp,sp
	sub sp,#6
	mov bx,[bp+4] ; buffer address

	mov ax,[bp+6] ; sector number
	mov cl,#36	  ; cl = 36
	div cl        ; ax/36
	mov ah,0
	mov [bp-2],ax ; sector/36 to function stack (track number)

	mov ax,[bp+6] ; sector number
	mov cl,#18    ; cl = 18
	div cl        ; sector/18,  al = division, ah = remainder
	and al,#0x1   ; mod for powers of 2, 2^1,  modulus 2
	mov dx,#0
	mov dl,al
	mov [bp-4],dx ; store sector/36 to function stack (head)

	add ah,#1	  ; ah stored remainder, add 1
	mov dx,0
	mov dl,ah
	mov [bp-6], dx ; store sector%18 + 1 to function stack (relative)

	; set registers for interrupt call
	mov al, #0x1
	mov ah, #0x2
	mov dl, #0x0

	mov ch,[bp-2]
	mov dh,[bp-4]
	mov cl, [bp-6]

	int #0x13
	add sp,#6
	pop bp
 	ret

_writeSector:
	push bp
	mov bp,sp
	sub sp,#6
	mov bx,[bp+4] ; buffer address

	mov ax,[bp+6] ; sector number
	mov cl,#36	  ; cl = 36
	div cl        ; ax/36
	mov ah,0
	mov [bp-2],ax ; sector/36 to function stack (track number)

	mov ax,[bp+6] ; sector number
	mov cl,#18    ; cl = 18
	div cl        ; sector/18,  al = division, ah = remainder
	and al,#0x1   ; mod for powers of 2, 2^1,  modulus 2
	mov dx,#0
	mov dl,al
	mov [bp-4],dx ; store sector/36 to function stack (head)

	add ah,#1	  ; ah stored remainder, add 1
	mov dx,0
	mov dl,ah
	mov [bp-6], dx ; store sector%18 + 1 to function stack (relative)

	; set registers for interrupt call
	mov al, #0x1
	mov ah, #0x3
	mov dl, #0x0

	mov ch,[bp-2]
	mov dh,[bp-4]
	mov cl, [bp-6]

	int #0x13
	add sp,#6
	pop bp
	ret

;void printChar (char character)
_printChar:
	push bp
	mov bp, sp
	mov al, [bp+4]
	mov ah, #0x0e
	int #0x10
	pop bp
	ret

;char printChar ()
_readChar:
	push bp
	mov bp, sp
	mov al, #0
	mov ah, #0
	int #0x16
	pop bp
	ret


;this routine runs on timer interrupts
_timer_ISR:

	cli
    push bx
    push cx
    push dx
    push si
    push di
    push bp
    push ax
    push ds
    push es

    mov bx,sp

    mov al,#0x20
    out #0x20,al

    mov ax,#0x1000
    mov ds,ax
    mov ss,ax
    mov sp,#0xfff0
    push bx
    call _scheduleProcess
    mov bx, [_currentProcess]

    mov sp,[bx+2]
    mov ss,[bx+4]

    pop es
    pop ds
    pop ax
    pop bp
    pop di
    pop si
    pop dx
    pop cx
    pop bx

    sti
    iret

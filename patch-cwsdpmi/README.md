The bash file patches CWSDPMI to request reduced amounts of 
extended memory when running in XMS mode, so that we may
request XMS memory at runtime for use with DMA.

The code that is patched is:

```
; ulong xms_query_extended_memory(void) - return largest free block
	public	_xms_query_extended_memory
_xms_query_extended_memory	proc	near
	mov     ah,88H                            ; b4 88
	call    [xms_entry]                       ; ff 1e 94 3d
	or      bl,bl                             ; 0a db
	jne short fail_new_call                   ; 75 08
	mov     edx,eax                           ; 66 8b d0
	shr     edx,16                            ; 66 c1 ea 10
	ret                                       ; c3
fail_new_call:
	mov     ah,08H                            ; b4 08
	call    [xms_entry]                       ; ff 1e 94 3d
	xor     dx,dx                             ; 33 d2
	ret                                       ; c3
_xms_query_extended_memory	endp
```

Note that the above code is returning a 32 bit result in DX:AX.  It tries
to call function 0x88 to get the size of the largest available region as a
32 bit value. If that fails it calls 0x08, which is the older 
16-bit entrypoint.  Note that it is returning the size of the largest
allocatable region in KB.   Note also the '66' opcode prefix which allows 
access to 32-bit registers from 16-bit code.  Also note the use of the 
multi-bit shift which was added in the 80186 and won't be found in
OG 8088/8086 opcode lists.

There are 2 patched versions.

The first is C16MDPMI.EXE, which returns allocates a fixed amount of
memory (16Meg).  It replaces the above routine with:

```
    mov dx, 0                                 ; ba 00 00
    mov ax, 4000H                             ; b8 00 40
    ret                                       ; c3
```

The second is CM1MDPMI.EXE, which returns 1Megabyte _less_ than the 
size of the largest allocatable page returned by function 0x88.  It
does NOT support falling back to 0x08 if 0x88 is unavailable.  Here
is the patched replacement:

```
	mov     ah,88H                            ; b4 88
	call	[xms_entry]                       ; ff 1e 94 3d
	sub     eax, 0x400                        ; 66 2d 00 04 00 00
	or      bl,bl                             ; 0a db
	je      xms_success                       ; 75 03
	xor     eax, eax                          ; 66 31 c0
xms_success:
	mov     edx,eax                           ; 66 8b d0
	shr     edx,16                            ; 66 c1 ea 10
	ret                                       ; c3
```


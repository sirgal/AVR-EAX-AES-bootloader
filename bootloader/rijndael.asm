;performs a round of encryption 
;using given expanded keys and s-box
Rijndael_encrypt:
	push	ZH
	push	ZL
	push	YH
	push	YL
	push	XH
	push	XL

	ldi		ZH,	high(KEYS*2)
	ldi		ZL,	low( KEYS*2)
	rcall	add_round_key
	ldi		temp_0,	9

encryption_cycle:
	push	temp_0	;store cycle counter

	rcall	substitute_shift_rows
	rcall	mix_columns
	rcall	add_round_key

	rjmp	continue_enc
	rjmp	BOOT_START		;0x1F00. BOOTSZ can be here
continue_enc:

	pop		temp_0	;restore cycle counter
	dec		temp_0
	brne	encryption_cycle

	rcall	substitute_shift_rows
	rcall	add_round_key

	pop		XL
	pop		XH
	pop		YL
	pop		YH
	pop		ZL
	pop		ZH
	ret

;Z should ALWAYS point to the keys
;block = block ^ current_key
;obfuscation would be nice
add_round_key:
	clr		YH		;point to register file
	ldi		YL,	4
xor_Z_to_Y:
	lpm		temp_0,	Z+			;load key byte
	ld		temp_1,	Y			;load data byte
	eor		temp_1,	temp_0		;xor them
	st		Y+,	temp_1			;store back to data
	cpi		YL,	low( 4 + 16 )	;check if it was the last byte
	brne	xor_Z_to_Y			;if not - process next data byte
	ret

.equ ROW_MAJOR = 1
;some weird shit happens, involving finite field arithmetics
;NO BRANCHING ALLOWED, further obfuscation is always nice
mix_columns:
	;point to register file
	clr		YH
	ldi		YL,	4
next_column:
.if ROW_MAJOR == 1
	ldd		temp_2, Y+0x00	;result0
	ldd		temp_3, Y+0x01	;r1
	ldd		temp_4, Y+0x02	;r2
	ldd		temp_5, Y+0x03	;r3
.else
	ldd		temp_2, Y+0x00	;result0
	ldd		temp_3, Y+0x04	;r1
	ldd		temp_4, Y+0x08	;r2
	ldd		temp_5, Y+0x0C	;r3
.endif
	mov		temp_0, temp_3	;r1 to operand
	rcall	mul_by_2		;r1 * 2

	mov		temp_1, temp_0	;save r1 * 2
	eor		temp_0, temp_2  ;r0 + r1 * 2 
	eor		temp_0, temp_5	;r0 + r1 * 2 + r3 (lacks r2 * 3)
.if ROW_MAJOR == 1
	std		Y+0x01,	temp_0	;to r1
.else 
	std		Y+0x04,	temp_0	;to r1
.endif
	mov		temp_0,	temp_2	;r0 to operand
	rcall	mul_by_2		;r0 * 2

	mov		OP0, temp_0		;OP0 <- r0 * 2
	eor		temp_0, temp_1	;r0 * 2 + r1 * 2
	eor		temp_0, temp_3	;r0 * 2 + r1 * 3
	eor		temp_0, temp_4	;r0 * 2 + r1 * 3 + r2
	eor		temp_0, temp_5	;r0 * 2 + r1 * 3 + r2 + r3 (done)
	std		Y+0x00, temp_0	;to r0
	mov		temp_1, OP0		;OP0 -> r0 * 2
	mov		temp_0,	temp_5	;r3 to operand
	rcall	mul_by_2		;r3 * 2

	mov		OP0,	temp_0	;OP0 <- r3 * 2
	eor		temp_0, temp_1	;r3 * 2 + r0 * 2
	eor		temp_0, temp_2	;r0 * 3 + r3 * 2
	eor		temp_0, temp_3	;r0 * 3 + r1 + r3 * 2
	eor		temp_0, temp_4	;r0 * 3 + r1 + r2 + r3 * 2 (done)
.if ROW_MAJOR == 1
	std		Y+0x03,	temp_0	;to r3
.else
	std		Y+0x0C,	temp_0	;to r3
.endif
	mov		temp_1, OP0		;OP0 -> r3 * 2
	mov		temp_0,	temp_4	;r2 to operand
	rcall	mul_by_2		;r2 * 2

	mov		OP0, 	temp_0	;OP0 <- r2 * 2
	eor		temp_0, temp_1	;r2 * 2 + r3 * 2
	eor		temp_0, temp_5	;r2 * 2 + r3 * 3
	eor		temp_0, temp_2	;r0 + r2 * 2 + r3 * 3
	eor		temp_0, temp_3	;r0 + r1 + r2 * 2 + r3 * 3 (done)
.if ROW_MAJOR == 1
	std		Y+0x02, temp_0	;to r2
.else
	std		Y+0x08, temp_0	;to r2
.endif

	mov		temp_1, OP0		;OP0 -> r2 * 2
.if ROW_MAJOR == 1
	ldd		temp_0,	Y+0x01	;r0 + r1 * 2 + r3 (from memory)
.else
	ldd		temp_0,	Y+0x04	;r0 + r1 * 2 + r3 (from memory)
.endif
	eor		temp_0, temp_1	;r0 + r1 * 2 + r2 * 2 + r3
	eor		temp_0, temp_4	;r0 + r1 * 2 + r2 * 3 + r3 (done)
.if ROW_MAJOR == 1
	std		Y+0x01,	temp_0	;to r1
.else
	std		Y+0x04,	temp_0	;to r1
.endif

.if ROW_MAJOR == 1
	adiw	Y,	4			;pointer to next column
	cpi		YL,	20			;if not done
.else
	adiw	Y,	1			;pointer to next column
	cpi		YL,	8			;if not done
.endif
	brne	next_column		;process next
	ret

substitute_shift_rows:
	ldi		XH,	high(SBOX)
	ldi		XL,	low( SBOX)
	movw	OP0,	X

	;one column at a time
	clr		YH
	ldi		YL,	4
sub_next:
	movw	X,	OP0
	ldd		temp_0,	Y+0x08
	add		XL,	temp_0
	adc		XH,	NULL
	ld		temp_0,	X
	std		Y+0x08,	temp_0

	movw	X,	OP0
	ldd		temp_0,	Y+0x0C
	add		XL,	temp_0
	adc		XH,	NULL
	ld		temp_0,	X
	std		Y+0x0C,	temp_0

	movw	X,	OP0
	ldd		temp_0,	Y+0x04
	add		XL,	temp_0
	adc		XH,	NULL
	ld		temp_0,	X
	std		Y+0x04,	temp_0

	movw	X,	OP0
	ldd		temp_0,	Y+0x00
	add		XL,	temp_0
	adc		XH,	NULL
	ld		temp_0,	X
	st		Y+,	temp_0

	sbrs	YL,	3			;XL == 8
	rjmp	sub_next

;cyclical shift: 0_row << 0; 1_row << 1; 2_row << 2; 3_row << 3
shift_rows:
.if ROW_MAJOR == 1
	;1st row
	eor		b1,	bD
	eor		bD,	b1
	eor		b1,	bD

	eor		b1,	b5
	eor		b5,	b1
	eor		b1,	b5

	eor		b5,	b9
	eor		b9,	b5
	eor		b5,	b9

	;2nd row
	eor		b2,	bA
	eor		bA,	b2
	eor		b2,	bA

	eor		b6,	bE
	eor		bE,	b6
	eor		b6,	bE

	;3rd row
	eor		b3,	bF
	eor		bF,	b3
	eor		b3,	bF

	eor		b7,	bF
	eor		bF,	b7
	eor		b7,	bF

	eor		bB,	bF
	eor		bF,	bB
	eor		bB,	bF
	;done
.else
	;0th row - unnecessary
	;1st row
	eor b7, b4
	eor b4, b7
	eor b7, b4

	eor b6, b4
	eor b4, b6
	eor b6, b4

	eor b5, b4
	eor b4, b5
	eor b5, b4
	;2nd row
	eor b8, bA
	eor bA, b8
	eor b8, bA

	eor b9, bB
	eor bB, b9
	eor b9, bB
	;3rd row
	eor bC, bF
	eor bF, bC
	eor bC, bF

	eor bD, bF
	eor bF, bD
	eor bD, bF

	eor bE, bF
	eor bF, bE
	eor bE, bF
	;done
.endif
	ret

;temp_0 <- temp_0 * 2 (in a finite field)
;temp_0 = (temp_0 << 1) ^ (0x1B & MSB(temp_0))
;NO BRANCHING HERE
;uses NULL in a dirty way
mul_by_2:
	bst		temp_0,	7	;store 7th bit in T
	bld		NULL,	0	;we form 0x1B in NULL if T is set

	rjmp	cont_mul	
	rjmp	BOOT_START	;0x1F80. BOOTSZ can be here
cont_mul:

	bld		NULL,	4	
	lsl		temp_0		
	bld		NULL,	3	
	bld		NULL,	1
	eor		temp_0,	NULL
	clr		NULL
	ret

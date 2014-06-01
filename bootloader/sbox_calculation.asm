	ldi		XH,	high(SBOX)	;point X to SBOX memory location
	ldi		XL,	low( SBOX)
	ser		bF				;first inc will overflow to 0
next_box:
	inc		bF
	mov		temp_1,	bF		;save input in temp_1
	cp		temp_1,	NULL	;if it's null - return
	breq	sbox_byte_done	;return here
	mov		OP0,	OxFF	;so it overflows

look_more:
	inc		OP0			;try next candidate

;temp_0 <- OP0 * temp_1 (in a Galois field)
;branching is fine, function used in precomputation only
finite_multiplication:
	mov		b0,	OP0		;operand 0 (candidate)
	mov		b1,	temp_1	;operand 1 (current byte)

	ldi		temp_2,	0x1B;0x1B holder
	clr		temp_0		;multiplication result

next_bit:
	lsr		b0			;operand 0 >> 1
	brcc	PC+2		;if lsb of operand 0 was 1
	eor		temp_0,	b1	;xor operand 1 into result

	lsl		b1			;operand 1 << 1
	brcc	PC+2		;if msb of operand 1 was 1
	eor		b1,	temp_2	;xor 0x1B into operand 1

	cp		b0,	NULL	;while there are bits in operand0
	brne	next_bit	;work on it

	cpi		temp_0,	1	;if multiplication result was not 1
	brne	look_more	;inverse is in OP0

	clr		temp_1		;affine transform result

	;affine transform
	ldi		temp_5,	0b11110001	;matrix producer
	ldi		temp_3,	0b00000001	;current bit mask
process_bit:
	mov		temp_4,	OP0			;multiplicative inverse
	and		temp_4,	temp_5		;and with matrix producer

pop_next_bit:
	lsl		temp_4				;inv&matrix << 1
	brcc	PC+2				;if it had msb
	eor		temp_1,	temp_3		;sum bit into result
	cp		temp_4,	NULL		;while operand has bits
	brne	pop_next_bit		;work on it
	
	lsl		temp_3				;move to next bit
	lsl		temp_5				;cyclically shift matrix producer
	brcc	PC+2				;if it had msb
	ori		temp_5,	1			;move msb to lsb
	cp		temp_3,	NULL		;while there are bits left
	brne	process_bit			;process next bit

sbox_byte_done:
	ldi		temp_2,	0b01100011	;0x63
	eor		temp_1,	temp_2		;xor it into result
	st		X+,	temp_1			;save to memory
	cpse	bF,	OxFF			;if we're at last byte
	rjmp	next_box			;we're done 

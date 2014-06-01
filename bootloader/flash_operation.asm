;D( RCVD_PAGE ) -> flash
store_page:
	;erase current page
	ldi		temp_1, 0b00000011
	rcall	spm_it

	ldi		YH, high(RCVD_PAGE)
	ldi		YL, low( RCVD_PAGE)
	ldi		XH, high(ENC_IV)
	ldi		XL, low( ENC_IV)
write_next:
	ld		r0, Y+
	ld		temp_2, X+
	eor		r0, temp_2
	ld		r1, Y+
	ld		temp_2, X+
	eor		r1, temp_2
	;last countermeasure - if we jumped through tag check
	eor		r0, temp_0
	eor		r1, temp_0
	;store word to page buffer
	ldi		temp_1, 0b00000001
	rcall	spm_it
	adiw	Z, 2
	cpi		YL, low( RCVD_PAGE + PAGE_BYTES )
	brne	write_next
	;write page

	;back to page start
    subi	ZL, low( PAGE_BYTES)
    sbci	ZH, high(PAGE_BYTES)
	;write page
	ldi		temp_1, 0b00000101
	rcall	spm_it
	;to next page start
    subi	ZL, low( -PAGE_BYTES)
    sbci	ZH, high(-PAGE_BYTES)
	;re-enable flash
	ldi		temp_1, 0b00010001
	rcall	spm_it

	ret

.ifdef SPMCSR
	.equ SPM_REGISTER = SPMCSR
.else 
	.equ SPM_REGISTER = SPMCR
.endif

spm_it:
	in		temp_2, SPM_REGISTER
	sbrc	temp_2, SPMEN
	rjmp	spm_it
	out		SPM_REGISTER, temp_1
	spm
	ret

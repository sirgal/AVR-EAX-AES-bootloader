.dseg
.org 0x60
	SBOX:		.byte 256			;rijndael substitution box
	;these three SHOULD be consecutive
	SAVED_IV:	.byte BLOCK_SIZE	;E(L0^N^B)
	RCVD_PAGE:	.byte PAGE_BYTES	;page to be written
	TAG:		.byte BLOCK_SIZE	;initially - precomputed header value

	ENC_IV:		.byte PAGE_BYTES	;IV's to xor with page to decrypt
.cseg

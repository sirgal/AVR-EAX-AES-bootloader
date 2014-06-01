.include "m8Adef.inc"

.equ PAGE_BYTES 	 = PAGESIZE*2
.equ BOOT_SIZE		 = 1024
.equ BLOCK_SIZE		 = 16
.equ PAGES 			 = (FLASHEND+1)/PAGESIZE - BOOT_SIZE/PAGE_BYTES
.equ BLOCKS_PER_PAGE = PAGE_BYTES / BLOCK_SIZE
.equ KEY_ADDR 		 = (FLASHEND + 1) - (BLOCK_SIZE*(11+4))/2	;11 keys, 4 precomputed values

;Reset address (Where to jump to if not asked to load boot)
.equ RESET_VECT		= 0
;Is 0th flash page used?
.equ USE_0th_PAGE	= 1

;////////////////////////PORT SETUP
// use port letter...
// A / B / C / D / E
.equ port_used = 'C'
// check status of pin number...
.equ pin   = 5
// load boot only if port is...
// (S)ET (1) / (C)LEAR (0)
.equ level = 'S'

;////////////////////////BAUD RATE SETUP
.equ Fosc = 8000000	;clock frequency
.equ baud = 9600 ;baud rate

.equ UBRR = Fosc / ( BLOCK_SIZE * baud ) - 1
.if high( UBRR ) != 0
	.error "Unsupported baud rate setting - high byte of UBRR is not 0!"
.endif

.include "boot_memory.inc"
.include "registers.asm"
.include "key_const.asm"

.org THIRDBOOTSTART
BOOT_START:
	ldi		temp_0,	low( RAMEND)
	out		SPL,	temp_0
	ldi		temp_0,	high(RAMEND)
	out		SPH,	temp_0

	cli		

	clr		NULL
	mov		OxFF,	NULL
	com		OxFF

	ldi		temp_0,	low( UBRR )
	out		UBRRH,	NULL
	out		UBRRL,	temp_0
	ldi		temp_0,	( 1 << RXEN ) | ( 1 << TXEN )
	out		UCSRB,	temp_0
	ldi		temp_0,	( 1 << URSEL ) | ( 1 << UCSZ1 ) | ( 1 << UCSZ0 )
	out		UCSRC,	temp_0

	;inline compile-time state machine
	.include "check_port_state.asm"

	;if user asked to load boot - this will be skipped
	;jmp		RESET_VECT		
	rjmp	RESET_VECT
	nop

	;big inline function
	.include "sbox_calculation.asm"

wait_for_start:
	rcall	confirm_and_read
	cpi		temp_0,	0x60
	brne	wait_for_start

;/////////////////////////////PAGE ADDR INIT
.if USE_0th_PAGE == 0
	ldi		ZH,	high(PAGE_BYTES)
	ldi		ZL,	low( PAGE_BYTES)
	ldi		temp_0,	PAGES - 1
.else 
	clr		ZH
	clr		ZL
	ldi		temp_0,	PAGES
.endif

next_page:
	;save page counter and address
	push	temp_0
	push	ZH
	push	ZL

;/////////////////////////////BLOCK RECEPTION
	;receive whole block
	ldi		XH,	high(SAVED_IV)
	ldi		XL,	low( SAVED_IV)
	ldi		temp_1,( BLOCK_SIZE /*nonce*/ + PAGE_BYTES /*page*/ + BLOCK_SIZE /*expected tag*/ )
get_more_block:
	rcall	confirm_and_read
	st		X+,	temp_0
	dec		temp_1
	brne	get_more_block

;/////////////////////////////TAG INITIALIZATION
	;initialize precomputed header with tag
	;tag <- H ^ tag
header_to_tag:
	ldi		ZH,	high(PRECOMP_HEADER_TAG*2)
	ldi		ZL,	low( PRECOMP_HEADER_TAG*2)
	ldi		YH,	high(TAG)
	ldi		YL,	low( TAG)
next_header_byte:
	lpm		temp_0,	Z+
	ld		temp_1,	Y
	eor		temp_0,	temp_1
	st		Y+,	temp_0
	cpi		YL,	low( TAG + BLOCK_SIZE )
	brne	next_header_byte

;/////////////////////////////NONCE
	;block <- N
	ldi		XH,	high(SAVED_IV)
	ldi		XL,	low( SAVED_IV)
	rcall	from_X_to_regs
	;block <- N ^ B
	ldi		ZH,	high(PRECOMP_B*2)
	ldi		ZL,	low( PRECOMP_B*2)
	rcall	add_round_key
	;block <- N ^ B ^ L0
	ldi		ZH,	high(PRECOMP_L0*2)
	ldi		ZL,	low( PRECOMP_L0*2)
	rcall	add_round_key
	;block <- E( N^B^L0 ) (nonce)
	rcall	Rijndael_encrypt
	;save calculated nonce
	rcall	save_IV	
	;tag <- H ^ N ^ expected
	ldi		YH,	high(TAG)
	ldi		YL,	low( TAG)
	ldi		ZH,	high(SAVED_IV)
	ldi		ZL,	low( SAVED_IV)
	rcall	xor_Z_to_Y_RAM
	
;/////////////////////////////DECRYPTION IVs
	ldi		YH,	high(ENC_IV)
	ldi		YL,	low( ENC_IV)
IV_calc_cycle:
	;block <- E(IV)
	rcall	Rijndael_encrypt
	;ENC_IV <- E(IV)
	rcall	from_regs_to_Y
	push	YH
	push	YL
	;IV++
	rcall	rest_IV
	rcall	increment_regs
	rcall	save_IV
	pop		YL
	pop		YH
	cpi		YL,	low( ENC_IV + PAGE_BYTES )
	brne	IV_calc_cycle

;/////////////////////////////CMAC / OMAC TAG CALCULATION ( block <- C )
	;X countains 20 after last save_IV command
clear_registers:
	st		-X,	NULL
	cpi		XL,	4
	brne	clear_registers
	;block <- L2
	ldi		ZH,	high(PRECOMP_L2*2)
	ldi		ZL,	low( PRECOMP_L2*2)
	rcall	add_round_key
	;last block is processed individually
	ldi		temp_0,	BLOCKS_PER_PAGE
	ldi		ZH,	high(RCVD_PAGE)
	ldi		ZL,	low( RCVD_PAGE)
CBC_TAG:
	;block <- block ^ m(i)
	;temp_0 is fine
	rcall	xor_Z_to_block_RAM

	push	temp_0
	cpi		temp_0,	1
	brne	dont_add_B

	ldi		ZH,	high(PRECOMP_B*2)
	ldi		ZL,	low( PRECOMP_B*2)
	rcall	add_round_key
dont_add_B:
	;Z is saved properly
	rcall	Rijndael_encrypt

	pop		temp_0
	dec		temp_0
	brne	CBC_TAG

	;block <- H ^ N ^ C ^ expected
	ldi		ZH,	high(TAG)
	ldi		ZL,	low( TAG)
	rcall	xor_Z_to_block_RAM

;/////////////////////////////TAG CHECK
	clr		temp_0
check_more:
	ld		temp_1,	-Y
	or		temp_0,	temp_1
	cpi		YL,	4
	brne	check_more
	cp		temp_0,	NULL
	breq	do_write
	rjmp	die
;/////////////////////////////TAG SUCCESS - CTR AND WRITE
do_write:
	;restore page pointers
	pop		ZL
	pop		ZH
	;decrypt and write page
	rcall	store_page
	;restore page counter
	pop		temp_0
	dec		temp_0
	;continue if not done, else - die
	breq	upload_done
	rjmp	next_page

	;yep, we're done!
upload_done:
	ldi		temp_0,	0x0C
	rcall	UART_send
;/////////////////////////////TAG FAILURE AND EXIT
die:
	ldi		temp_0,	0xFF
	rcall	UART_send
	rjmp	die

;block++
;all carrying is done properly
increment_regs:
	ldi		YH,	0
	ldi		YL,	20
	clr		temp_0
carry_next:
	ld		temp_0,	Y
	cpi		temp_0,	1
	ld		temp_0,	-Y
	adc		temp_0,	NULL
	st		Y,	temp_0
	cpi		YL,	5
	brsh	carry_next
	ret

;block <- block ^ Z
xor_Z_to_block_RAM:
	ldi		YH,	0
	ldi		YL,	4
;Y <- Y ^ Z
xor_Z_to_Y_RAM:
	ldi		temp_2,	BLOCK_SIZE
;Y <- Y ^ Z ( temp_2 times )
ram_xor_cycle:
	ld		temp_3,	Z+
	ld		temp_1,	Y
	eor		temp_1,	temp_3
	st		Y+,		temp_1
	dec		temp_2
	brne	ram_xor_cycle
	ret

;block -> SAVED_IV
save_IV:
	ldi		YH,	high(SAVED_IV)
	ldi		YL,	low( SAVED_IV)
;block -> Y
from_regs_to_Y:
	ldi		XH,	0
	ldi		XL,	4
	rjmp	move_from_X_to_Y
;SAVED_IV -> block
rest_IV:
	ldi		XH,	high(SAVED_IV)
	ldi		XL,	low( SAVED_IV)
;X -> block
from_X_to_regs:
	ldi		YH,	0
	ldi		YL,	4
;X -> Y
move_from_X_to_Y:
	ldi		temp_0,	0x10
;X -> Y ( temp_0 times )
ram_save_cycle:
	ld		temp_1,	X+
	st		Y+,	temp_1
	dec		temp_0
	brne	ram_save_cycle
	ret

;UART   <- 0xC0
;temp_0 <- UART
confirm_and_read:
	ldi		temp_0,	0xC0
;UART   <- temp_0
;temp_0 <- UART
UART_send:
	sbis	UCSRA,	UDRE
	rjmp	UART_send
	out		UDR,	temp_0
;temp_0 <- UART
UART_read:
	sbis	UCSRA,	RXC
	rjmp	UART_read
	in		temp_0,	UDR
	ret

.include "flash_operation.asm"
.include "rijndael.asm"

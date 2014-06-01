.if port_used == 'A'
	cbi		DDRA,	pin
	sbi		PORTA,	pin
	nop
	.if level == 'S'
		sbis	PINA,	pin
	.elif level == 'C'
		sbic	PINA,	pin
	.endif

.elif port_used == 'B'
	cbi		DDRB,	pin
	sbi		PORTB,	pin
	nop
	.if level == 'S'
		sbis	PINB,	pin
	.elif level == 'C'
		sbic	PINB,	pin
	.endif

.elif port_used == 'C'
	cbi		DDRC,	pin
	sbi		PORTC,	pin
	nop
	.if level == 'S'
		sbis	PINC,	pin
	.elif level == 'C'
		sbic	PINC,	pin
	.endif

.elif port_used == 'D'
	cbi		DDRD,	pin
	sbi		PORTD,	pin
	nop
	.if level == 'S'
		sbis	PIND,	pin
	.elif level == 'C'
		sbic	PIND,	pin
	.endif

.elif port_used == 'E'
	cbi		DDRE,	pin
	sbi		PORTE,	pin
	nop
	.if level == 'S'
		sbis	PINE,	pin
	.elif level == 'C'
		sbic	PINE,	pin
	.endif
.endif

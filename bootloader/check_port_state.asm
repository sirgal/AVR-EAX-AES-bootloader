.macro port_settings /*PORT LETTER, PIN NUMBER, LOGIC LEVEL*/
	cbi		DDR@0 ,	@1 
	sbi		PORT@0, @1 
	nop
	sbi@2	PINB,	@1
.endmacro 

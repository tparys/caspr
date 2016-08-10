.arch tiny
.define OUT $FF

	cla			; Clear Acc and then adding the operand is
LOOP:	add ONE			; equivalent to move the operand to Acc
	str OUT
	jnz LOOP
	rst

	;; skip ahead to ROM constant section
	.org $78
	
ONE:	byte $01
	

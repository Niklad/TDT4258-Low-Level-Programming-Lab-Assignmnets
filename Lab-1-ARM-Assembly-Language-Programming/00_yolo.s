.equ DDR_HIGH_WORD, 0x3FFFFFFC

.global _start

.section .text

_start:
	// Here your execution starts
	b 		check_input


	
check_input:
	// You could use this symbol to check for your input length
	// you can assume that your input string is at least 2 characters 
	// long and ends with a null byte
	LDR 	r0, =input
	LDR 	r1, [r0]
	
	B 		palindrome_found
	
	
check_palindrome:
	// Here you could check whether input is a palindrome or not
	
	
palindrome_found:
	// Switch on only the 5 rightmost LEDs
	// Write 'Palindrome detected' to UART
	CMP 	r1, #0
	BEQ 	cont
	
	BL 		PUT_JTAG
	ADD 	r0, r0, #1
	B 		check_input
	
palindrom_not_found:
	// Switch on only the 5 leftmost LEDs
	// Write 'Not a palindrome' to UART
	
cont:
	BL 		GET_JTAG
	CMP 	r1, #0
	BEQ 	cont
	BL 		PUT_JTAG
	B 		cont
	
/********************************************************************************
* Subroutine to send a character to the JTAG UART
* R0 = character to send
********************************************************************************/
.global PUT_JTAG


PUT_JTAG:
	LDR 	R1, =0xFF201000 // JTAG UART base address
	LDR 	R2, [R1, #4] // read the JTAG UART control register
	LDR 	R3, =0xFFFF
	ANDS 	R2, R2, R3 // check for write space
	BEQ 	END_PUT // if no space, ignore the character
	STR 	R0, [R1] // send the character
END_PUT:
	BX LR
	
/********************************************************************************
* Subroutine to get a character from the JTAG UART
* Returns the character read in R0
********************************************************************************/
.global GET_JTAG

GET_JTAG:
			LDR 	R1, =0xFF201000 // JTAG UART base address
			LDR 	R0, [R1] // read the JTAG UART data register
			ANDS 	R2, R0, #0x8000 // check if there is new data
			BEQ 	RET_NULL // if no data, return 0
			AND 	R0, R0, #0x00FF // return the character
			B 		END_GET
RET_NULL:	MOV 	R0, #0
END_GET: 	BX 		LR
	
exit:
	// Branch here for exit
	B 		exit
	

.section .data
.align
	// This is the input you are supposed to check for a palindrome
	// You can modify the string during development, however you
	// are not allowed to change the label 'input'!
	input: .asciz "level"
	// input: .asciz "8448"
    // input: .asciz "KayAk"
    // input: .asciz "step on no pets"
    // input: .asciz "Never odd or even"
	palindrome_detected_str: .asciz "Palindrome detected"

.end

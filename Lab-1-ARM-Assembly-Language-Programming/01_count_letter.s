.global _start

.section .text

_start:
	// variables for check_input	
	MOV 	r0, #0				// letter counter
	MOV		r1, #0xff			// letter mask
	MOV		r5, #0				// offset to next register
	// variables for check_palindrome
	MOV 	r6, #0				// rightward iterator
	MOV		r7, #0				// leftward iterator
	
	// load input
	LDR 	r2, =input
	LDR 	r3, [r2]

	B 		check_input

check_input:
	// You could use this symbol to check for your input length
	// you can assume that your input string is at least 2 characters 
	// long and ends with a null byte
	
	// mask input to get one and one letter
	AND 	r4, r3, r1		
	
	// exit if null byte
	CMP		r4, #0
	BNE		.+8			
	B 		check_palindrome
	
	// loop and check next letter, move to next register if end of register
	ADD		r0, r0, #1
	ROR		r1, r1, #(32-8)		// ROL doesn't seem to work so 32-8 it is
	// big brain time for checking for modulo 4
	AND		r4, r0, #0b11
	CMP		r4, #0
	BNE		.+12
	ADD		r5, r5, #4
	LDR 	r3, [r2, r5]
	// loop
	B 		check_input
	

	
check_palindrome:
	// Here you could check whether input is a palindrome or not
	
	
palindrome_found:
	// Switch on only the 5 rightmost LEDs
	// Write 'Palindrome detected' to UART
	
	
palindrom_not_found:
	// Switch on only the 5 leftmost LEDs
	// Write 'Not a palindrome' to UART
	
	
exit:
	// Branch here for exit
	b exit
	

.section .data
.align
	// This is the input you are supposed to check for a palindrome
	// You can modify the string during development, however you
	// are not allowed to change the label 'input'!
	input: .asciz "levelaaaaaa"
	// input: .asciz "8448"
    // input: .asciz "KayAk"
    // input: .asciz "step on no pets"
    // input: .asciz "Never odd or even"


.end

.global _start

.section .text

_start:
	// variables for check_input	
	MOV 	r0, #0				// letter counter
	MOV		r1, #0xff			// letter mask
	MOV		r5, #0				// offset to next register
	// variables for check_palindrome
	MOV 	r6, #0				// foreward iterator
	// variables for palindrome_found and palindrome_not_found
	MOV		r11, #0b11111		// for turning on five LEDs
		
	// load input
	LDR 	r2, =input
	LDR 	r3, [r2]
	
	// load LED and UART registers
	LDR 	r10, =0xFF200000	// red LEDs
	
	BL 		check_input

check_input:
	// checking for input length,
	// assuming that the input string is at least 2 characters long and 
	// ends with a null byte
	
	// NOTE: 	It has come to my attention that this can be done
	// 			a lot easier using LDRB, but it works now and I'll
	//			leave it like this. Using LDRSB from check_palindrome
	// 			and onward.
	
	// mask input to get one and one letter
	AND 	r4, r3, r1		
	
	// exit if null byte
	CMP		r4, #0
	BNE		.+12
	SUB 	r0, r0, #1			// need r0-1 for the backward iterator
	BL 		check_palindrome
	
	// loop and check next letter, move to next register if end of register
	ADD		r0, r0, #1
	ROR		r1, r1, #(32-8)		// ROL doesn't seem to work so 32-8 it is
	// checking for modulo 4 the cool way
	AND		r4, r0, #0b11
	CMP		r4, #0
	BNE		.+12
	ADD		r5, r5, #4
	LDR 	r3, [r2, r5]
	// loop
	B 		check_input
	
check_palindrome:

	LDRB 	r8, [r2, r6]		// load first letter
	LDRB 	r9, [r2, r0]		// load last letter using r0 as backward iterator
	
	// skip letter if whitespace
	CMP 	r8, #0x20			// whitespace = 0x20
	BNE		.+12
	ADD		r6, r6, #1
	LDRB	r8, [r2, r6]
	CMP		r9, #0x20			// whitespace = 0x20
	BNE		.+12
	SUB		r0, r0, #1
	LDRB	r9, [r2, r0]
	
	// change to all lower case
	CMP		r8, #0x61			// a = 0x61
	BGE		.+8
	ADD		r8, r8, #0x20		// add offset to lower case
	CMP		r9, #0x61			// a = 0x61
	BGE		.+8
	ADD		r9, r9, #0x20		// add offset to lower case
	
	
	// compare letters to see if they match
	CMP		r8, r9
	BLNE	palindrome_not_found
	
	// iterate iterators
	ADD 	r6, r6, #1	
	SUB		r0, r0, #1
	
	// check if we're halfway thorugh the word
	CMP		r6, r0
	BLGE	palindrome_found
	
	B		check_palindrome
	
palindrome_found:
	// Switch on the 5 rightmost LEDs
	// Write 'Palindrome detected' to UART
	STR 	r11, [r10] 			// turn on rightmost LEDs
	
	
	
	BL 		exit
	
	
palindrome_not_found:
	// Switch on the 5 leftmost LEDs
	// Write 'Not a palindrome' to UART
	
	
	LSL		r11, #5				// change LEDs from right to left side
	STR		r11, [r10]			// turn on leftmost LEDs
	
	
	
	BL 		exit
	
exit:
	// Branch here for exit
	B .
	

.section .data
.align
	// This is the input you are supposed to check for a palindrome
	// You can modify the string during development, however you
	// are not allowed to change the label 'input'!
	// input: .asciz "level"
	// input: .asciz "8448"
    // input: .asciz "KayAk"
    // input: .asciz "step on no pets"
    // input: .asciz "Never odd or even"
	
	input: .asciz "bing bong gnob gnib"
.end

; abr.asm
;
;*****************************************************************
;TMS32020 Source code - asm320 Macro Assembler
;*****************************************************************
; Note:
; AR1 will be used for dac_a.  AR2 will be used for accbuf
; AR3 for acc2.  AR4 will be used for points counting
;*****************************************************************
; The Ariel DSP-16+ has 16KW of static memory. This memory
; is used for both program and data through separate memory maps
; and is accessible to the PC when the DSP-16+ is halted.
; When the "global register" (GREG) is set to provide 8 KW of
; data memory the program and data spaces are separate;
; the DSP program memory is addessed from 0 to 1FFFH (words) and
; the DSP data memory is addressed from  E000H to FFFFH (words)
; When the "global register" (GREG) is set to provide 16 KW of
; data memory the program and data spaces overlap;
; the DSP program memory is addessed from 0 to 3FFFH (words) and
; the DSP data memory is addressed from  C000H to FFFFH (words)
; Each DSP word is 16-bits.
;*****************************************************************
; Version 1.0
; Aug-20-93,  Zhiqiang Liu
; Boys Town National Research Hospital
;****************************************************************
;
; parameter block equates
;
mode:	equ	0
npts:	equ	1
sweeps:	equ	2
dac1:	equ	3
acc1:   equ	4
acc2:   equ	5
gr:	equ	6
spidx:	equ	7
;
; memory address equates
;
SPAG1	equ	506     ; 1st single-points initial page [unused]
SPAG2	equ	507     ; 2nd single-points initial page [unused]
DPAG    equ     510     ; parameter block page
spstrt1 equ	64768	; start of 1st single-points block = SPAG1*128
spstrt2 equ	64896	; start of 2nd single-points block = SPAG2*128
dtstrt	equ	65280	; start of parameter block = DPAG*128 [unused]
;
; upload parameter block starting from 64
;
max1:	equ	64
min1:	equ	65
max2:	equ	66
min2:	equ	67
sq1lo:	equ	68
sq1hi:	equ	69
sq1hir:	equ	70
sq2lo:	equ	71
sq2hi:	equ	72
sq2hir:	equ	73
;
;local variable equates
;
one:	equ	96
zero:	equ	97
npts1:	equ	98
swpcnt: equ	99
index:	equ	100
sp_loc: equ	101
temp:	equ	102
tmp1:	equ	103
;
; I/O port equates
;
ADC:	EQU	0		; Serial receive
DAC:	EQU	13		; DAC I/O
HOST:	EQU	15		; HOST I/O

;******************* Interrupt Vectors *************

	ORG	0
reset:	b	init

	ORG	4
INT1:	B	dac_a		; Interrupt 1 service routine

	ORG	6
INT2:	B	dac_b		; Interrupt 2 service routine

	ORG	26
RCV:	B	adc_isr		; ADC service routine

;******************* Main Program *************

INIT:
	DINT		;(redundant)
	ROVM		;reset overflow mode
	SSXM		;set sign-extension mode
	SPM	0	;set product mode for no-shift
	FORT	0	;set serial port for 16-bit words
	LDPK	DPAG	;data page for program variables
	zals	gr	;get GREG flag
	bnz	sixteen
	LACK	0E0H	;change upper 8K of program RAM into "global" RAM
	b	save
sixteen:
	LACK	0C0H	;change upper 16K of program RAM into "global" RAM
save:
	LDPK	0	;data page for mapped registers
	SACL	GREG
	zac
	sacl	imr	; disable interrupts

	LDPK	DPAG	;data page for program variables

; initialize variables

	zac
	sacl	zero
	sacl	min1
	sacl	max1
	sacl	min2
	sacl	max2
	sacl	sq1lo
	sacl	sq1hi
	sacl	sq1hir
	sacl	sq2lo
	sacl	sq2hi
	sacl	sq2hir
	lack	1
	sacl	one
	lac	npts
	sub	one
	sacl	npts1
	sacl	index
	sub	spidx
	sacl	sp_loc
	zals	sweeps
	sacl	swpcnt

; Zero accumulator buffers

	lac	npts, 1
	sacl	temp	; npts*2

	lar	6, temp
	lar	7, acc1
	zac
	larp	7
zerbuf1:
	sacl	*+, 0, 6
	banz	zerbuf1, *-, 7

	lar	6, temp
	lar	7, acc2
	zac
	larp	7
zerbuf2:
	sacl	*+, 0, 6
	banz	zerbuf2, *-, 7

; Check the mode
	zals	mode
	blez	loop	; mode 0 for idle

Setup:
	lar	1, dac1
	lar	2, acc1
	lar	3, acc2
	lrlk	4, spstrt1
	lrlk	5, spstrt2
	zals	npts1
	sacl	index

; Start the interrupts

resbio:	bioz	resbio		; wait here while BIO == 0
setbio: bioz	intmsk		; wait here until BIO == 0, again
	b	setbio
resbi2:	bioz	resbi2		; wait here while BIO == 0

intmsk:				; Start interrupts with a/d channel A
	LDPK	0
	LACK	22		; Interrupt mask: INT1 | INT2 | RCV
	SACL	IMR		; Set interrupt mask
	LDPK	DPAG
	RXF	           	; Reset external flag bit
	EINT

; Idle loop

LOOP:	idle
	B	LOOP  		; Do nothing

;************************ output **********************************

dac_a:				; Channel A interrupt
	larp	1
	out	*+, dac		; Load DAC LATCH 
	EINT			; Enable interrupts
	RET			; Return from interrupt

dac_b:				; DAC B is unused for ABR
	out	zero, dac	; Load DAC LATCH
	EINT
	RET

;************************ input **********************************

adc_isr:			; ADC interrupt
	LDPK	0
	lac	adc
	LDPK	DPAG
	sacl	temp		; store adv for later use
	BIOZ	chan_B	        ; Is it channel B ?

chan_A:			        ; accum. channel A into acc1
	larp	2
	adds	*+
	addh	*-
	sacl	*+		; store low word
	sach	*+		; store high word

	zals	sp_loc
	sub	index
	bnz	try_max1	; not the right point

	lt	temp
	mpy	temp            ; compute sp^2
	pac
	adds	sq1lo
	sacl	sq1lo
	sach	tmp1
	zals	sq1hi
	adds	tmp1
	addh	sq1hir
	sacl	sq1hi
	sach	sq1hir

	lac	temp            ; save sp value
	larp	4
	sacl	*+

try_max1:			; see if adv > max1
 	lac	temp
	sub	max1
	blez	try_min1
	zals	temp
	sacl	max1
	b	end_try1

try_min1:			; see if adv < min1
	lac	temp
	sub	min1
	bgez	end_try1
	zals	temp
	sacl	min1

end_try1:
	SXF
	EINT
	RET

chan_B:			        ; accum. channel B into acc2
	larp	3
	adds	*+
	addh	*-
	sacl	*+		; store low word
	sach	*+		; store high word

	lac	sp_loc
	sub	index
	bnz	try_max2	; not the right point

	lt	temp
	mpy	temp            ; compute sp^2
	pac
	adds	sq2lo
	sacl	sq2lo
	sach	tmp1
	zals	tmp1
	adds	sq2hi
	addh	sq2hir
	sacl	sq2hi
	sach	sq2hir

	lac	temp            ; save sp value
	larp	5
	sacl	*+

try_max2:			; see if adv > max2
	lac	temp
	sub	max2
	blez	try_min2
	zals	temp
	sacl	max2
	b	ck_sweep

try_min2:			; see if adv < min2
	lac	temp
	sub	min2
	bgez	ck_sweep
	zals	temp
	sacl	min2

ck_sweep:			; check for end of sweep
	lac	index
	sub	one
	sacl	index
	bgez	ad_ret

; set up pointers for start of next sweep & swap a/d buffers

	lar	1, dac1
	lar	2, acc1
	lar	3, acc2
	zals	npts1
	sacl	index

	zals	swpcnt
	sub	one
	sacl	swpcnt
	out	swpcnt, host	; send data to host
	bgz	ad_ret

ad_done:
	LDPK	0
	zac
	sacl	imr		; interrupt mask: no interrupts
	RET			; return without enabling interrupts

ad_ret:
	RXF
	EINT
	RET

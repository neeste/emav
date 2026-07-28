; tbabr2.asm - DSP code for ABRAV with TB Pinnacle/Fiji soundcard
;
;*****************************************************************
; M56001 assembly source code (use a56 to assemble)
;*****************************************************************
;
; The Turtle Beach "Multisound" (Pinnacle or Fiji) has 32KW of 
; static memory used for both program and data. In addition,
; The M56001 has 256 words of data memory and 512 words of program 
; memory on the chip. The program memory is addressed from 
; P:0000 to P:$01FF and from P:$4000 to P:$BFFF. The data memory 
; is adressed from X:$0000 to X:$00FF and from X:$4000 to X:$BFFF. 
; The upper 16KW of data memory is also addressed from Y:$4000 to 
; Y:$BFFF. The Y:$4000 to Y:$7FFF adresses access the same memory 
; as the X:$8000 to X:$BFFF addresses, and vice versa. Each DSP word 
; is 24-bits. The upper 16 bits of each word in the 32KW of data 
; memory is accessible to the PC through a shared memory area.
; 
; The soundcard has a 20-bit, stereo A/D converter (Crystal CS5335) and 
; an 20-bit, stereo D/A convertor (Crystal CS4327). 
;
;*****************************************************************
;
; parameter block equates
;
MODE	EQU	0
NPTS	EQU	1
SWEEPS	EQU	2
DAC1    EQU	3
ACC1    EQU	4
ACC2    EQU	5
GR	EQU	6
SPIDX	EQU	7
SKIP    EQU     8
NP	EQU	9       ; number of parameters
;
; memory address equates
;
TOPMEM	EQU	$0000
SPSTRT1 EQU	$7D00   ; start of 1st single-points block
SPSTRT2 EQU	$7D80   ; start of 2nd single-points block
PRMBLK  EQU	$7F00   ; start of parameter block
;
; upload parameter block starting from 64
;
MAX1	EQU	$7F40   ; 64
MIN1	EQU	$7F41   ; 65
MAX2	EQU	$7F42   ; 66
MIN2	EQU	$7F43   ; 67
SQ1LO	EQU	$7F44   ; 68
SQ1HI	EQU	$7F45   ; 69
SQ1HIR	EQU	$7F46   ; 70
SQ2LO	EQU	$7F47   ; 71
SQ2HI	EQU	$7F48   ; 72
SQ2HIR 	EQU	$7F49   ; 73
;
;local variable equates
;
SWPCNT  EQU     16
SP_LOC  EQU	17
TEMP	EQU	18
;
; I/O register equates
;
SRCR    EQU  $FFC0    ; sample rate control register
CLKE    EQU  $FFC2    ; sample clock enable ???
VOLC    EQU  $FFC4    ; volume control register
;
PBCR    EQU  $FFE0    ; port B control register
PCCR    EQU  $FFE1    ; port C control register
PCDD    EQU  $FFE3    ; port C data direction register
HOCR    EQU  $FFE8    ; host control register
HOSR    EQU  $FFE9    ; host status register
HOTR    EQU  $FFEB    ; host receive data register
SSCA    EQU  $FFEC    ; SSI control register A
SSCB    EQU  $FFED    ; SSI control register B
SSSR    EQU  $FFEE    ; SSI status/time slot register
STRD    EQU  $FFEF    ; serial transmit/receive data register
PLLC    EQU  $FFFD    ; PLL control register
PABC    EQU  $FFFE    ; port A bus control register
IPRI    EQU  $FFFF    ; interrupt priority register
;
        ORG P:$0000
; interrupt vector [00] - hardware reset
        JMP <START                                      ; jump start
        ORG P:$0024
; interrupt vector [12] - host command 'STOP'
        BCLR #$02,Y:<$04                                ; clear PLAY bit
        NOP                          			; [place holder]
; interrupt vector [13] - host command 'PLAY'
        BSET #$02,Y:<$04                                ; set PLAY bit
        NOP                                             ; [place holder]
; interrupt vector [14] - host command 'RATE'
        JSR >SETRATE                                    ; call setrate()
; interrupt vector [15] - host command 'SETPOT'
        JSR >SETPOT                                     ; call setpot()
; interrupt vector [16] - host command 'SCALE1'
        JSR >SCALE1                                     ; call scale1()
; interrupt vector [17] - host command 'SCALE2'
        JSR >SCALE2                                     ; call scale2()
;
        ORG P:$0040
;
; interrupt routine [00] - hardware reset
;
START   ORI #$03,MR                             ; mask interrupts
        CLR A                                   ; put zero in reg A
        MOVEP #>$0004,X:<<PABC                  ; set mem wait states
        MOVEP #>$0001,X:<<PBCR                  ; Port-B set periph
        MOVEP #>$0004,X:<<HOCR                  ; Host cmd interrupt enable
        MOVEP A,X:<<PCCR                        ; Port-C clear control reg
        MOVEP #>$0008,X:<<PCDD                  ; Port-C cfg pin3 output
        MOVEP #>$FFFFE7,X:<<PCCR                ; Port-C periph except pin3,4
        MOVEP A,X:<<STRD                        ; SSI clear data reg
        MOVEP #>$6100,X:<<SSCA                  ; SSI 2wrd/frm, 24bit/wrd
        MOVEP #>$FA00,X:<<SSCB                  ; SSI T/R enbl, net.mode
                                                ; cont.clk
        MOVEP #>$0400,X:<<IPRI                  ; IntPrior HST=1 SCI=0 SSI=0
        MOVEP A,X:<<STRD                        ; SSI clear data reg
        MOVE                     A,Y:<$04       ; clear PLAY/RECORD flags
        MOVEP #>$150003,X:<<PLLC
        MOVEP #>$000B,Y:<<SRCR
        MOVEP #>$008F,Y:<<VOLC
        MOVEC  A,SR                             ; enable interrupts
IDLUP   JSSET #$02,Y:<$04,>S0                   ; chk PLAY cmd
        JMP <IDLUP                              ; loop back
;
;    PLAY command
;
S0      
        JSR S1					; fetch paramters

; initialize variables

        CLR A                                   ; CLEAR REG A
        MOVE                     A,X:MIN1
        MOVE                     A,X:MAX1
        MOVE                     A,X:MIN2
        MOVE                     A,X:MAX2
        MOVE                     A,X:SQ1LO
        MOVE                     A,X:SQ1HI
        MOVE                     A,X:SQ1HIR
        MOVE                     A,X:SQ2LO
        MOVE                     A,X:SQ2HI
        MOVE                     A,X:SQ2HIR
        MOVE                     X:NPTS,A
        MOVE                     X:<SPIDX,B
        SUB                      B,A
        MOVE                     A,X:SP_LOC
        MOVE                     X:SWEEPS,A     ; load SWEEPS
        MOVE                     X:SKIP,B       ; load SKIP
        ADD                      B,A            ; compute SWEEPS+SKIP
        MOVE                     A,X:<SWPCNT    ; store as SWPCNT
        MOVE                     #>SPSTRT1,R4   ; load start of SP1 buf
        MOVE                     #>SPSTRT2,R5   ; load start of SP2 buf

; Zero accumulator buffers

        CLR A                                   ; CLEAR REG A
        MOVE                     X:<ACC1,R2     ; load start of ACC1 buf
        MOVE                     X:<ACC2,R3     ; load start of ACC2 buf
        DO X:<NPTS,>L0                          ; loop over NPTS
        MOVE                     A,Y:(R2)+      ; zero ACC1 buf (24b)
        MOVE                     A,Y:(R3)+      ; zero ACC2 buf (24b)
L0      NOP                                     ; bottom of NPTS loop

;
        JCLR #$02,Y:<$04,>WRBK                  ; chk for early exit
        BCLR #$03,Y:<$04                        ; clear 'ACC' bit
;
;   sync to DAC A/B
;
SYNC0   MOVEP #>$0000,X:<<STRD                  ; output zero to DAC
SYNC1   JCLR #$06,X:<<SSSR,>SYNC1               ; wait for txd empty
        JSET #$02,X:<<SSSR,>SYNC0               ; loop if no frame-sync occurred
SYNC2   MOVEP #>$0000,X:<<STRD                  ; output zero to DAC
SYNC3   JCLR #$06,X:<<SSSR,>SYNC3               ; wait for txd empty
        JSET #$02,X:<<SSSR,>SYNC2               ; loop if frame-sync occurred
;
        DO X:<SWPCNT,>SWLUP                     ; loop over SWPCNT
        MOVE                     X:<DAC1,R1     ; load start of DAC1 buf
        MOVE                     X:<ACC1,R2     ; load start of ACC1 buf
        MOVE                     X:<ACC2,R3     ; load start of ACC2 buf
        MOVEC                    LC,A           ; load loop count
        MOVE                     X:<SWEEPS,B    ; load SWEEPS
        SUB                      B,A            ; chk if loop count > SWEEPS
        JGT L1
        BSET #$03,Y:<$04                        ; set 'ACC' bit
L1      NOP
        DO X:<NPTS,>NPLUP                       ; loop over NPTS samples
; Check for SP
        BCLR #$04,Y:<$04                        ; clear 'SP' bit
        MOVEC                    LC,A           ; load loop count
        MOVE                     X:<SP_LOC,B    ; load SP location
        SUB                      B,A            ; chk if loop count == SP_LOC
	JNE DAC_1                               ; continue if not SP
        BSET #$04,Y:<$04                        ; set 'SP' bit
DAC_1   JCLR #$06,X:<<SSSR,>DAC_1               ; wait for SSI-txd empty
        MOVEP X:(R1)+,X:<<STRD                  ; output to DAC-A
ADC_1   JCLR #$07,X:<<SSSR,>ADC_1               ; wait for SSI-rxd full
        MOVEP X:<<STRD,A                        ; input ADC-A
        JCLR #$03,Y:<$04,>END_1                 ; chk for skip acc
        MOVE                     A,X:<TEMP      ; save ADC value
        REP #$008                               ; repeat 8 times ...
        ASR A                                   ; ... shift right reg A
        MOVE                     Y:(R2),B       ; load prev 24b total
        ADD                      B,A            ; compute new 24b total
        MOVE                     A,Y:(R2)+      ; store 24b acc
        MOVE                     X:<TEMP,A      ; restore AD value
SP_1    JCLR #$04,Y:<$04,>MAX_1                 ; chk for SP
        MOVE                     A,X:(R4)+      ; save value in SP1 list
MAX_1   MOVE                     X:MAX1,B       ; load AD max
        SUB A,B                                 ; subtract AD value from max
        JGE MIN_1                               ; proceed if max >= value
        MOVE                     A,X:MAX1       ; store new AD max
MIN_1   MOVE                     X:MIN1,B       ; load AD min
        SUB A,B                                 ; subtract AD value from min
        JLE END_1                               ; proceed if min <= value
        MOVE                     A,X:MIN1       ; store new AD min
END_1   NOP

DAC_2   JCLR #$06,X:<<SSSR,>DAC_2               ; wait for SSI txd empty
        MOVEP #>$0000,X:<<STRD                  ; send zero to DAC-B
ADC_2   JCLR #$07,X:<<SSSR,>ADC_2               ; wait for SSI rxd full
        MOVEP X:<<STRD,A                        ; input ADC-B
        JCLR #$03,Y:<$04,>END_2                 ; chk for skip acc
        MOVE                     A,X:<TEMP      ; save ADC value
        REP #$008                               ; repeat 8 times ...
        ASR A                                   ; ... shift right reg A
        MOVE                     Y:(R3),B       ; load prev 24b total
        ADD                      B,A            ; compute new 24b total
        MOVE                     A,Y:(R3)+      ; store 24b acc
        MOVE                     X:<TEMP,A      ; restore ADC value
SP_2    JCLR #$04,Y:<$04,>MAX_2                 ; chk for SP
        MOVE                     A,X:(R5)+      ; save value in SP2 list
MAX_2   MOVE                     X:MAX2,B       ; load AD max
        SUB A,B                                 ; subtract AD value from max
        JGE MIN_2                               ; proceed if max >= value
        MOVE                     A,X:MAX2       ; store new AD max
MIN_2   MOVE                     X:MIN2,B       ; load AD min
        SUB A,B                                 ; subtract AD value from min
        JLE END_2                               ; proceed if min <= value
        MOVE                     A,X:MIN2       ; store new AD min
END_2   NOP

CHKX    JSET #$02,Y:<$04,>D1                    ; chk for early exit
        ENDDO
	JMP NPLUP
D1      NOP
NPLUP   NOP                                     ; bottom of NPTS loop
;
        MOVEP #>$01,X:<<HOTR                    ; send '1' to host
        JSET #$02,Y:<$04,>D2                    ; chk for early exit
        ENDDO
	JMP SWLUP
D2      NOP
SWLUP   NOP                                     ; bottom of SWPCNT loop
;
        JCLR #$02,Y:<$04,>RET0                  ; chk for early exit
WRBK
        MOVE                     X:<ACC1,R2     ; load start of ACC1 buf
        JSR WRBK32                              ; write back 32b data
        MOVE                     X:<ACC2,R2     ; load start of ACC2 buf
        JSR WRBK32                              ; write back 32b data

;   signal host that data is ready

READY   JCLR #$06,X:<<SSSR,>READY               ; wait for SSI-txd
        MOVEP #>$0000,X:<<STRD                  ; send zero to DAC-A
        BCLR #$02,Y:<$04                        ; clear 'PLAY' bit
WH      JCLR #$01,X:<<HOSR,>WH                  ; wait for HOST-txd empty
        MOVEP #>$00,X:<<HOTR                    ; send '0' to host
RET0    RTS                                     ; return
;
; routine to write 24b Y-data back to 32b X-data
;
WRBK32
        MOVE                     R2,R3          ; copy start of ACC buf
        DO X:<NPTS,>WBL1                        ; loop over NPTS
        MOVE                     Y:(R2)+,A      ; load ACC1 buf (24b)
        REP #$008                               ; repeat 8 times ...
        ASL A                                   ; ... shift left reg A
        MOVE                     A1,X:(R3)+     ; store ACC1 (32b) low
        REP #$010                               ; repeat 16 times ...
        ASR A                                   ; ... shift right reg A
        MOVE                     A1,X:(R3)+     ; store ACC1 (32b) high
WBL1                                            ; bottom of NPTS loop
        RTS                                     ; return
;
;  routine to fetch paramters
;
S1
        MOVE                     #>PRMBLK,R0    ; load param block location
        MOVE                     #>TOPMEM,R1    ; load on-chip data location
        MOVE                     #>$7FFF,Y0     ; mask clears upper 5 bits
                                                ; to covert TMS320 addr
        DO #NP,>L3                              ; convert parm data
        CLR A                                   ; clear reg A
        MOVE X:(R0)+,A1                         ; read uploaded parms
        REP #$008                               ; repeat 8 times ...
        ASR A                                   ; ... shift right reg A
        AND Y0,A                                ; clr upper 5 bits
        MOVE A,X:(R1)+                          ; store shifted parms
L3                                              ; bottom of loop
        RTS                                     ; return
;
; interrupt routine [14] - host command 'SETRATE'
;
SETRATE
        JCLR #$00,X:<<HOSR,>SETRATE             ; wait for host data
        MOVEP X:<<HOTR,A                        ; read host data into reg A
        BSET #$00,Y:<<CLKE                      ; enable sample clock
        MOVEP A,Y:<<SRCR                        ; set sample clock rate
        RTI                                     ; return from SETRATE
;
; interrupt routine [15] - host command 'SETPOT'
;
SETPOT   
        JCLR #$00,X:<<HOSR,>SETPOT              ; wait for host data
        MOVEP X:<<HOTR,A1                       ; read host data into reg A1
                                                ; A1=LLRRPP: left,right,POT
        JCLR #$00,A1,>EVEN                      ; check pot-select bit 0
        JCLR #$01,A1,>POT1                      ; select POT1
POT3    JMP <SPOT3                              ; select POT3
POT2    MOVE                     #>$008B,X0     ; POT2 & AMPS bits
        JMP <SPOT1
EVEN    JCLR #$01,A1,>POT0
POT1    MOVE                     #>$008D,X0     ; POT1 & AMPS bits
        JMP <SPOT1
POT0    MOVE                     #>$008E,X0     ; POT0 & AMPS bits
SPOT1   MOVEP X0,Y:<<VOLC                       ; enable AMPS and selected pot
        DO #$010,>SPOT3                         ; loop over LLRR bits
        ROL A                                   ; rotate out leading bit
        JCC >SPOT2                              ; if CARRY=0, then pot-bit=0
        BSET #$05,X0                            ; otherwise vol-bit=1
SPOT2   MOVEP X0,Y:<<VOLC                       ; output vol bit w/ lo clock 
        BSET #$06,X0                            ; set clock bit
        MOVEP X0,Y:<<VOLC                       ; output vol bit w/ hi clock
        BCLR #$05,X0                            ; clear clock bit
        BCLR #$06,X0                            ; clear vol bit
        MOVEP X0,Y:<<VOLC                       ; output lo vol & clock bits
SPOT3
        MOVEP #>$008F,Y:<<VOLC                  ; enable AMPS
        RTI                                     ; return from SETPOT
;
; interrupt routine [16] - host command 'SCALE1'
;
SCALE1
	JCLR #$00,X:<<HOSR,>SCALE1              ; wait for host data
        MOVEP X:<<HOTR,Y1                       ; read host data into reg A
        MOVE                     X:<DAC1,R0     ; load offset into DAC1 buf
        JSR SCLBUF
        RTI
;
; interrupt routine [17] - host command 'SCALE2'
;
SCALE2
	JCLR #$00,X:<<HOSR,>SCALE2              ; wait for host data
        MOVEP X:<<HOTR,Y1                       ; read host data into reg A
        NOP                                     ; do nothing (no DAC2 buffer)
        RTI
;
SCLBUF
        DO X:<NPTS,>SCLUP                       ; loop over NPTS samples
        MOVE X:(R0),Y0                          ; put DAC buf value in Y0
        MPY Y1,Y0,A                             ; multipy Y1*Y0 => A
        MOVE                     A1,X:(R0)+     ; store scaled value
SCLUP   NOP                                     ; end of scale loop
        RTS

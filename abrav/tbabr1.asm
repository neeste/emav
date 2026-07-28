; tbabr1.asm - DSP code for ABRAV with TB Monterey/Tahiti soundcard
;
;*****************************************************************
; M56001 assembly source code (use a56 to assemble)
;*****************************************************************
;
; The Turtle Beach "Multisound" (Monterey or Tahiti) has 32KW of 
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
; The soundcard has a 16-bit, stereo A/D converter (Crystal CS5336) 
; and an 18-bit, stereo D/A convertor (Crystal CS4328).
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
        MOVEP #>$0400,X:<<IPRI                  ; IntPrior HST=1 SCI=0 SSI=0
        MOVEP #>$0004,X:<<HOCR                  ; Host cmd int. enable
        MOVEP #>$6100,X:<<SSCA                  ; SSI 2wrd/frm, 24bit/wrd
        MOVEP #>$3A00,X:<<SSCB                  ; SSI netmode, T/R enbl/sync
        CLR A                                   ; put zero in reg A
        MOVEP A,X:<<STRD                        ; SSI transmit a zero
        MOVEP #>$3330,X:<<PABC                  ; X,Y,P mem set 3 wait st.
        MOVEP #>$0001,X:<<PBCR                  ; Port-B set periph
        MOVEP A,X:<<PCCR                        ; Port-C set gp I/O
        MOVEP #>$0004,X:<<PCDD                  ; Port-C cfg pin-2 output
        MOVEP #>$FFFFFB,X:<<PCCR                ; Port-C periph except pin-2
        MOVEP A,X:<<STRD                        ; SSI transmit a zero
        MOVE                     A,Y:<$04       ; clear play/rec flags
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
        BSET #$00,Y:<<$FFC4                     ; ??
        MOVEP A,Y:<<$FFC3                       ; set sample rate
        MOVEP A,Y:<<$FFC2                       ; set sample rate
;        MOVEP #>$0003,Y:<<$FFC1                 ; was #>$0011
        MOVEP #>$0011,Y:<<$FFC1                 ; was #>$0011
        MOVEP #>$0000,Y:<<$FFC1                 ; ??
	RTI                                     ; return
;
; interrupt routine [15] - host command 'SETPOT'
;
SETPOT  
        MOVE                     #>$0080,Y0     ; AMPS bit set
        MOVE                     Y0,X0          ; copy Y0 to X0
        BSET #$01,X0                            ; select IN pot
SP1     JCLR #$00,X:<<HOSR,>SP1                 ; wait for host data
        MOVEP X:<<HOTR,A1                       ; get data from HOST
        JCLR #$00,A1,>DO_POT                    ; chk bit 0: 0=IN,1=AUX
        MOVE                     Y0,X0          ; copy Y0 to X0
        BSET #$02,X0                            ; select AUX pot
DO_POT  MOVEP X0,Y:<<$FFC0                      ; enable selected pot
        DO #$010,>SP2                           ; A1=LLRRPP: left,right,pot
        ROL A                                   ; rotate out leading bit
        JCC >BITOUT                             ; if CARRY=0, then pot-bit=0
        BSET #$04,X0                            ; otherwise pot-bit=1
BITOUT  MOVEP X0,Y:<<$FFC0                      ; output pot-bit w/ lo clock
        BSET #$03,X0                            ; set clock-bit high
        MOVEP X0,Y:<<$FFC0                      ; output pot-bit w/ hi clock
        BCLR #$03,X0                            ; clear clock-bit
        MOVEP X0,Y:<<$FFC0                      ; output pot-bit w/ lo clock
        BCLR #$04,X0                            ; clear pot-bit
SP2                                             ; bottom of do loop
        MOVEP Y0,Y:<<$FFC0                      ; AMPS on with no pot selected
        RTI                                     ; return from subroutine
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


# SISTEMI.BAS - implementazione del metodo Gauss-Jordan per la soluzione dei sistemi di equazioni lineari
100 REM "SISTEMI.BAS"
105 DIM A(100)
110 PRINT "Linear system equations"
115 PRINT "**Gauss-Jordan method**"
120 DELAY 2000
125 INPUT "System size (max 10): ",N
130 IF N>10 THEN GOTO 125
140 M=N+1
150 FOR I=0 TO N-1
160 FOR J=0 TO N
170 INPUT "A(",I,",",J,")=",A(I*M+J) 
180 NEXT 
190 NEXT
200 FOR I=0 TO N-1
210 IF A(I*M+I)=0 THEN GOSUB 400
220 FOR J=0 TO N-1
230 IF  I<>J THEN GOSUB 600
240 NEXT
250 NEXT
255 PRINT "Solutions :"
260 FOR I=0 TO N-1        
270 PRINT "X",I,"=",A(I*M+N)/A(I*M+I)
280 NEXT 
390 END
400 REM SUB **SWAP**
410 C=0
420 C=C+1
430 IF (I+C)<N & A((I+C)*M+I)=0 THEN GOTO 420           
440 IF (I+C)=N THEN GOSUB 700
450 J = I
460 FOR  K=0 TO N
470 T=A(J*M+K)
480 A(J*M+K) = A((J+C)*M+K)
490 A((J+C)*M+K)=T
500 NEXT
510 RETURN
600 REM SUB **CALC**
610 P = A(J*M+I)/A(I*M+I)
620 FOR K=0 TO N                
630 A(J*M+K) = A(J*M+K)-A(I*M+K)*P 
640 NEXT
650 RETURN
700 REM SUB **EXIT**
710 PRINT "NO Solutions !"
720 END

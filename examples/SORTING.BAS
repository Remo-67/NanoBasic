# SORTING.BAS - semplice programma che mostra l'ordinamento di serie di rettangoli.
100 REM "SORTING.BAS"
300 DIM R(10),C(10)
305 PRINTAT 10,10,65535,2,"SORTING VALUES .." 
310 FOR I=0 TO 9
330 R(I)=RAND(80)+20
335 PRINTAT 30*I+30,160-R(I),65535,1,R(I):C(I)=25000+5000*I 
340 RECT 30*I+30,180-R(I),20,R(I),C(I),0,1
350 NEXT
400 I=0:SWAP=0
410 IF R(I)>R(I+1) THEN GOSUB 600
420 I=I+1
430 IF I<9 THEN GOTO 410
435 CLS 0
436 PRINTAT 10,10,65535,2,"SORTING VALUES .." 
440 FOR I=0 TO 9
445 PRINTAT 30*I+30,160-R(I),65535,1,R(I)
450 RECT 30*I+30,180-R(I),20,R(I),C(I),0,1
460 NEXT
465 DELAY 1000
470 IF SWAP=1 THEN GOTO 400
475 PRINTAT 10,10,65535,2,"SORTING VALUES .." 
480 FOR I=0 TO 9
485 PRINTAT 30*I+30,160-R(I),65535,1,R(I)
490 RECT 30*I+30,180-R(I),20,R(I),C(I),0,1
500 NEXT
510 END
600 A=R(I):R(I)=R(I+1):R(I+1)=A
610 SWAP=1
620 RETURN

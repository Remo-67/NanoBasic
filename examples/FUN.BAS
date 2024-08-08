# FUN.BAS - programma che grafica sul display una funzione matematica
# il programma traccia l'andamento della funzione y=sin(x)/x
# XMIN, XMAX sono gli estremi dell'intervallo dell'asse delle ascisse
# il programma calcola automaticamente i valori YMAX e YMIN
# che la funzione raggiunge nell'intervallo XMIN, XMAX e 
# adatta la scala del tracciato.
100 REM "FUN.BAS"
105 W=320:H=240:PI=3.14159
120 XMAX=6*PI
130 XMIN=-6*PI
140 YMAX=-1E5
150 YMIN=1E5
160 CL=1949:CP=65504
170 D=(XMAX-XMIN)/W
# salta alla subroutine dove vengono calcolati YMAX e YMIN
180 GOSUB 600
190 M=YMAX-YMIN+1
200 X=XMIN
210 FOR I=0 TO W
220 X=X+D
# stampa a video il tracciato della funzione
230 GOSUB 900
240 PIXEL I,H/2-(Y/M)*H,CL 
250 NEXT
300 END
600 REM CALC Y MIN/MAX
610 PRINTAT 10,200,CL,2,"wait, calculating ..."
620 FOR X=XMIN TO XMAX STEP D
630 GOSUB 900
640 IF Y>YMAX THEN YMAX=Y
650 IF Y<YMIN THEN YMIN=Y
660 NEXT:CLS 0
665 PRINTAT 10,5,CL,1,"Y=SIN(X)/X"
670 PRINTAT 10,230,CL,1,"Ymin=",YMIN," : Ymax=",YMAX
680 LINE 0,120,320,120,CP
690 LINE 160,0,160,240,CP
695 XC=XMIN
700 FOR X1=0 TO 320 STEP 64
710 LINE X1,123,X1,117,CP
715 IF X1>0 & X1<320 THEN PRINTAT X1-8,125,CL,1,XC
718 XC=XC+(D*64)
720 NEXT
730 FOR Y1=0 TO 240 STEP 24
740 LINE 158,Y1,163,Y1,CP
750 NEXT
800 RETURN
900 REM FUNCTION TO PLOT
910 Y=SIN(X)/X
920 RETURN

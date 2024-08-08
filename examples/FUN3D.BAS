# FUN3D.BAS – Il programma grafica sul display una funzione 3D. 
# Si tratta di un programma scritto per i vecchi home computer degli anni 80 (ZX spectrum),
# il codice è stato riadattato con alcune piccole modifiche.
100 REM "FUN3D.BAS"
105 DIM M(500)
110 PI=3.141592653:CL=18400
120 A=COS(PI/4)
130 FOR Y=1 TO 241 STEP 5
140 E=A*Y
150 C=Y-120
160 C=C*C 
170 FOR X=1 TO 321
180 D=X-160
185 ARG=0.01*(C+D*D)
190 Z=30*SIN((PI/2)*SQRT(ARG))
200 X1=X+E
210 Y1=Z+E
220 IF Y1 >= M(X1) THEN GOSUB 300
225 IF Y1 < M(X1) THEN GOSUB 600
240 NEXT
250 NEXT
260 END
300 M(X1)=Y1
310 IF X=1 THEN GOSUB 400
315 IF x>1 THEN GOSUB 500
320 RETURN
400 PIXEL X1-60,220-Y1,CL
410 XO=X1:YO=Y1
420 RETURN
500 IF XO>60 THEN LINE XO-60,220-YO,X1-60,220-Y1,CL
510 XO=X1:YO=Y1
520 RETURN
600 XO=0
610 RETURN

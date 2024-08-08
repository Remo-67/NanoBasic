# BLINK.BAS - il classico esempio del LED lampeggiante. 
# Il LED va collegato tramite una resistenza di 470 ohm tra il pin 3 e la massa (GND) 
100 REM "BLINK.BAS"
105 PIN=3
110 PINMODE PIN,1
120 PRINTAT 100,100,65300,3,"<EXIT>"
130 IF GETX()>100 & GETX()<200 & GETY()>100 & GETY()<130 THEN GOTO 200
140 DWRITE PIN,1
150 DELAY 500
160 DWRITE PIN,0
170 DELAY 500
180 GOTO 130
200 PRINTAT 100,100,0,3,"<EXIT>"
210 END



/*
    NanoBASIC a simple basic interpreter with a TFT graphic touch screen     
                          NanoBASIC ver 1.0.3.5



license:
_____________________________________________________________________________
MIT License

Copyright (c) 2022-2023 Remo Riglioni

Permission is hereby granted, free of charge, to any person obtaining  a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction,  including without limitation the rights
to use,  copy,  modify,  merge,  publish, distribute, sublicense, and/or sell
copies of the Software,  and  to  permit   persons  to whom   the Software is 
furnished to do so, subject to the following conditions:

The  above copyright  notice  and this permission notice shall be included in 
all copies or substantial portions of the Software.

THE SOFTWARE  IS PROVIDED "AS IS",   WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED,  INCLUDING BUT NOT   LIMITED   TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE  AND  NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR  COPYRIGHT   HOLDERS BE LIABLE   FOR ANY CLAIM,   DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/



/*
 Hardware:

 Board: Arduino Nano Every  
 https://docs.arduino.cc/hardware/nano-every
 
 
 Display: 2.8 inch ILI9341 240x320 SPI TFT LCD Display
 http://www.lcdwiki.com/2.8inch_SPI_Module_ILI9341_SKU:MSP2807

*/


#include <avr/wdt.h>
#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ILI9341.h>
#include <XPT2046_Touchscreen.h>


#include <SdFat.h>

/*
very important !                                                      

SdFat library ver. 2.1.2                                     

For minimum flash size use these settings:                   

in SdFatConfig.h                        


#define USE_FAT_FILE_FLAG_CONTIGUOUS 0                      
#define ENABLE_DEDICATED_SPI 0                               
#define USE_LONG_FILE_NAMES 0                                
#define SDFAT_FILE_TYPE 1                                   
*/


#include "nanoBasic.h"

/*

graphic display and touch instantiation

*/

Adafruit_ILI9341 tft = Adafruit_ILI9341(TFT_CS, TFT_DC, TFT_RST);
XPT2046_Touchscreen ts(TS_CS);

/*

general utility functions 

*/


/* 
 
 updating touch screen tx, ty coordinates
 tx and ty are global variables
 
*/

void getXYCoord(TS_Point p) {
 tx = round((p.x * XCALM) + XCALC);
 ty = round((p.y * YCALM) + YCALC);
 if(tx < 0) tx = 0;
 if(tx >= tft.width()) tx = tft.width() - 1;
 if(ty < 0) ty = 0;
 if(ty >= tft.height()) ty = tft.height() - 1;
}



/*

  returns the available RAM memory   

*/

int availableMemory () {  
  extern char *__brkval;
  extern int __heap_start; 
  int v;
  return (int) &v - (__brkval == 0 ? (int) &__heap_start : (int) __brkval);
}


/*

remove initial and final spaces  of an input string  

*/

char *stringTrim(char* s1) {
  int i = 0;
  while (s1[i] == ' ') i++;
  strcpy(s1, &s1[i]);
  i = strlen(s1) - 1;
  while (s1[i] == ' ') i--;
  s1[i + 1] = '\0';
  return s1;
}


/*
 
returns a sub string index position  
(s1 = substring)                  

*/

int getSubStringIndex(char *s1, char *s2) {
  int i = 0;
  int M = strlen(s1);
  int N = strlen(s2);
  for (i = 0; i <= N - M; i++) {
    int j;
    for (j = 0; j < M; j++)
      if (s2[i + j] != s1[j]) break;
    if (j == M) return i;
  }
  return -1;
}

/*         

returns program instruction code 

*/

PRGINS getInstructionCode(char *identifier) {
  char keystring[MINSTRING] = ",";
  int id = -1;
  strcat(keystring, identifier);
  strcat(keystring, ",");
  id = getSubStringIndex(strupr(keystring),(char *)prg_ins_list);
  if (id != -1)
    return (PRGINS)id;
  return NOINS;
}



/*         

returns program function code 

*/

PRGFUN getFunctionCode(char *identifier) {
  char keystring[MINSTRING] = ",";
  int id = -1;
  strcat(keystring, identifier);
  strcat(keystring, ",");
  id = getSubStringIndex(strupr(keystring), (char *)prg_fun_list);
  if (id != -1)
    return (PRGFUN)id;
  return NOFUN;
}


/*    

returns BASIC command code           

*/


BASCMD getCommandCode(char *identifier) {
  char keystring[MAXSTRING] = ",";
  int i = 1, j = 0, id = -1;
  while (identifier[j] != '\0') {
   if (identifier[j] == ' ') break;
   keystring[i++] = identifier[j++];
  }
  keystring[i++] = ','; 
  keystring[i]  = '\0';
  id = getSubStringIndex(strupr(keystring),(char *)bas_cmd_list);
  if (id != -1)
    return (BASCMD)id;
  return NOCMD;
}



/*    

check if a identifier is a reserved keyword

*/


bool isReservedIdentifier(char *identifier) {
  if (getInstructionCode(identifier) == NOINS &&
      getFunctionCode(identifier)    == NOFUN &&
      getCommandCode(identifier)     == NOCMD)
    return false;
  return true;
}




/*    

check if a variable is an Array     

*/

bool isArray(char *var_name) {
  struct variables_struct *s = varlist_ptr;
  while (s != NULL) {
    if (!strcmp(s->var_name, var_name) && s->var_size > 1)
      return true;
    s = s->next;
  }
  return false;
}




/*    

update/set the numerical value at  "array_index" position into array variable     

*/


ERR updateArrayValues(char  *var_name, double var_value, int array_index) {
  struct variables_struct *s = varlist_ptr;
  if (isReservedIdentifier(var_name)) return RESERVED;
  while (s != NULL) {
    if (!strcmp(s->var_name, var_name)) {
      if (s->var_size > array_index) {
        s->var_value[array_index] = var_value;        
        return NO_ERROR;
      } else
        return OUT_OF_BOUNDS;
    }
    s = s->next;
  }
  return ARRAY_UNDEFINED;
}



/*    

returns the value stored at the position "array_index" of an array variable     

*/

double getArrayValue(char *var_name, int array_index) {
  struct variables_struct *s = varlist_ptr;
  while (s != NULL) {
    if (!strcmp(s->var_name, var_name))
      return s->var_value[array_index];
    s = s->next;
  }
  return 0.0;
}


/*
 
inserts a new variable into the list or updates its value 

*/

ERR insertOrUpdateVariable(char *var_name, double var_value, int var_size) {
  int i = 0, var_name_length = strlen(var_name) + 1;
  if (isReservedIdentifier(var_name)) return RESERVED;
  if (varlist_ptr == NULL) {
    varlist_ptr = (struct variables_struct *)malloc(sizeof(struct variables_struct));
    if (varlist_ptr == NULL) return OUT_OF_MEMORY;
    varlist_ptr->var_name = (char *)malloc(var_name_length * sizeof(char));
    if (varlist_ptr->var_name == NULL) return OUT_OF_MEMORY;
    strcpy(varlist_ptr->var_name, var_name);
    varlist_ptr->var_size = var_size;
    varlist_ptr->var_value = (double *)malloc(var_size * sizeof(double));
    if (varlist_ptr->var_value == NULL) return OUT_OF_MEMORY;
    if (var_size == 1) {
      varlist_ptr->var_value[0] = var_value;
    } else {
      for (i = 0; i < var_size; i++)
        varlist_ptr->var_value[i] = 0.0;
    }
    varlist_ptr->next = NULL;
  } else {
    struct variables_struct *s = varlist_ptr;
    while (s->next != NULL) {
      if (!strcmp(s->var_name, var_name)) {
        s->var_value[0] = var_value;
        return NO_ERROR;
      }
      s = s->next;
    }
    if (!strcmp(s->var_name, var_name)) {
      s->var_value[0] = var_value;
      return NO_ERROR;
    }
    s->next = (struct variables_struct *)malloc(sizeof(struct variables_struct));
    if (s->next == NULL) return OUT_OF_MEMORY;
    s->next->var_name = (char *)malloc(var_name_length * sizeof(char));
    if (s->next->var_name == NULL) return OUT_OF_MEMORY;
    strcpy(s->next->var_name, var_name);
    s->next->var_size = var_size;
    s->next->var_value = (double *)malloc(var_size * sizeof(double));
    if (s->next->var_value == NULL) return OUT_OF_MEMORY;
    if (var_size == 1) {
      s->next->var_value[0] = var_value;
    } else {
      for (i = 0; i < var_size; i++)
        s->next->var_value[i] = 0.0;
    }
    s->next->next = NULL;
  }
  return NO_ERROR;
}



/*
 
returns the pointer to the position of a variable stored in the list  

*/
struct variables_struct *getVarPointerFromName(char *var_name) {
  struct variables_struct *s = varlist_ptr;
  while (s != NULL) {
    if (!strcmp(s->var_name, var_name))
      return s;
    s = s->next;
  }
  return NULL;
}


/*
 
returns the value of a variable stored in the list        

*/
double getVarValueFromName(char *var_name) {
  struct variables_struct *s = varlist_ptr;
  while (s != NULL) {
    if (!strcmp(s->var_name, var_name))
      return s->var_value[0];
    s = s->next;
  }
  return 0.0;
}






/*
 
deallocated memory used for managing the variable list 

*/

void deleteVariableList() {
  struct variables_struct *current = varlist_ptr;
  struct variables_struct *next    = NULL;
  while (current != NULL) {
    next = current->next;
    free(current->var_name);
    free(current->var_value);
    free(current);
    current = next;
  }
  varlist_ptr = NULL;
}



/*

recursive descent expression parser functions 

*/


/*
 
evaluate the FREAD() function, read a decimal number from an opened file 

*/

double evalFREAD()  {
  char rbuf[MINEXPRES];
  double value=0.0;
  int c,i=0;
  eof = 0; // eof a global variable indicating the end of the file
  while((c = SD_File.read()) >= 0) {     
   if ((char)c != '\r') { // read a single line 
    if ((char)c != '\n')  
     rbuf[i++] = (char) c;
   } else {
     rbuf[i]   = '\0';         
     break;
   }                
  }    
  eof = c; // if  c== -1 the end of the file is reached
  return atof(rbuf) ;
}

/*

evaluate a math function   

*/

double evalFunction(ERR *err, PRGFUN function, double arg) {
  *err = NO_ERROR;
  switch (function) {
    case _SQRT:                    // returns the square root  
      if (arg > 0.0)
        return sqrt(arg);
      else *err = OUT_OF_RANGE;
    case _LOG:                     // returns the common logarithm (base-10 logarithm)
      if (arg > 0.0)
        return log10(arg);
      else *err = OUT_OF_RANGE;
    case _LN:                      // returns the natural logarithm (base-e logarithm)
      if (arg > 0.0)
        return log(arg);
      else *err = OUT_OF_RANGE;
    case _EXP:                     // returns the value of e raised to the xth power
      return exp(arg);
    case _ABS:                     // returns the absolute value of x  
      return fabs(arg);           
    case _SIN:                     // returns the sine of a radian angle
      return sin(arg);            
    case _COS:                     // returns the cosine of a radian angle
      return cos(arg);            
    case _TAN:                     // returns the tangent of a radian angle
      return tan(arg);            
    case _ASIN:                    // returns the arc sine of x in radians.
      return asin(arg);           
    case _ACOS:                    // returns the arc cosine of x in radians.
      return acos(arg);           
    case _ATAN:                    // returns the arc tangent of x in radians.
      return atan(arg);           
    case _RAND:                    // returns a random number from 0 to <arg>
      return (random((int)arg));  
    case _MEM:                     // returns available RAM memory
      return availableMemory();           
    case _AREAD:                   // returns 0..1023 value at analogic pin <arg>
      return analogRead((int)arg);  
    case _DREAD:                   // returns HIGH or LOW value at digital pin <arg>     
      return digitalRead((int)arg); 
    case _FREAD:                   // returns a numeric value read from an opened file
      return evalFREAD();   
    case _GETX:                    // returns x pos in tft touch event
      if (ts.touched()) {
       getXYCoord(ts.getPoint());              
       return tx;
      } else
       return 0.0;      
    case _GETY:                    // returns y pos in tft touch event
      if (ts.touched()) {
       getXYCoord(ts.getPoint());              
       return ty;
      }  else
       return 0.0;        
    case _MILLIS:
     return millis();              // returns the value of millis() function
    case _EOF: {                   
     if (eof == -1)
      return 1.0;                  // return the end of file
     else
      return 0.0;  
    }
    case _INT:
     return trunc(arg);              // returns the value of millis() function
  }
  *err = FUN_ERROR;
  return 0.0;
}


/*
evaluate the last term of math expression, eval a number, variable, array or 
a new expression in parentheses
*/

double evalTermsExpression(ERR *err, char **expr) {
  bool negative = false, chk_array = false, chk_not = false;
  char *end_ptr, identifier[MINSTRING];
  double res = 0.0, value = 0.0;
  int var_index = 0, i = 0;
  PRGFUN function = NOFUN;

  // Skip spaces
  while (**expr == ' ') (*expr)++;
  // Handle the sign before parenthesis (or before number)
  if (**expr == '-') {
    negative = true;
    (*expr)++;
  }
  if (**expr == '+') {
    (*expr)++;
  }
  // Handle boolean not
  if (**expr == '!') {
    chk_not = true;
    (*expr)++;
  }
  if (isalpha(**expr)) { // get identifier
    i = 0;
    while (isalnum(**expr)) {
      identifier[i++] = **expr;
      (*expr)++;
    }
    identifier[i] = '\0';
    if ((chk_array = isArray(identifier))) {
      while (**expr == ' ') (*expr)++;
      if (**expr != '(') {
        // Unmatched opening parenthesis
        *err = PARENTHESIS;
        return 0;
      }
    } else if ((function = getFunctionCode(identifier))) {
      while (**expr == ' ') (*expr)++;
      if (**expr != '(') {
        // Unmatched opening parenthesis
        *err = PARENTHESIS;
        return 0;
      }
    } else {
      res = getVarValueFromName(identifier);
      if (negative) res = -res;
      if (chk_not) {
        if (res)
          res = 0;
        else
          res = 1;
      }
      return res;
    }
  }
  // Check if there is parenthesis
  if (**expr == '(') {
    (*expr)++;
    parenthesis_check++;
    res = evalBooleanExpression(err, expr);
    if (**expr != ')') {
      // Unmatched opening parenthesis
      *err = PARENTHESIS;
      return 0;
    }
    (*expr)++;
    parenthesis_check--;
    if (chk_array)
      value = getArrayValue(identifier, (int)res);
    else if (function)
      value = evalFunction(err, function, res);
    else
      value = res;
    if (negative)
      value = -value;
    if (chk_not) {
      if (value)
        value = 0;
      else
        value = 1;
    }
    return value;
  }
  // It should be a number; convert it to double
  res = strtod(*expr, &end_ptr);
  if (end_ptr == *expr) {
    // Report error
    *err = WRONG_CHAR;
    return 0;
  }
  // Advance the pointer and return the result
  *expr = end_ptr;
  if (negative)
    res = -res;
  if (chk_not) {
    if (res)
      res = 0;
    else
      res = 1;
  }
  return res;
}

/*

evaluate power expression (x^y)

*/
double evalPowerExpression(ERR *err, char **expr) {
  OPERATOR op = NOP;
  double num1, num2;
  char *pos;
  num1 = evalTermsExpression(err, expr);
  for (;;) {
    // Skip spaces
    while (**expr == ' ') (*expr)++;
    // Save the operation and position
    if (**expr == '^')
      op = POW;
    else
      op = NOP;
    pos = *expr;
    if (op != POW) return num1;
    (*expr)++;
    num2 = evalTermsExpression(err, expr);
    // Perform the saved operation
    num1 = pow(num1, num2);
  }
}



/*

evaluate multiplication or division expression (x*y , x/y)

*/
double evalProductsExpression(ERR *err, char **expr) {
  OPERATOR op = NOP;
  double num1, num2;
  char *pos;
  num1 = evalPowerExpression(err, expr);
  for (;;) {
    // Skip spaces
    while (**expr == ' ') (*expr)++;
    // Save the operation and position
    if (**expr == '/')
      op = DIV;
    else if (**expr == '*')
      op = MUL;
    else
      op = NOP;
    pos = *expr;
    if (op != DIV && op != MUL)
      return num1;
    (*expr)++;
    num2 = evalPowerExpression(err, expr);
    // Perform the saved operation
    if (op == DIV) {
      // Handle division by zero
      if (num2 == 0) {
        *err = DIVIDE_BY_ZERO;
        return 0;
      }
      num1 /= num2;
    } else
      num1 *= num2;
  }
}



/*

evaluate additions and subtractions expression (x+y , x-y)

*/
double evalSumsExpression(ERR *err, char **expr) {
  OPERATOR op = NOP;
  double num1, num2;
  num1 = evalProductsExpression(err, expr);
  for (;;) {
    // Skip spaces
    while (**expr == ' ') (*expr)++;
    if (**expr == '-')
      op = SUB;
    else if (**expr == '+')
      op = ADD;
    else
      op = NOP;
    if (op != SUB && op != ADD)
      return num1;
    (*expr)++;
    num2 = evalProductsExpression(err, expr);
    if (op == SUB)
      num1 -= num2;
    else
      num1 += num2;
  }
}



/*

evaluate conditional expression  
(x>y , x>=y, x<y, x<=y, x<>y, x=y)

*/

double evalConditionalExpression(ERR *err, char **expr) {
  OPERATOR op = NOP;
  double num1, num2;
  num1 = evalSumsExpression(err, expr);
  for (;;) {
    // Skip spaces
    while (**expr == ' ')  (*expr)++;
    // relations operators
    switch (**expr) {
      case '<':
        op = LT;
        (*expr)++;
        if (**expr == '=')
          op = LTE;
        else if (**expr == '>')
          op = NEQ;
        else
          (*expr)--;
        break;
      case '>':
        op = GT;
        (*expr)++;
        if (**expr == '=')
          op = GTE;
        else
          (*expr)--;
        break;
      case '=':
        op = EQ;
        break;
      default:
        op = NOP;
    }
    if (op != LT  && op != LTE && op != GT &&
        op != GTE && op != NEQ && op != EQ)  return num1;
    (*expr)++;
    num2 = evalSumsExpression(err, expr);
    switch (op) {
      case LT:
        if (num1 <   num2)
          num1 = 1;
        else
          num1 = 0;
        break;
      case LTE:
        if (num1 <=  num2)
          num1 = 1;
        else
          num1 = 0;
        break;
      case GT:
        if (num1 >   num2)
          num1 = 1;
        else
          num1 = 0;
        break;
      case GTE:
        if (num1 >=  num2)
          num1 = 1;
        else
          num1 = 0;
        break;
      case NEQ:
        if (num1 !=  num2)
          num1 = 1;
        else
          num1 = 0;
        break;
      case EQ:
        if (num1 ==  num2)
          num1 = 1;
        else
          num1 = 0;
        break;
    }
  }
}



/*

evaluate boolean expression 
 
and = x&y 
or  = x|y

*/
double evalBooleanExpression(ERR *err, char **expr) {
  OPERATOR op = NOP;
  double num1, num2;
  num1 = evalConditionalExpression(err, expr);
  for (;;) {
    // Skip spaces
    while (**expr == ' ')  (*expr)++;
    // relations operators
    switch (**expr) {
      case '&':
        op = AND;
        break;
      case '|':
        op = OR;
        break;
      default:
        op = NOP;
    }
    if (op != AND  && op != OR)  return num1;
    (*expr)++;
    num2 = evalConditionalExpression(err, expr);
    if (op == AND) {
      if (num1 && num2)
        num1 = 1;
      else
        num1 = 0;
    } else if (op == OR) {
      if (num1 || num2)
        num1 = 1;
      else
        num1 = 0;
    }
  }
}


/*     

evaluate a mathematical, conditional or boolean expression  

*/

double evalMathExpression(ERR *err, char* expr) {
  double res;
  parenthesis_check = 0;
  res = evalBooleanExpression(err, &expr); 
  // Now, expr should point to '\0', and parenthesis_check should be zero
  if (parenthesis_check != 0 || *expr == ')') {
    *err = PARENTHESIS;
    return 0.0;
  }
  if (*expr != '\0') {
    *err = WRONG_CHAR;
    return 0.0;
  }
  return res;
}


/*

TFT Graphics User interface functions  

*/

void clearScreen(uint16_t sc) {
  tft.fillScreen(sc);
}

void setFontSizeAndColor(int ts,uint16_t tc){
  tft.setTextSize(ts);
  tft.setTextColor(tc);
}

void moveCursorTo(uint16_t x,uint16_t y) {
  tft.setCursor(x,y);
} 

 
void setupDisplay() {
  tft.begin();
  tft.setRotation(3);
  clearScreen(BACKGROUND);
  setFontSizeAndColor(2,TEXTCOLOR);
}


void setupTouch() {
  ts.begin();
  ts.setRotation(3);
}

/*

print error message

*/

void printErrorMessage(int id, int exit_at_line) {
  clearScreen(BACKGROUND);
  setFontSizeAndColor(2,TEXTCOLOR);
  moveCursorTo(0,0);
  if (exit_at_line) {
   tft.print(F("At line : "));
   tft.println(exit_at_line);
  } else 
   tft.println(F("Statement execution")); 
  moveCursorTo(0, 20);
  setFontSizeAndColor(2,RED);
  tft.println("ERROR, ");
    moveCursorTo(0, 40);
  tft.println(error_list[id]);
  exitTouchedButton(exit_label);  // wait touch screen to exit //
}


/*

draw touch screen keyboard 

*/
void drawKeyBoard(uint16_t button_color, uint16_t text_color) {
  char lbl[2], rtn[4];
  uint16_t i, p, j = 0;
  lbl[1] = '\0';
  tft.setTextSize(2);
  tft.setTextColor(text_color);
  p = SCR_LEFT_GAP;
  tft.drawRoundRect(p - 2, 126, 315, 112, 4, text_color);
  tft.drawRoundRect(0, 124, 319, 116, 4, text_color);
  for (i = 0; i < 12; i++) {
    tft.fillRoundRect(p, 128, 25, 20, 4, button_color);
    if (i == 11) { // back canc //
     moveCursorTo(p + 1, 131);
     tft.print("<-");
     j++;      
    } else {
     moveCursorTo(p + 7, 131);
     lbl[0] = keylist[j++];
     tft.print(lbl);      
    }
    p += 26;
  }
  p = SCR_LEFT_GAP;
  for (i = 0; i < 12; i++) {
    tft.fillRoundRect(p, 150, 25, 20, 4, button_color);
    moveCursorTo(p + 7, 153);
    lbl[0] = keylist[j++];
    tft.print(lbl);
    p += 26;
  }
  p = SCR_LEFT_GAP;
  for (i = 0; i < 12; i++) {
    tft.fillRoundRect(p, 172, 25, 20, 4, button_color);
    moveCursorTo(p + 7, 175);
    lbl[0] = keylist[j++];
    tft.print(lbl);
    p += 26;
  }
  p = SCR_LEFT_GAP;
  for (i = 0; i < 12; i++) {
    tft.fillRoundRect(p, 194, 25, 20, 4, button_color);
    moveCursorTo(p + 7, 197);
    lbl[0] = keylist[j++];
    tft.print(lbl);
    p += 26;
  }
  p = SCR_LEFT_GAP;
  for (i = 0; i < 12; i++) {
    if (i== 0) { // white space //
     tft.fillRoundRect(p, 216, 51, 20, 4, button_color);
     moveCursorTo(p + 9, 222);
     tft.setTextSize(1);
     tft.print("space");
     tft.setTextSize(2);
     j++;
    } else
    if (i==10) { // return //
     tft.fillRoundRect(p, 216, 51, 20, 4, button_color);
     moveCursorTo(p + 7, 219);
     rtn[0] = '<'; rtn[1] = 195;
     rtn[2] = 216; rtn[3] = '\0';
     tft.print(rtn);
     j++;    
    } else 
    if (i==1 || i==11) 
     j++;
    else { 
     tft.fillRoundRect(p, 216, 25, 20, 4, button_color);
     moveCursorTo(p + 7, 219);
     lbl[0] = keylist[j++];
     tft.print(lbl);
    }
    p += 26;    
  }
}


/*

return the touched key code 

*/
int getTouchedButton() {
  uint16_t px = 0, py = 0;
  int i, p, j = 0;
  checkFordisplayTouch(&px, &py);
  p = SCR_LEFT_GAP;
  for (i = 0; i < 12; i++) {
    if (px > p && px < p + 25 && py > 124 && py < 144)
     if (i == 11)
      return KEY_DEL; // back canc //
     else
      return j;  
    j++; p += 26;
  }
  p = SCR_LEFT_GAP;
  for (i = 0; i < 12; i++) {
    if (px > p && px < p + 25 && py > 146 && py < 166) 
     return j; 
    j++; p += 26;   
  }
  p = SCR_LEFT_GAP;
  for (i = 0; i < 12; i++) {
    if (px > p && px < p + 25 && py > 168 && py < 188) 
     return j; 
    j++; p += 26;
  }
  p = SCR_LEFT_GAP;
  for (i = 0; i < 12; i++) {
    if (px > p && px < p + 25 && py > 190 && py < 210) 
     return j; 
    j++; p += 26;
  }
  p = SCR_LEFT_GAP;
  for (i = 0; i < 12; i++) {
    if (px > p && px < p + 25 && py > 212 && py < 232)
    switch (i) {
      case 0:
      case 1:
        return KEY_SPACE;
      case 10:
      case 11:
        return KEY_RETURN;
      default:
       return j;
    }    
    j++; p += 26;
  }
  return KEY_NULL;
}


/*

return the touched x,y coords

*/
void checkFordisplayTouch(uint16_t *x, uint16_t *y) {
  if (ts.touched()) {
    if (!insert)
      touch = true;
    else
      touch = false;
    getXYCoord(ts.getPoint());  
    *x = tx;  //Get touch point
    *y = ty;
    return;
  }
  insert = false;
}


/*

set cursor to background color

*/

void clearCursor(uint16_t x, uint16_t y) {
  char c[2];
  c[0] = 218; c[1] = '\0';
  moveCursorTo(x, y);
  setFontSizeAndColor(2,BACKGROUND);
  tft.print(c);
}


/*

cursor blinking

*/

void printBlinkCursor(uint16_t x, uint16_t y, uint16_t text_color, short int blinkTime ) {
  static bool blinkCursor = false;
  static long blinkMillis = 0;
  char c[2];
  c[0] = 218; c[1] = '\0';
  moveCursorTo(x, y);
  tft.setTextSize(2);
  if ((millis() - blinkMillis) > blinkTime) {
    if (blinkCursor)
      blinkCursor = false;
    else
      blinkCursor = true;
    blinkMillis = millis();
  }
  if (blinkCursor) {
    tft.setTextColor(text_color);
  } else
    tft.setTextColor(BACKGROUND);
  tft.print(c);
}


void printStringAt(uint16_t x, uint16_t y, char *text, uint16_t text_color) {
  moveCursorTo(x, y);
  setFontSizeAndColor(2,text_color);
  tft.print(text);
}


/*

key to touch to exit or to display the information message

*/
void exitTouchedButton(char *label) {
  uint16_t px = 0, py = 0;
  tft.drawRoundRect(255, 206, 51, 20, 4, TEXTCOLOR);
  tft.setTextColor(TEXTCOLOR);
  moveCursorTo(270, 209);
  tft.print(label);
  if (info) {
   tft.drawRoundRect(19, 206, 51, 20, 4, TEXTCOLOR);
   moveCursorTo(40, 209);
   tft.print("?");
  }
  while (1) {
    checkFordisplayTouch(&px, &py);
    if (px > 255 && px < 306 && py > 208 && py < 228) {
      info = false;
      return;
    }  
    if (info && px>19 && px<70 && py>208 && py < 228){
     clearScreen(BACKGROUND);
     tft.setTextColor(YELLOW);
     moveCursorTo(15, 90);
     tft.print("(c) 2022-23 Remo Riglioni");
     moveCursorTo(85, 120);
     tft.print(version_number);
     info = false;
     exitTouchedButton(exit_label);
    } 
  }
}


/*
  
 Editor
 returns the string (statement) containing the program line or command
 
*/

void lineEditor(char *statement, char *title, char *message, uint16_t text_color, uint16_t board_color, uint16_t key_color) {
  static short int t  = 0, px = 1, py = 40;
  short int button = KEY_NULL;
  clearScreen(BACKGROUND);                  // clear screen //
  printStringAt(0, 0, title, text_color);   // print title //
  printStringAt(0, 16,message, text_color); // print ready message //
  printStringAt(0, 40,(char *)">", text_color);  // text line indicator //
  drawKeyBoard(board_color,key_color);
  while (button != KEY_RETURN) {
    button = getTouchedButton(); // return key index //
    if (touch && !insert) {
      clearCursor(px * CHAR_W, py);
      if (button  >= 0  && t < MAXSTRING - 1) {
        statement[t++] = keylist[button];
        if (px < 25) {
          px++;
        } else {
          px = 0;
          py += 16;
        }
      }
      if (button == KEY_SPACE && t < MAXSTRING - 1) {
        statement[t++] = ' ';
        if (px < 25) {
          px++;
        } else {
          px = 0;
          py += 16;
        }
      }
      if (button == KEY_DEL  && t > 0) {
        t--;
        if (px > 0) {
          px--;
        } else {
          px  = 25;
          py -= 16;
        }
      }
      statement[t] = '\0';
      printStringAt(CHAR_W, 40, statement, text_color);
      insert = true; 
      touch  = false;
    }
    printBlinkCursor(px * CHAR_W, py, text_color, 500);
  }
  printStringAt(CHAR_W, 40, statement, BACKGROUND);
  clearCursor(px * CHAR_W, py);
  insert = true; 
  touch  = false;
  t = 0; px = 1; py = 40;
}




/* 
 
 return the line number label of program line

*/                        


int getStatementLineNumber(char *statement) {
  char *pstr     = statement;
  int lineNumber = atoi(statement);
  stringTrim(pstr);
  while (isdigit(*pstr)) pstr++;
  strcpy(statement, pstr);
  stringTrim(statement); 
  return lineNumber;
}




/* 
 
  insert a new program line

*/      

ERR insert_program_line(int line_number, char *instruction) {
  struct program_struct **pl = &start_program_pointer;
  struct program_struct  *s, *r, *t;
  int size_str = strlen(instruction) + 1;
  // create the linked list //
  if (*pl == NULL) {
    *pl = (struct program_struct *)malloc(sizeof(struct program_struct));
    if (!(*pl)) return OUT_OF_MEMORY;
    (*pl)->line_number   = line_number;
    (*pl)->instruction = (char *)malloc(size_str * sizeof(char));
    if (!((*pl)->instruction)) return OUT_OF_MEMORY;
    strcpy((*pl)->instruction, instruction);
    (*pl)->next = NULL;
    return NO_ERROR;
  }
  // insert as first //
  if ((*pl)->line_number > line_number) {
    s = *pl;
    *pl = (struct program_struct *)malloc(sizeof(struct program_struct));
    if (!(*pl)) return OUT_OF_MEMORY;
    (*pl)->line_number = line_number;
    (*pl)->instruction = (char *)malloc(size_str * sizeof(char));
    if (!((*pl)->instruction)) return OUT_OF_MEMORY;
    strcpy((*pl)->instruction, instruction);
    (*pl)->next = s;
    return NO_ERROR;
  }
  s = *pl;
  // insert in the middle//
  while (s->next != NULL) {
    if (s->next->line_number > line_number) {
      r = s->next;
      s->next = (struct program_struct *)malloc(sizeof(struct program_struct));
      if (!(s->next)) return OUT_OF_MEMORY;
      s->next->line_number = line_number;
      s->next->instruction = (char *)malloc(size_str * sizeof(char));
      if (!(s->next->instruction)) return OUT_OF_MEMORY;
      strcpy(s->next->instruction, instruction);
      s->next->next = r;
      return NO_ERROR;
    }
    s = s->next;
  }
  //appends to the end//
  s->next = (struct program_struct *)malloc(sizeof(struct program_struct));
  if (!(s->next)) return OUT_OF_MEMORY;
  s->next->line_number = line_number;
  s->next->instruction = (char *)malloc(size_str * sizeof(char));
  if (!(s->next->instruction)) return OUT_OF_MEMORY;
  strcpy(s->next->instruction, instruction);
  s->next->next = NULL;
  return NO_ERROR;
}




/* 
 
 print the program list 

*/      

void printProgramList() {
  struct program_struct  *temp = start_program_pointer;
  int line_number = -1;
  clearScreen(BACKGROUND);
  setFontSizeAndColor(2,TEXTCOLOR);
  moveCursorTo(0, 0);
  while (temp != NULL) {
    if (line_number != temp->line_number) {
     if (line_number!=-1) tft.println();
     tft.print(temp->line_number);
     tft.print(" ");
     tft.print(temp->instruction);
     line_number = temp->line_number;
    } else {
     tft.print(":");
     tft.print(temp->instruction);
    }
    if (tft.getCursorY() > 168) {
     exitTouchedButton(exit_label);
     clearScreen(BACKGROUND);
     moveCursorTo(0, 0);
    }
    temp = temp->next;
  }
  tft.println();
  exitTouchedButton(exit_label);
}



/* 
 
free the allocated programa memory (linked list)

*/      

void deleteProgramList() {
  struct program_struct *current = start_program_pointer;
  struct program_struct *next    = NULL;
  while (current != NULL) {
    next = current->next;
    free(current->instruction);
    free(current);
    current = next;
  }
  start_program_pointer = NULL;
}

/* 
 
get the identifier string 

*/      

ERR getIdentifier(char *identifier, char *statement) {
  char *pstr = statement;
  short int c=0; 
  if (isalpha(*pstr)) {
    while (isalnum(*pstr)) {
      *identifier++ = *pstr++;
       if ((c++)>MINSTRING-1) return IDENTIF_ERR;
    }
    *identifier = '\0';
    while (*pstr == ' ') pstr++;  // skip white space //
    strcpy(statement, pstr);
  } else
    return SYNTAX_ERROR;
  return NO_ERROR;
}



/*
 
gets a mathematical expression enclosed in parentheses ex.:(x-1),(2*x+3*(y-1)) ecc...
 
*/

ERR getParenthesisExpression(char *expr_index, char *statement) {
  uint16_t c = 0, pcount = 0;
  char *pstr = statement;
  do {
    *expr_index++ = *pstr;
    if ((c++)>MINEXPRES-1) return IDENTIF_ERR;
    if (*pstr == '(') pcount++;
    if (*pstr == ')') pcount--;
    pstr++;
  } while (*pstr != '\0' && pcount);
  if (pcount) return PARENTHESIS;
  *expr_index = '\0';
  while (*pstr == ' ') pstr++;  // skip white space //
  strcpy(statement, pstr);
  return NO_ERROR;
}


/*

checks assignment expression

*/

bool isAssignment(char *expr) {
  char *pstr = expr;
  while (*pstr == ' ') pstr++;
  if (*pstr == '=') {
    pstr++;
    while (*pstr == ' ') pstr++;  // skip white space //
    strcpy(expr, pstr);
    return true;
  }
  return false;
}




/*

split the arguments list of an instruction

*/

ERR parseInstructionArgs(char *arg, char *arg_list) {
  ERR err = NO_ERROR;
  bool string = false;
  char *pstr = arg_list;
  if (*pstr == '\0')
    return SYNTAX_ERROR;
  while (*pstr != '\0') {
    if (*pstr == ',' && !string) {
      pstr++;
      strcpy(arg_list, pstr);
      *arg = '\0';
      return NO_ERROR;
    }
    if (*pstr == '"') {
      if (!string)
        string = true;
      else
        string = false;
    }
    *arg++  = *pstr++;
  }
  *arg = '\0';
  strcpy(arg_list, pstr);
  return NO_ERROR;
}


/*

to convert an instruction argument to a numeric value

*/

ERR getArgValue(uint16_t *argv, char *arg_list) {
  char expression[MINSTRING];
  ERR err = NO_ERROR;
  err = parseInstructionArgs(expression, arg_list);
  *argv = (uint16_t)evalMathExpression(&err, expression);
  if (err) return err;
  return NO_ERROR;
}


/*

Nanobasic instructions 

*/


////////////////////////////////////////////////
//          execute DELAY instruction         //
//         DELAY ms   (milliseconds)          //
////////////////////////////////////////////////
ERR execDELAY(char *arg) {
  ERR err = NO_ERROR;
  uint16_t  timed;
  err = getArgValue(&timed,arg);
  if (err) return err;
  delay(timed);
  return NO_ERROR;  
}

////////////////////////////////////////////////
//       execute PINMODE instruction          //
//             PINMODE pin, value             //
// pin   = analog pin                         //
// value = 0,1,2                              //
// (0=INPUT,1=OUTPUT,2=INPUT_PULLUP)          //
////////////////////////////////////////////////

ERR execPINMODE(char *arg) {
  ERR err = NO_ERROR;
  uint16_t  pin, value;
  err = getArgValue(&pin, arg);
  if (err) return err;
  err = getArgValue(&value, arg);
  if (err) return err;
  switch(value) {
    case 0:
     pinMode(pin,INPUT);
     break;
    case 1:
     pinMode(pin,OUTPUT);
     break;
    case 2:
     pinMode(pin,INPUT_PULLUP);
     break;
    default:
     return OUT_OF_RANGE;
  }
  return NO_ERROR;
}


////////////////////////////////////////////////
//       execute AWRITE instruction           //
//           AWRITE pin, value                //
// pin   = analog pin                         //
// value = 0..255                             //
////////////////////////////////////////////////
ERR execAWRITE(char *arg) {
  ERR err = NO_ERROR;
  uint16_t  pin, value;
  err = getArgValue(&pin, arg);
  if (err) return err;
  if ( pin<0 && pin>7) 
   return OUT_OF_RANGE;
  err = getArgValue(&value, arg);
  if (err) return err;
  if (value < 0 || value >255)
   return OUT_OF_RANGE;
  else
   analogWrite(pin,value);
  return NO_ERROR;  
}


////////////////////////////////////////////////
//       execute DWRITE instruction           //
//           DWRITE pin, value                //
//pin   = digital pin
//value = 0,1
////////////////////////////////////////////////

ERR execDWRITE(char *arg) {
  ERR err = NO_ERROR;
  uint16_t  pin, value;
  err = getArgValue(&pin, arg);
  if (err) return err;
  err = getArgValue(&value, arg);
  if (err) return err;
  if (!value)
   digitalWrite(pin,LOW);
  else
   digitalWrite(pin,HIGH);
  return NO_ERROR;
}


////////////////////////////////////////////////
//        execute TONE instruction            //
//      TONE pin, frequency, duration         //
////////////////////////////////////////////////

ERR execTONE(char *arg) {
  ERR err = NO_ERROR;
  uint16_t  pin,f,d ;
  err = getArgValue(&pin,arg);
  if (err) return err;
  err = getArgValue(&f,arg);
  if (err) return err;
  if (*arg != '\0'){
   err = getArgValue(&d,arg);
   if (err) return err;
   tone(pin,f,d);
  } else
   tone(pin,f);
  return NO_ERROR;
}

////////////////////////////////////////////////
//       execute no TONE instruction          //
//                NOTONE pin                  //
////////////////////////////////////////////////
ERR execNOTONE(char *arg) {
  ERR err = NO_ERROR;
  uint16_t  pin ;
  err = getArgValue(&pin,arg);
  if (err) return err;
  noTone(pin);
  return NO_ERROR;
}


////////////////////////////////////////////////
//           execute CLS instruction          //
//                                            //
////////////////////////////////////////////////
ERR execCLS(char *arg) {
  ERR err = NO_ERROR;
  uint16_t color = (uint16_t) evalMathExpression(&err, arg);
  if (err) return err;
  clearScreen(color);
  return NO_ERROR;
}


////////////////////////////////////////////////
//         execute PIXEL instruction          //
//                                            //
////////////////////////////////////////////////
ERR execPIXEL(char *arg_list) {
  ERR err = NO_ERROR;
  uint16_t  x, y, c;
  err = getArgValue(&x, arg_list);
  if (err) return err;
  err = getArgValue(&y, arg_list);
  if (err) return err;
  err = getArgValue(&c, arg_list);
  if (err) return err;
  tft.drawPixel(x, y, c);
  return NO_ERROR;
}


////////////////////////////////////////////////
//         execute LINE instruction           //
//                                            //
////////////////////////////////////////////////

ERR execLINE(char *arg_list) {
  ERR err = NO_ERROR;
  uint16_t  x0, y0, x1, y1, c;
  err = getArgValue(&x0, arg_list);
  if (err) return err;
  err = getArgValue(&y0, arg_list);
  if (err) return err;
  err = getArgValue(&x1, arg_list);
  if (err) return err;
  err = getArgValue(&y1, arg_list);
  if (err) return err;
  err = getArgValue(&c, arg_list);
  if (err) return err;
  if (x0 == x1)
   tft.drawFastVLine(x0,y0,y1-y0,c);
  else
  if (y0 == y1) 
   tft.drawFastHLine(x0,y0,x1-x0,c);
  else
   tft.drawLine(x0,y0,x1,y1,c);
  return NO_ERROR;
}



////////////////////////////////////////////////
//          execute RECT instruction          //
//                                            //
////////////////////////////////////////////////

ERR execRECT(char *arg_list) {
  ERR err = NO_ERROR;
  uint16_t  x0, y0, w, h, c, r, f;
  err = getArgValue(&x0, arg_list);
  if (err) return err;
  err = getArgValue(&y0, arg_list);
  if (err) return err;
  err = getArgValue(&w, arg_list);
  if (err) return err;
  err = getArgValue(&h, arg_list);
  if (err) return err;
  err = getArgValue(&c, arg_list);
  if (err) return err;
  err = getArgValue(&r, arg_list);
  if (err) return err;
  err = getArgValue(&f, arg_list);
  if (err) return err;
  if (!r) {
   if (f)
    tft.fillRect(x0, y0, w, h, c);
   else
    tft.drawRect(x0, y0, w, h, c);
  } else {
   if (f)
    tft.fillRoundRect(x0, y0, w, h, r, c);
   else
    tft.drawRoundRect(x0, y0, w, h, r, c); 
  } 
  return NO_ERROR;
}



////////////////////////////////////////////////
//           execute CIRCLE instruction       //
//                                            //
////////////////////////////////////////////////

ERR execCIRCLE(char *arg_list) {
  ERR err = NO_ERROR;
  uint16_t  x, y, r, c, f;
  err = getArgValue(&x, arg_list);
  if (err) return err;
  err = getArgValue(&y, arg_list);
  if (err) return err;
  err = getArgValue(&r, arg_list);
  if (err) return err;
  err = getArgValue(&c, arg_list);
  if (err) return err;
  err = getArgValue(&f, arg_list);
  if (err) return err;
  if (f)
    tft.fillCircle(x, y, r, c);
  else
    tft.drawCircle(x, y, r, c);
  return NO_ERROR;
}


////////////////////////////////////////////////
//           execute REM instruction              //
//                                            //
////////////////////////////////////////////////
ERR execREM(struct program_struct **ptr_to_next_statement) {
  struct program_struct *program_ptr = *ptr_to_next_statement;
  int line_number = program_ptr->line_number;
  while (program_ptr != NULL) {
    if (line_number != program_ptr->line_number) {
      *ptr_to_next_statement = program_ptr;
      return NO_ERROR;
    }
    program_ptr = program_ptr->next ;
  }
  return NO_ERROR;
}


////////////////////////////////////////////////
// parse the arguments list of DIM instruction    //
//   return "true" at end of the process      //
////////////////////////////////////////////////
bool parseDimArgument(char *array_list, char *var_name, int *dim_size, ERR *err) {
  static uint16_t i = 0;
  uint16_t j = 0;
  // get the array name  //
  if (isalpha(array_list[i])) {
    while (isalnum(array_list[i])) {
      if (j < MINSTRING - 1)
        var_name[j++] = array_list[i++];
      else {
        i = 0;
        *err = OUT_OF_MEMORY;
        return true;
      }
    }
    var_name[j] = '\0'; j = 0;
  } else {
    i = 0;
    *err = SYNTAX_ERROR;
    return true;
  }
  if (isReservedIdentifier(var_name)) {
    i = 0;
    *err = RESERVED;
    return true;
  }
  while (array_list[i] == ' ') i++;
  // get the expression size //
  if (array_list[i] == '(') {
    char dim_size_expr[MINSTRING] = "";
    while (isdigit(array_list[++i]))
      dim_size_expr[j++] = array_list[i];
    if (array_list[i] != ')') {
      i = 0;
      *err = SYNTAX_ERROR;
      return true;
    }
    i++;
    dim_size_expr[j] = '\0';
    *dim_size = atoi(dim_size_expr);
  } else {
    i = 0;
    *err = SYNTAX_ERROR;
    return true;
  }
  while (array_list[i] == ' ') i++;
  if (array_list[i] == ',') {
    i++;
    return false;
  }
  if (array_list[i] == '\0') {
    i = 0;
    return true;
  }
  i = 0;
  *err = SYNTAX_ERROR;
  return true;
}



////////////////////////////////////////////////
//           execute DIM instruction              //
//                                            //
////////////////////////////////////////////////
ERR execDIM(char *array_list) {
  struct variables_struct *s = varlist_ptr;
  char var_name[MINSTRING] = "";
  bool exit_loop = false;
  ERR err = NO_ERROR;
  int dim_size = 0;
  while (!exit_loop) {
    exit_loop = parseDimArgument(array_list, var_name, &dim_size, &err);
    if (err) return err;
    // search into variable list //
    while (s != NULL) {
      if (!strcmp(s->var_name, var_name))
        return ALREADY_DEFINED;
      s = s->next;
    }
    if (dim_size > 1) {
      // allocate new array //
      err = insertOrUpdateVariable(var_name, 0.0, dim_size);
      if (err) return err;
    } else return ARRAY_DIM;
  }
  return NO_ERROR;
}



////////////////////////////////////////////////
//           execute PRINTAT instruction            //
//                                            //
////////////////////////////////////////////////
ERR execPRINTAT(char *arg_list) {
 uint16_t  x, y, c, s;
 ERR err = NO_ERROR;
 err = getArgValue(&x, arg_list); // x position
 if (err) return err;
 err = getArgValue(&y, arg_list); // y position
 if (err) return err;
 err = getArgValue(&c, arg_list); // color
 if (err) return err;
 err = getArgValue(&s, arg_list); // text size
 if (err) return err;
 moveCursorTo(x,y);
 setFontSizeAndColor(s,c);
 err = execPRINT(arg_list,true);
 if (err) return err;
 setFontSizeAndColor(2,TEXTCOLOR);
 return NO_ERROR;
}

////////////////////////////////////////////////
//           execute PRINT instruction            //
//                                            //
////////////////////////////////////////////////
ERR execPRINT(char *arg_list, bool printat) {
  ERR err = NO_ERROR;
  double value = 0.0;
  if (!printat) {
   if (tft.getCursorY() > 204) {
    exitTouchedButton(exit_label);
    clearScreen(BACKGROUND);
    moveCursorTo(0, 0);
   }
  }
  while (arg_list[0] != '\0') {
    char arg[MAXEXPRES] = "";
    err = parseInstructionArgs(arg, arg_list);
    if (err) return err;
    if (arg[0] != '"') {
      value = evalMathExpression(&err, arg);
      if (err) return err;
      if (value-trunc(value)==0.0) 
       tft.print((long int)value);
      else 
       tft.print(value);
    } else {
      // remove double apex '"' //
      if (arg[strlen(arg) - 1] == '"')
        arg[strlen(arg) - 1] = '\0';
      else
        return SYNTAX_ERROR;
      tft.print(&arg[1]);
    }
  }
  if (!printat){
   tft.println();
  }
  return NO_ERROR;
}



////////////////////////////////////////////////
//           execute INPUT instruction            //
//                                            //
////////////////////////////////////////////////

ERR execINPUT(char *arg_list)  {
 ERR err = NO_ERROR;
 double value = 0.0;
 char title[]="Input value";
 char val[MINSTRING] = "";
 char buf[MINSTRING] = "";
 char msg[MAXEXPRES] = "";
 
 while (arg_list[0] != '\0') {
  char arg[MAXEXPRES] = "";
  err = parseInstructionArgs(arg, arg_list);
  if (err) return err;
  if (arg_list[0] == '\0') { 
   strcpy(arg_list,arg); // the last argument is the variable where to save the input value
   break;
  }
  if (arg[0] != '"') {
   value = evalMathExpression(&err,arg);
   if (err) return err;
   if (value-trunc(value)==0.0) 
    sprintf(buf,"%d",(long int)value);
   else 
    sprintf(buf,"%f",value);
   } else {
    // remove double apex '"' //
    if (arg[strlen(arg) - 1] == '"')
        arg[strlen(arg) - 1] = '\0';
      else
        return SYNTAX_ERROR;
      sprintf(buf,"%s",&arg[1]);
    }
   strcat(msg,buf); 
  }
  lineEditor(val,title,msg,TEXTCOLOR,KEYBOARD2,TEXTCOLOR);
  err = getIdentifier(buf,arg_list);
  if (err) return err;
  // check if is an array element //
  if (arg_list[0] == '(') { 
   int array_index = (int)evalMathExpression(&err, arg_list); 
   err=updateArrayValues(buf,atof(val),array_index);
   if (err) return err;
  } else {  
   err=insertOrUpdateVariable(buf,atof(val),1); 
   if (err) return err; 
  }  
  //clear and reset tft display//
  clearScreen(BACKGROUND);
  setFontSizeAndColor(2,TEXTCOLOR);
  moveCursorTo(0,0);
  return NO_ERROR;
}

////////////////////////////////////////////////
//           execute IF instruction               //
//                                            //
////////////////////////////////////////////////
ERR execIF(char *arg, struct program_struct **ptr_to_next_statement) {
  int condition, then_index;
  ERR err = NO_ERROR;
  then_index = getSubStringIndex((char *)" THEN ", arg);
  if (then_index == -1) return THEN_ERROR;
  arg[then_index] = '\0';
  condition = (int) evalMathExpression(&err, arg);
  if (err) return err;
  if (condition) // IF condition is true , execute instruction after THEN//
    return execInstruction(stringTrim(&arg[then_index + 5]), ptr_to_next_statement);
  return NO_ERROR;
}

////////////////////////////////////////////////
//          execute RETURN instruction            //
//                                            //
////////////////////////////////////////////////
ERR execRETURN(char *arg, struct program_struct **ptr_to_next_statement) {
  if (arg[0] != '\0') return SYNTAX_ERROR;
  if (gsb_index)
    *ptr_to_next_statement = gosub_stack[--gsb_index];
  else
    return GOSUB_ERROR;
  return NO_ERROR;
}

////////////////////////////////////////////////
//           execute GOSUB instruction            //
//                                            //
////////////////////////////////////////////////
ERR execGOSUB(char *arg, struct program_struct **ptr_to_next_statement) {
  ERR err = NO_ERROR;
  struct program_struct *program_ptr = start_program_pointer;
  if (gsb_index < MAXSUB) // check the maximum number of nested subroutines 
   gosub_stack[gsb_index++] = (*ptr_to_next_statement)->next;
  else
   return OUT_OF_MEMORY;
  int line_number = (int) evalMathExpression(&err, arg);
  if (err) return err;
  while (program_ptr != NULL) {
    if (line_number == program_ptr->line_number) {
      *ptr_to_next_statement = program_ptr;
      return NO_ERROR;
    }
    program_ptr = program_ptr->next ;
  }
  return WRONG_LABEL;
}

////////////////////////////////////////////////
//           execute GOTO instruction             //
//                                            //
////////////////////////////////////////////////
ERR execGOTO(char *arg, struct program_struct **ptr_to_next_statement) {
  ERR err = NO_ERROR;
  struct program_struct *program_ptr = start_program_pointer;
  int line_number = (int) evalMathExpression(&err, arg);
  if (err) return err;
  while (program_ptr != NULL) {
    if (line_number == program_ptr->line_number) {
      *ptr_to_next_statement = program_ptr;
      return NO_ERROR;
    }
    program_ptr = program_ptr->next ;
  }
  return WRONG_LABEL;
}

////////////////////////////////////////////////
//           execute END instruction              //
//                                            //
////////////////////////////////////////////////
ERR execEND(struct program_struct **ptr_to_next_statement) {
  *ptr_to_next_statement = NULL;
  return NO_ERROR;
}

////////////////////////////////////////////////
//execute FOR instruction:  FOR X=1 TO 10 STEP 2  //
//        FOR A(3)=5 TO 15 STEP 5             //
////////////////////////////////////////////////
ERR execFOR(char *arg, struct program_struct **ptr_to_next_statement) {
  int i = 0, var_index = 0, array_index = 0;
  char loop_var_name[MINSTRING] = "";
  double for_start_value = 0.0;
  double for_stop_value  = 0.0;
  double for_step_value  = 0.0;
  char expr[MINEXPRES]   =  "";
  ERR err = NO_ERROR;
  bool isArray = false;

  i = getSubStringIndex((char *)" TO ", arg);
  if (i == -1) return FOR_ERROR;
  arg[i] = '\0';

  strcpy(expr, arg);        // expr -> "X=1" or "A(3)=5"
  strcpy(arg, &arg[i + 4]); // arg ->  "10 STEP 2" or "15 STEP 5"

  err=getIdentifier(loop_var_name, expr); // loop_var_name -> "X","A"   expr -> "=1","(3)=5"
  if (err) return err;

  //  check if is an array  //
  if (expr[0] == '(') {
    char array_index_expr[MINEXPRES] = "";
    err = getParenthesisExpression(array_index_expr, expr); // array_index_expr ->"(3)", expr -> "=5"
    if (err) return err;
    array_index = (int) evalMathExpression(&err, array_index_expr); // array_index = 3
    if (err) return err;
    isArray = true;
  } else
    isArray = false;

  if (isAssignment(expr)) {
    for_start_value = evalMathExpression(&err, expr);
    if (err) return err;
    if (isArray)
      err = updateArrayValues(loop_var_name, for_start_value, array_index);
    else
      err = insertOrUpdateVariable(loop_var_name, for_start_value, 1);
    if (err) return err;
  } else
    return SYNTAX_ERROR;

  i = getSubStringIndex((char *)" STEP ",arg);
  if (i == -1) {
    for_stop_value = evalMathExpression(&err, arg);
    if (err) return err;
    for_step_value = 1;
  } else {
    arg[i] = '\0';
    for_stop_value = evalMathExpression(&err, arg);
    if (err) return err;
    strcpy(arg, &arg[i + 5]);
    for_step_value = evalMathExpression(&err, arg);
    if (err) return err;
  }

  for_loop[for_index].program_ptr   = *ptr_to_next_statement;
  for_loop[for_index].var_ptr       = getVarPointerFromName(loop_var_name);
  for_loop[for_index].array_index   = array_index;
  for_loop[for_index].start_value   = for_start_value;
  for_loop[for_index].stop_value    = for_stop_value;
  for_loop[for_index].step_value    = for_step_value;

  if ((for_step_value > 0 && for_start_value >= for_stop_value) ||
      (for_step_value < 0 && for_start_value <= for_stop_value)) {
    struct program_struct  *temp = (*ptr_to_next_statement)->next;
    // search the first NEXT and exit //
    while (temp != NULL) {
      if (!strcmp(temp->instruction,"NEXT")) {
         *ptr_to_next_statement = temp->next;
        temp = NULL;
      }
      temp = temp->next;
    }
  }
  if (for_index<MAXFORLOOP) 
   for_index++;
  else return OUT_OF_MEMORY; 
  return NO_ERROR;
}

////////////////////////////////////////////////
//          execute NEXT instruction              //
//                                            //
////////////////////////////////////////////////
ERR execNEXT(struct program_struct **ptr_to_next_statement) {
  double for_start_value = 0.0, for_stop_value = 0.0, for_step_value = 0.0;
  if (!for_index) return FOR_ERROR;
  for_start_value = for_loop[for_index - 1].start_value;
  for_stop_value  = for_loop[for_index - 1].stop_value;
  for_step_value  = for_loop[for_index - 1].step_value;
  for_start_value += for_step_value;
  if ((for_step_value < 0 && for_start_value >= for_stop_value) || (for_step_value > 0 && for_start_value <= for_stop_value)) {
    *ptr_to_next_statement = for_loop[for_index - 1].program_ptr->next;
    for_loop[for_index - 1].start_value = for_start_value;
    for_loop[for_index - 1].var_ptr->var_value[for_loop[for_index - 1].array_index] = for_start_value;
  } else
    for_index--;
  return NO_ERROR;
}


ERR execOPEN(char *arg_list)  {
  char fileName[MINSTRING] = "";
  ERR err = NO_ERROR;
  int mode;
  if (*arg_list != '\0') {
   err = parseInstructionArgs(fileName, arg_list);
   if (err) return err;
   if (*fileName == '"') {
    if (fileName[strlen(fileName) - 1] == '"')
     fileName[strlen(fileName) - 1] = '\0';
    else
     return SYNTAX_ERROR;
   } else
   return SYNTAX_ERROR;
  } else 
  return SYNTAX_ERROR;
  mode = (int) evalMathExpression(&err,arg_list);
  if (err) return err;
  switch(mode) {
   case 1: 
    if (!SD_File.open(&fileName[1],O_WRITE | O_CREAT| O_TRUNC)) 
     return SD_FILE_ERR;
    break;
   case 2:
    if (!SD_File.open(&fileName[1],O_WRITE |O_CREAT |O_AT_END))
     return SD_FILE_ERR;     
    break;
   case 3:
    if (!SD_File.open(&fileName[1],O_READ))
     return SD_FILE_ERR;
    break; 
  }
  return NO_ERROR;
}


ERR execCLOSE(char *arg_list)  {
 if (*arg_list == '\0') {
  SD_File.close();
 } else 
  return SYNTAX_ERROR;
 return NO_ERROR;  
}


ERR execFWRITE(char *arg_list)  {
  ERR err = NO_ERROR;
  double value;
  value = evalMathExpression(&err, arg_list);
  if (err) return err;   
  SD_File.println(value);
  return NO_ERROR;
}

////////////////////////////////////////////////
//        execute instruction or assigns          //
//   a value to a variable or array element   //
////////////////////////////////////////////////
ERR execInstruction(char *instruction, struct program_struct **ptr_to_next_statement) {
  char identifier[MINSTRING] = "", args[MINEXPRES] = "";
  int i = 0, j = 0, array_index = 0, var_index = 0;
  PRGINS code = NOINS;
  bool isArray = false;
  double value = 0.0;
  ERR err = NO_ERROR;
  err = getIdentifier(identifier, instruction);
  if (err) return err;
  code = getInstructionCode(identifier);
  /*
           execute instructions            
  */
  switch (code) {
    case _DIM:
      return execDIM(instruction);
    case _INPUT: 
      return execINPUT(instruction);
    case _PRINT: 
      return execPRINT(instruction,false);
    case _PRINTAT:
      return execPRINTAT(instruction);  
    case _FOR:
      return execFOR(instruction, ptr_to_next_statement);
    case _NEXT:
      return execNEXT(ptr_to_next_statement);
    case _IF:
      return execIF(instruction, ptr_to_next_statement);
    case _GOTO:
      return execGOTO(instruction, ptr_to_next_statement);
    case _GOSUB:
      return execGOSUB(instruction, ptr_to_next_statement);
    case _RETURN:
      return execRETURN(instruction, ptr_to_next_statement);
    case _REM:
      return execREM(ptr_to_next_statement);
    case _END:     
      return execEND(ptr_to_next_statement);
    case _CLS:
      return execCLS(instruction);
    case _OPEN:
      return execOPEN(instruction);
    case _CLOSE:
      return execCLOSE(instruction);
    case _PIXEL:
      return execPIXEL(instruction);
    case _LINE:
      return execLINE(instruction);
    case _RECT:
      return execRECT(instruction);  
    case _CIRCLE:
      return execCIRCLE(instruction);        
    case _PINMODE:
      return execPINMODE(instruction);  
    case _AWRITE:   
      return execAWRITE(instruction);
    case _DWRITE: 
      return execDWRITE(instruction);
    case _FWRITE:
      return execFWRITE(instruction);   
    case _DELAY:
      return execDELAY(instruction);
    case _TONE: 
      return execTONE(instruction); 
    case _NOTONE:
      return execNOTONE(instruction);   
  }
  // if it is not a instruction , it should be a variable //
  //           first check if is an array             //
  if (*instruction == '(') {
    err = getParenthesisExpression(args, instruction);
    if (err) return err;
    array_index = (int) evalMathExpression(&err, args);
    if (err) return err;
    isArray = true;
  } else
    isArray = false;

  // check if is an assignment  //
  if (isAssignment(instruction)) {
    value = evalMathExpression(&err, instruction);
    if (err) return err;
    if (isArray) {
      return updateArrayValues(identifier, value, array_index);
    } else {
      return insertOrUpdateVariable(identifier, value, 1);
    }
  }
  return UNKNOW_INSTRUCTION;
}

////////////////////////////////////////////////
// split the statement into single instructions.  //
// returns "true" at the end of               //
// splitting process                          //
////////////////////////////////////////////////
bool splitStatement(char *statement, char *instruction) {
  char *pstr = statement;
  bool string = false;
  while (*pstr != '\0') {
    if (*pstr == '"') {
      if (!string)
        string = true;
      else
        string = false;
    }
    if (*pstr == ':' && !string) {
      *instruction = '\0';
      pstr++;
      strcpy(statement, pstr);
      return false;
    }
    if (string) {
      *instruction++  = *pstr++;
    } else {
      *instruction++  = toupper(*pstr++);
    }
  }
  strcpy(statement, pstr);
  *instruction = '\0';
  return true;
}


/*

 BASIC commands functions

*/

////////////////////////////////////////////////
//         execute CLEAR  command              //
//                                            //
////////////////////////////////////////////////
ERR execCLEAR() {
  deleteVariableList();
  deleteProgramList();
  return NO_ERROR;
}
////////////////////////////////////////////////
//         execute RESET  command              //
//                                            //
////////////////////////////////////////////////
void execRESET() {
  wdt_enable(WDTO_30MS); 
  while(1){} 
}
////////////////////////////////////////////////
//          execute LIST command              //
//                                            //
////////////////////////////////////////////////
ERR execLIST() {
  printProgramList();
  return NO_ERROR;
}


////////////////////////////////////////////////
//          execute RUN command               //
//                                            //
////////////////////////////////////////////////
ERR execRUN(int *exit_at_line) {
  struct program_struct *last_program_ptr = NULL;
  struct program_struct *program_ptr = NULL;
  char instruction[MAXSTRING];
  ERR err = NO_ERROR;
  
  //////////////////////////////////////////
  // clear variable table  and loop index //
  //////////////////////////////////////////
  for_index = 0; // reset for index counter //
  gsb_index = 0; // reset gosub index counter //
  deleteVariableList(); // delete variables list //
  // clear tft screen, set font color, size and // 
  // position to  print string and number //
  clearScreen(BACKGROUND);
  setFontSizeAndColor(2,TEXTCOLOR);
  moveCursorTo(0,0);
  // initialize random number //
  randomSeed(analogRead(0));
  // start from first statemet //
  program_ptr = start_program_pointer;
  // program loop //
  while (program_ptr != NULL) {
    // store the last program pointer
    last_program_ptr  = program_ptr;
    *exit_at_line = program_ptr->line_number;
    strcpy(instruction, program_ptr->instruction);    
    err = execInstruction(instruction, &program_ptr);
    if (err) return err;
    // if the program pointer has not change, it is advanced to point to next statement
    if (program_ptr == last_program_ptr) program_ptr = program_ptr->next ;
  }
  exitTouchedButton(exit_label);
  return err;
}




////////////////////////////////////////////////
//          execute LOAD command              //
//                                            //
////////////////////////////////////////////////
ERR execLOAD(char *statement, int *exit_at_line) {
  char program_line[MAXSTRING];
  ERR err = NO_ERROR;
  bool exit_loop = false;
  int i = 0, line_number = 0;
  // statement hold the "filename" //
  while (statement[i] != '"' && statement[i] != '\0') i++;
  if (statement[i] == '\0') return SYNTAX_ERROR;
  strcpy(statement, &statement[i + 1]);
  stringTrim(statement);
  if (statement[strlen(statement) - 1] == '"')
    statement[strlen(statement) - 1] = '\0';
  else
    return SYNTAX_ERROR;
  if (!SD_File.open(statement, O_READ)) {
   return SD_FILE_ERR;
  } else {
   int c = 0;i = 0; 
   deleteVariableList(); // clear memory list
   deleteProgramList();  // clear last program //
   clearScreen(BACKGROUND);
   setFontSizeAndColor(2,TEXTCOLOR);
   moveCursorTo(0, 0);
   tft.print(F("LOAD file : "));
   tft.println(statement);
   // read from the file until there's nothing else in it:
   while ((c = SD_File.read()) >= 0) {     
    if ((char)c != '\r') {
     if ((char)c != '\n')
      if (i < MAXSTRING-1)
       program_line[i++] = (char) c;
      else
       return OUT_OF_BOUNDS;  
    } else {
     program_line[i] = '\0';     
     exit_loop = false; 
     i = 0; 
     stringTrim(program_line);
     if (*program_line != '#' && *program_line != '\0') { //if the line start with '#' it is a remark  
      // try to get the line number of the statement //
      line_number = getStatementLineNumber(program_line);
      if (line_number <= 0) return WRONG_LABEL;
      while (!exit_loop) {
       char instruction[MAXSTRING] = "";
       // split the statement into single program instruction //
       exit_loop = splitStatement(program_line, instruction);
       // if it is not empty, it is inserted in the program list//
       if (*instruction != '\0') {
        err = insert_program_line(line_number, stringTrim(instruction));
        if (err) return err;
       }
      }
     }           
    }     
   }    
   // close the file:
   SD_File.close();
 }
 tft.println(F("file loaded...OK"));
 exitTouchedButton(exit_label);
 return NO_ERROR;
}




////////////////////////////////////////////////
//          execute SAVE command              //
//                                            //
////////////////////////////////////////////////
ERR execSAVE(char *statement) {
  struct program_struct  *temp = start_program_pointer;
  int line_number = -1, i = 0;
  // statement hold the "filename" //
  while (statement[i] != '"' && statement[i] != '\0') i++;
  if (statement[i] == '\0')
    return SYNTAX_ERROR;
  strcpy(statement, &statement[i + 1]);
  stringTrim(statement);
  if (statement[strlen(statement) - 1] == '"')
    statement[strlen(statement) - 1] = '\0';
  else
    return SYNTAX_ERROR;
  if (!SD_File.open(statement,O_WRITE|O_CREAT))
   return SD_FILE_ERR;
   while (temp != NULL) {
    if (line_number != temp->line_number) {
     if (line_number != -1 ) {
      SD_File.println("");
     }
     SD_File.print(temp->line_number);
     SD_File.print(" ");
     SD_File.print(temp->instruction);
     line_number = temp->line_number;
    } else {
     SD_File.print(":");
     SD_File.print(temp->instruction);
    }
   temp = temp->next;
  }
  SD_File.println();
  SD_File.close();
  clearScreen(BACKGROUND);
  setFontSizeAndColor(2,TEXTCOLOR);
  moveCursorTo(0, 0); 
  tft.println(statement);
  tft.println(F("file saved...OK"));
  exitTouchedButton(exit_label);
  return NO_ERROR;
}



////////////////////////////////////////////////
// delete a instruction from the program list     //
//                                            //
////////////////////////////////////////////////
bool deleteinstruction(int line_number) {
  //temp is used to freeing the memory
  struct program_struct **head = &start_program_pointer;
  struct program_struct *temp;
  if ((*head) == NULL) return false;
  if ((*head)->line_number == line_number) {
    temp = *head;    //backup to free the memory
    *head = (*head)->next;
    free(temp->instruction);
    free(temp);
    return true;
  } else {
    struct program_struct *current  = *head;
    while (current->next != NULL) {
      //if yes, we need to delete the current->next node
      if (current->next->line_number == line_number) {
        temp = current->next;
        //node will be disconnected from the linked list.
        current->next = current->next->next;
        free(temp->instruction);
        free(temp);
        return true;
      }
      //Otherwise, move the current node and proceed
      else
        current = current->next;
    }
  }
  return false;
}

////////////////////////////////////////////////
// delete all the instructions that have the same //
//               line number                  //
////////////////////////////////////////////////
void searchinstructionTodelete(int line_number) {
  while (deleteinstruction(line_number));
}



////////////////////////////////////////////////
// load the statement and store it in program //
// linked list. Execute the statement, if     //
// the line program is without number         //
// label.                                     //
////////////////////////////////////////////////

ERR loadOrExecute(char *statement, int *exit_at_line) {
  bool exit_loop = false;
  ERR err   =   NO_ERROR;
  int line_number  =  0;
  BASCMD  cmd   =  NOCMD;
  // check if the statement is a basic command  RUN,LIST,LOAD ...//
  cmd = getCommandCode(statement);
  // if is not a basic command //
  if (cmd == NOCMD) {
    // try to get the line number of the statement //
    line_number = getStatementLineNumber(statement);
    // a line number must be greater than zero !
    if (line_number < 0) return WRONG_LABEL; 
    // if a program line is inserted with a line number already present, the
    // previous program line is deleted                                     
    if (line_number && start_program_pointer != NULL ) 
     searchinstructionTodelete(line_number); 
    // splits the program line into the individual instructions to be executed //
    while (!exit_loop) {
      char instruction[MAXSTRING] = "";
      // split the statement into single program instruction //
      exit_loop = splitStatement(statement, instruction);
      // if it is not empty, it is inserted in the program list//
      if (*instruction != '\0') {
        *exit_at_line = line_number;
        err = insert_program_line(line_number, stringTrim(instruction));
        if (err) return err;
      }
    }
    // if the program line does not have a line number, it is executed immediately! //
    if (!line_number) {
      // end the program line with the "END" instruction //
      insert_program_line(0,(char *) "END");
      err = execRUN(exit_at_line);
      searchinstructionTodelete(0); // delete program line number 0 //
      if (err) return err;
      deleteVariableList(); // delete variables list //
    }
    return NO_ERROR;
  } else {
    if (cmd == _RUN) {
      // runs the stored program //
      err = execRUN(exit_at_line);
      if (err) return err;
    } else 
    if (cmd == _LIST) {
      // displays the stored program //
      err = execLIST();
      if (err) return err;
    } else 
    if (cmd == _LOAD) {
      // load a program from an external file
      err = execLOAD(statement, exit_at_line);
      if (err) return err;
    } else 
    if (cmd == _SAVE) {
      // save a program in an external file
      err = execSAVE(statement);
      if (err) return err;
    } else 
    if (cmd == _CLEAR) {
      // deletes the stored program //
      err = execCLEAR();
      if (err) return err;
    } else 
    if (cmd == _RESET) {
      // software board reset //
      execRESET();
    } 
  }
 return NO_ERROR; 
}




void logo() {
  tft.drawRect(5,5,315,230,WHITE);
  tft.fillRect(7,7,311,226,BLUE);
  tft.drawRect(60,20,200,100,PURPLE);
  tft.drawRect(65,25,190,90,GREEN);  
  moveCursorTo(80, 40);
  setFontSizeAndColor(3,PURPLE);
  tft.print(F("NanoBASIC"));
  moveCursorTo(90, 70);
  tft.setTextColor(GREEN);
  tft.print(F("ver 1.0"));
  moveCursorTo(60, 130);
  setFontSizeAndColor(1,GREEN);
  tft.println(F("for Arduino Nano Every"));
  tft.setTextSize(2);
}


void setup() {
  pinMode(TS_CS, OUTPUT);
  digitalWrite(TS_CS, HIGH);
  pinMode(TFT_CS, OUTPUT);
  digitalWrite(TFT_CS, HIGH);
  pinMode(SD_CS, OUTPUT);
  digitalWrite(SD_CS, HIGH);
  setupDisplay();
  setupTouch();
  logo();
  if (!sd.begin(SD_CS,SPI_HALF_SPEED)) {
   moveCursorTo(48,145);
   tft.setTextColor(RED);
   tft.println(F("SD card not found !"));
  }
  exitTouchedButton(exit_label);
}

void loop() {
 char statement[MAXSTRING] = ""; /* statement string hold the command to execute
                                    or program line to insert in program list */ 
 int exit_at_line = 0; /* return the program line number where stop the program execution */
 char title[MAXEXPRES]; 
 sprintf(title,"NanoBASIC 1.0 (%d bytes)",availableMemory());
 lineEditor(statement,title,(char *)"Ready !",TEXTCOLOR,KEYBOARD1,TEXTCOLOR);
 stringTrim(statement); // remove blank space //
 if (*statement != '\0') {
  ERR err = NO_ERROR;
  err = loadOrExecute(statement, &exit_at_line);
  if (err) {
    printErrorMessage((int)err, exit_at_line);
  }
 }
}

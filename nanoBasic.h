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

/* TFT display connection to Arduino board */

/*   TFT pin     |   Arduino board pin     */

#define SD_CS         6  // SD card CS
#define TS_CS         7  // Touch screen CS 
#define TFT_RST       8
#define TFT_DC        9
#define TFT_CS        10
#define TFT_MOSI      11
#define TFT_CLK       13
#define TFT_MISO      12


/*
some colors to use for text and graphics
 */

#define WHITE         0xFFFF
#define BLACK         0x0000
#define RED           0xF800
#define YELLOW        0xFFE0
#define BLUE          0x016A 
#define GREEN         0x47E0
#define PURPLE        0xF81E
#define ORANGE        0xFCA0
#define BROWN         0x9241
#define GRAY          0x4A49

#define BACKGROUND    BLACK
#define TEXTCOLOR     WHITE
#define KEYBOARD1     BROWN  
#define KEYBOARD2     GRAY   


#define MAXSTRING     128    // max string length 
#define MINSTRING     16     // min string length 
#define MAXEXPRES     48     // max expression length
#define MINEXPRES     16     // min expression length
#define MAXFORLOOP     5     // max nested for loop
#define MAXSUB         5     // max nested gosub/return

#define SCR_LEFT_GAP   4
#define CHAR_W        12


/* 
touch screen calibration values
 */
 
#define XCALM         -0.09
#define XCALC         337.12
#define YCALM         -0.07
#define YCALC         253.95

/* 
keyboard main button 
*/

#define KEY_DEL       -5
#define KEY_SPACE     -10
#define KEY_RETURN    -15
#define KEY_NULL      -1


/* 
touch screen, point coord. 
 */
int16_t tx,ty;        

bool    touch  =    false;
bool    insert =    false;
bool    info   =    true;
char    exit_label[]     = "->";
char    version_number[] = "ver 1.0.3.5";

/* 
SD and file global struct and variable 
 */

SdFat   sd;         // SD fat library
SdFile  SD_File;    // read and write files to SD
int8_t  eof=0;      // end of file check

/*
list BASIC commands                    
the numeric value is the position into 'bas_cmd_list' string 
*/
 
 typedef enum {
  NOCMD     = 0,
  _RUN      = 6,
  _LIST     = 10,
  _CLEAR    = 15,
  _LOAD     = 21,
  _SAVE     = 26,
  _RESET    = 31
} BASCMD;
const char bas_cmd_list[]= ",NOCMD,RUN,LIST,CLEAR,LOAD,SAVE,RESET,";

/*
list of programmable BASIC instructions 
the numeric value is the position into 'prg_cmd_list' string 
*/


typedef enum {
  NOINS     =0,
  _DIM      =6,
  _INPUT    =10,
  _PRINT    =16,
  _PRINTAT  =22,
  _FOR      =30,
  _NEXT     =34,
  _IF       =39,
  _GOTO     =42,
  _GOSUB    =47,
  _RETURN   =53,
  _REM      =60,
  _END      =64,
  _OPEN     =68,
  _CLOSE    =73,
  _CLS      =79,
  _PIXEL    =83,
  _LINE     =89,
  _RECT     =94,
  _CIRCLE   =99,
  _PINMODE  =106,
  _DWRITE   =114,
  _AWRITE   =121,
  _FWRITE   =128,
  _DELAY    =135,
  _TONE     =141,
  _NOTONE   =146 
} PRGINS;
const char prg_ins_list[]= ",NOINS,DIM,INPUT,PRINT,PRINTAT,FOR,NEXT,IF,GOTO,GOSUB,RETURN,REM,END,OPEN,CLOSE,CLS,PIXEL,LINE,RECT,CIRCLE,PINMODE,DWRITE,AWRITE,FWRITE,DELAY,TONE,NOTONE,";
                           

/*
list of math functions
the numeric value is the position into 'prg_fun_list' string
*/

typedef enum {
 NOFUN     =0,
 _SQRT     =6,
 _LOG      =11,
 _LN       =15,
 _EXP      =18,
 _ABS      =22,
 _SIN      =26,
 _COS      =30,
 _TAN      =34,
 _ASIN     =38,
 _ACOS     =43,
 _ATAN     =48,
 _RAND     =53,
 _MEM      =58,
 _AREAD    =62,
 _DREAD    =68,
 _GETX     =74,
 _GETY     =79,
 _MILLIS   =84,
 _FREAD    =91,
 _EOF      =97,
 _INT      =101,
} PRGFUN;

const char prg_fun_list[]=",NOFUN,SQRT,LOG,LN,EXP,ABS,SIN,COS,TAN,ASIN,ACOS,ATAN,RAND,MEM,AREAD,DREAD,GETX,GETY,MILLIS,FREAD,EOF,INT,";



/*
list of mathematical operators
*/

typedef enum {
  NOP       = 0,    // not operator
  ADD       = 2,    // +
  SUB       = 3,    // -
  MUL       = 4,    // *
  DIV       = 5,    // /
  POW       = 6,    // ^
  GT        = 7,    // >
  GTE       = 8,    // >=
  LT        = 9,    // <
  LTE       = 10,   // <=
  EQ        = 11,   // =
  NEQ       = 12,   // <>
  AND       = 13,   // & and
  OR        = 14,   // | or
  NOT       = 15    // ! not
} OPERATOR;



/*
errors list  
*/

typedef enum  {
  NO_ERROR        = 0,   // Ok, no error
  PARENTHESIS     = 1,   // parentheses error
  WRONG_CHAR      = 2,   // wrong char
  DIVIDE_BY_ZERO  = 3,   // divide by zero
  OUT_OF_MEMORY   = 4,   // out of memory
  SYNTAX_ERROR    = 5,   // syntax error
  UNKNOW_INSTRUCTION  = 6,   // unknown instruction
  GOSUB_ERROR     = 7,   // gosub error
  THEN_ERROR      = 8,   // if .. then error
  WRONG_LABEL     = 9,   // wrong label
  FUN_ERROR       = 10,  // math function error
  FOR_ERROR       = 11,  // for..next loop error
  OUT_OF_BOUNDS   = 12,  // array out of bounds error
  ARRAY_DIM       = 13,  // array dimensions error
  ARRAY_UNDEFINED = 14,  // array dimensions error
  ALREADY_DEFINED = 15,  // array already defined
  SD_FILE_ERR     = 16,  // file I/O error
  RESERVED        = 17,  // reserved identifier
  OUT_OF_RANGE    = 18,  // argument out of range
  IDENTIF_ERR     = 19   // expr or identifier too long
} ERR;

const char STR_NO_ERROR[] = "ok, ready!";
const char STR_PARENTHESIS[] = "parentheses error";
const char STR_WRONG_CHAR[]  = "wrong char";
const char STR_DIVISION_BY_ZERO[] = "division by zero";
const char STR_OUT_OF_MEMORY[] = "out of memory";
const char STR_SYNTAX_ERROR[]  = "syntax error";
const char STR_UNKNOW_INSTRUCTION[] = "unknown instruction";
const char STR_GOSUB_ERROR[] = "gosub error";
const char STR_THEN_ERROR[] = "if/then error";
const char STR_WRONG_LABEL[] = "wrong label";
const char STR_FUN_ERROR[] = "function error";
const char STR_FOR_ERROR[] = "for/next error";
const char STR_OUT_OF_BOUNDS[] = "out of bounds";
const char STR_ARRAY_DIM[] = "dimensions error";
const char STR_ARRAY_UNDEFINED[] = "undefined array";
const char STR_ALREADY_DEFINED[] = "already defined";
const char STR_SD_FILE_ERR[]  = "file I/O error";
const char STR_RESERVED[]  = "reserved identifier";
const char STR_OUT_OF_RANGE[] = "out of range";
const char STR_IDENTIF_ERR[] = "ID too long";


const char *const error_list[] = {
  STR_NO_ERROR, STR_PARENTHESIS, STR_WRONG_CHAR, STR_DIVISION_BY_ZERO,
  STR_OUT_OF_MEMORY, STR_SYNTAX_ERROR, STR_UNKNOW_INSTRUCTION,
  STR_GOSUB_ERROR, STR_THEN_ERROR, STR_WRONG_LABEL, STR_FUN_ERROR,
  STR_FOR_ERROR, STR_OUT_OF_BOUNDS, STR_ARRAY_DIM, STR_ARRAY_UNDEFINED,
  STR_ALREADY_DEFINED, STR_SD_FILE_ERR, STR_RESERVED,
  STR_OUT_OF_RANGE,STR_IDENTIF_ERR
};


/*
 keyboard editor
 */
const char keylist[] =  "1234567890.#()QWERTYUIOPASDFGHJKLZXCVBNM+-/*^<>=##&,|!$\":?##";



/*
general utility functions, display and GUI setting    
*/
 
int    availableMemory(); 
char   *stringTrim(char* s1);
int    getSubStringIndex(char *s1, char *s2);
bool   isReservedIdentifier(char *identifier);
PRGINS getInstructionCode(char *identifier);
PRGFUN getFunctionCode(char *identifier);
BASCMD getCommandCode(char *identifier);
void   printErrorMessage(uint8_t id, int exit_at_line);
void   defaultFontAspect();
void   logo();
void   setupDisplay();
void   setupTouch();
void   drawKeyBoard(uint16_t button_color, uint16_t text_color);
int    getTouchedButton();
void   checkFordisplayTouch(uint16_t *x, uint16_t *y);
void   clearCursor(uint16_t x, uint16_t y);
void   printBlinkCursor(uint16_t x, uint16_t y, uint16_t text_color, short int blinkTime);
void   printStringAt(uint16_t x, uint16_t y, char *text, uint16_t text_color); 
void   exitTouchedButton(char *label); 
void   lineEditor(char *statement, char *title, char *message, uint16_t text_color, uint16_t board_color, uint16_t key_color);




/*

variables handling, struct and functions   

*/

struct variables_struct {
  char   *var_name;
  double *var_value;
  int     var_size;
  struct  variables_struct *next;
};

struct variables_struct *varlist_ptr = NULL;
struct variables_struct *getVarPointerFromName(char *var_name);
double getVarValueFromName(char *var_name);
double getArrayValue(char *var_name, int array_index);
ERR updateArrayValues(char  *var_name, double var_value, int array_index);
ERR insertOrUpdateVariable(char *var_name, double var_value, int var_size);
bool isArray(char *var_name);
void printVariableList();
void deleteVariableList();



/*

 recursive descent expressions parser

*/

unsigned short int parenthesis_check = 0;          // to check the correct number of open and close parentheses //
double evalFunction(ERR *, PRGFUN, double);        // functions evaluation
double evalTermsExpression(ERR *, char **);        // final terms (numbers, variables, array, functions or expression)
double evalPowerExpression(ERR *, char **);        // raise to power expressions
double evalProductsExpression(ERR *, char **);     // Multiplications and divisions expressions
double evalSumsExpression(ERR *, char **);         // additions and subtractions expressions
double evalConditionalExpression(ERR *, char **);  // relations expressions
double evalBooleanExpression(ERR *, char **);      // boolean expressions
double evalMathExpression(ERR *, char*);           // math expression evaluation



/*

program list handling management structure and functions
 
*/

 
struct program_struct {           // program list struct
  int line_number;                // line number label
  char *instruction ;             // instruction to execute
  struct program_struct *next;    // next line pointer
};

struct program_struct *start_program_pointer = NULL; 
bool deleteInstruction(int line_number);
void searchInstructionTodelete(int line_number);
bool splitStatement(char *statement, char *instruction);
int  getStatementLineNumber(char *statement);
ERR  insertProgramLine(int line_number, char *instruction);
void printProgramList();
void deleteProgramList();
ERR  loadOrExecute(char *statement,  int *exit_at_line);


/*

for/next loop management structure and functions

*/

struct {
  struct program_struct *program_ptr;
  struct variables_struct *var_ptr;
  int array_index;
  double start_value;
  double stop_value;
  double step_value;
} for_loop[MAXFORLOOP];
int for_index = 0;
ERR execFOR(char *arg, struct program_struct **ptr_to_next_statement);
ERR execNEXT(struct program_struct **ptr_to_next_statement);


/*

gosub/return subrutine management structure and functions
 
*/

struct program_struct *gosub_stack[MAXSUB];
int gsb_index = 0;
ERR execGOSUB(char *arg, struct program_struct **ptr_to_next_statement);
ERR execRETURN(char *arg, struct program_struct **ptr_to_next_statement);




/*

Nanobasic instructions and utility functions 

*/

ERR getIdentifier(char *identifier, char *statement);
ERR getParenthesisExpression(char *expr_index, char *statement);
ERR parseInstructionArgs(char *arg, char *arg_list);
ERR getArgValue(uint16_t *argv, char *arg_list); 
ERR execDELAY(char *arg);
ERR execPINMODE(char *arg);
ERR execAWRITE(char *arg);
ERR execDWRITE(char *arg);
ERR execTONE(char *arg);
ERR execNOTONE(char *arg);
ERR execCLS(char *arg);
ERR execPIXEL(char *arg_list);
ERR execLINE(char *arg_list);
ERR execRECT(char *arg_list);
ERR execCIRCLE(char *arg_list);
ERR execREM(struct program_struct **ptr_to_next_statement);
bool parseDimArgument(char *array_list, char *var_name, int *dim_size, ERR *err);
ERR execDIM(char *array_list);
bool parsePrintArgument(char *arg_list, char *arg, ERR *err);
ERR execPRINTAT(char *arg_list);
ERR execPRINT(char *arg_list,bool printat);
ERR execINPUT(char *arg);
ERR execIF(char *arg, struct program_struct **ptr_to_next_statement);
ERR execGOTO(char *arg, struct program_struct **ptr_to_next_statement);
ERR execEND(struct program_struct **ptr_to_next_statement);
ERR execRUN(int *exit_at_line);
ERR execLIST();
ERR execLOAD(char *statement, int *exit_at_line);
ERR execSAVE(char *statement);
ERR execCLEAR();
void execRESET();
bool isAssignment(char *expr);
ERR execInstruction(char *instruction, struct program_struct **ptr_to_next_statement);

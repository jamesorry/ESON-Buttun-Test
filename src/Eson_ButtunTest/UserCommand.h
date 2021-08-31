#ifndef _USER_COMMAND_H_
#define	_USER_COMMAND_H_

#include "Arduino.h"

typedef struct __CMD {
  const char* cmd;
  void (*func)(void);
} CMD, *PCMD;

void resetArduino(void);
void getMicros(void);
void cmdOutput(void);
void cmdInput(void);
void cmdSetTotalTimes();
void cmdSetSpeed();
void cmdReadEEPROM();
void cmdWriteEEPROM();
void cmdClearEEPROM();
void cmdRunMode(void);
void echoOn(void);
void echoOff(void);
void cmd_CodeVer(void);
void showHelp(void);
bool getNextArg(String &arg);
void cmd_ScanLCD_Address(void);
void cmd_LCD_Display(void);
void cmd_LCD_Clear(void);
void cmd_out_test();

void UserCommand_Task(void);
void UserCommand_Timer(void);
#endif //_USER_COMMAND_H_

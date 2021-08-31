#include <avr/wdt.h>
#include "SoftwareSerial.h"
#include "UserCommand.h"
#include <Wire.h>
#include "Display.h"
#include "MainProcess.h"
#include "hmi.h"
#include "EEPROM_Function.h"

#if INO_DEBUG
#define USER_COMMAND_DEBUG  1
#endif

extern HardwareSerial *cmd_port;
extern MainDataStruct maindata;
extern RuntimeStatus runtimedata;

CMD g_cmdFunc[] = {
//在這新增function name 以及所呼叫的function
	{"out", cmdOutput},
	{"in", cmdInput},
    {"RD", cmdReadEEPROM},
    {"SD", cmdWriteEEPROM},
    {"CD", cmdClearEEPROM},
    {"reset", resetArduino},
    {"getmicros", getMicros},
    {"ver", cmd_CodeVer},
    {"echoon", echoOn},
    {"echooff", echoOff},
    {"runmode", cmdRunMode},
    {"ScanLCD", cmd_ScanLCD_Address},
    {"Display", cmd_LCD_Display},
    {"lcdclr", cmd_LCD_Clear},
    {"out_test", cmd_out_test},
    {"TotalTimes", cmdSetTotalTimes},
    {"Speed", cmdSetSpeed},
    {"?", showHelp}
};


String g_inputBuffer0 = "";
String* g_inputBuffer = NULL;
String g_cmd = "";
String g_arg = "";

bool g_echoOn = true;

uint32_t targetcnt = 0;

bool getNextArg(String &arg)
{
	if (g_arg.length() == 0)
		return false;
	if (g_arg.indexOf(" ") == -1)
	{
		arg = g_arg;
		g_arg.remove(0);
	}
	else
	{
		arg = g_arg.substring(0, g_arg.indexOf(" "));
		g_arg = g_arg.substring(g_arg.indexOf(" ") + 1);
	}
	return true;
}

void cmdSetTotalTimes()
{
    String arg1;
    int TotalTimes = 0;
    
    getNextArg(arg1);
    if( (arg1.length()==0))
    {
        cmd_port->println("Now TotalTimes: " + String(maindata.TotalTimes));
        return;
    }
    TotalTimes = arg1.toInt();
    maindata.TotalTimes = TotalTimes;
    runtimedata.UpdateEEPROM = true;
}
void cmdSetSpeed()
{
    String arg1;
    int Speed = 0;
    
    getNextArg(arg1);
    if( (arg1.length()==0))
    {
        cmd_port->println("Now Speed: " + String(maindata.Speed));
        return;
    }
    Speed = arg1.toInt();
    maindata.Speed = Speed;
    runtimedata.UpdateEEPROM = true;
    calculateDelayTime();
}
#if 0
#define RunMode_Stop        0
#define RunMode_Normal      1
#define RunMode_Start       2
#define RunMode_ChooseMode  3
#define RunMode_SetTimes    4 
#define RunMode_SetSpeed    5
#define RunMode_SetOK           6
#define RunMode_CheckSpeed      7
#endif
char *runmodeStrings[] = 
{   "RunMode_Stop : 0", 
    "RunMode_Normal : 1",
    "RunMode_Start : 2",
    "RunMode_ChooseMode : 3",
    "RunMode_SetTimes : 4",
    "RunMode_SetSpeed : 5",
    "RunMode_SetOK : 6",    
    "RunMode_CheckSpeed : 7",
};
const size_t nb_strings = sizeof(runmodeStrings) / sizeof(runmodeStrings[0]);

void cmdRunMode(void)
{
	String arg1, arg2;
	int runmode = 0;
	int motorindex = 0;
	
	getNextArg(arg1);
	if( (arg1.length()==0))
	{
	    for(int i=0; i<nb_strings; i++){
            cmd_port->println(runmodeStrings[i]);
        }
        cmd_port->println("---------------------");
        cmd_port->println("Now run mode: " + String(runtimedata.Workindex[workindex_RunMode]));
		return;
	}
	runmode = arg1.toInt();
	runtimedata.Workindex[workindex_RunMode] = runmode;
	cmd_port->println("Run mode: " + String(runtimedata.Workindex[workindex_RunMode]));
}


void cmdReadEEPROM()
{
    READ_EEPROM();
#if 0
    char        Vendor[10];
    uint8_t     HMI_ID;
    uint16_t    TotalTimes;
    uint16_t    Speed;
    
    cmd_port->println(": " + String(maindata.));
#endif
    cmd_port->println("HMI_ID: " + String(maindata.HMI_ID));
    cmd_port->println("TotalTimes: " + String(maindata.TotalTimes));
    cmd_port->println("Speed: " + String(maindata.Speed));
}
void cmdWriteEEPROM()
{
    runtimedata.UpdateEEPROM = true;
}
void cmdClearEEPROM()
{
    Clear_EEPROM();
}

void cmd_out_test()
{
	String arg1, arg2;
	int time,count;
	if (!getNextArg(arg1))
	{
        count = 5;
        return;
	}
	if (!getNextArg(arg2))
	{
        time = 0;
        return;
	}
    count = arg1.toInt();
    time = arg2.toInt();
    runtimedata.test = true;
    runtimedata.test_count = count;
    runtimedata.test_time = time;
}
void resetArduino(void)
{
	wdt_enable(WDTO_500MS);
	while (1);
}
void getMicros(void)
{
	cmd_port->println(String("micros:") + micros());
}

void showHelp(void)
{
	int i;

	cmd_port->println("");
	for (i = 0; i < (sizeof(g_cmdFunc) / sizeof(CMD)); i++)
	{
		cmd_port->println(g_cmdFunc[i].cmd);
	}
}


void cmdInput(void)
{
	String arg1;
	unsigned long pinindex;

	getNextArg(arg1);
	if( (arg1.length()==0))
	{
		cmd_port->println("Please input enough parameters");
		return;
	}
	pinindex = arg1.toInt();
    if(pinindex < INPUT_6_NUMBER*6){
	    cmd_port->println("Sensor: " + String(digitalRead(InputPin[pinindex])));
        cmd_port->println(analogRead(InputPin[pinindex]));
        cmd_port->println(getInput(pinindex));
    }
}

void cmdOutput(void)
{
	String arg1, arg2;
	int digitalPin;
	int value;

	if (!getNextArg(arg1))
	{
		cmd_port->println("No parameter 1");
		return;
	}
	if (!getNextArg(arg2))
	{
		cmd_port->println("No parameter 2");
		return;
	}
	digitalPin = arg1.toInt();
	value = arg2.toInt();

	cmd_port->print("PIN index:");
	cmd_port->println(arg1);
	cmd_port->print("level:");
	cmd_port->println(arg2);

	setOutput((uint8_t)digitalPin, (uint8_t)(value ? HIGH : LOW));
	cmd_port->println("");
}


void echoOn(void)
{
  g_echoOn = true;
}

void echoOff(void)
{
  g_echoOn = false;
}

void cmd_CodeVer(void)
{
  cmd_port->println(2021012001);
}

void cmd_ScanLCD_Address(void)
{
    byte error, address;
    int nDevices;

    Serial.println("Scanning...");

    nDevices = 0;
    for(address = 1; address < 127; address++ )
    {
        // The i2c_scanner uses the return value of
        // the Write.endTransmisstion to see if
        // a device did acknowledge to the address.
        Wire.beginTransmission(address);
        error = Wire.endTransmission();

        if (error == 0)
        {
            Serial.print("I2C device found at address 0x");
            if (address<16)
            Serial.print("0");
            Serial.print(address,HEX);
            Serial.println("  !");

            nDevices++;
        }
        else if (error==4)
        {
            Serial.print("Unknow error at address 0x");
            if (address<16)
            Serial.print("0");
            Serial.println(address,HEX);
        }    
    }
    if (nDevices == 0)
        Serial.println("No I2C devices found\n");
    else
        Serial.println("done\n");

}

void cmd_LCD_Display(void)
{
    String arg1, arg2;
    int y;
    
    if (!getNextArg(arg1))
    {
      cmd_port->println("No parameter 1");
      return;
    }
    if (!getNextArg(arg2))
    {
      cmd_port->println("No parameter 2");
      return;
    }
    
    y = arg1.toInt();
    if(y>3 || y<0)
    {
        cmd_port->println("parameter 1 need include 0~3");
        return;
    }
	Display(0, 0, y, String(arg2));
}

void cmd_LCD_Clear(void)
{
    String arg1, arg2;
    int y;
    
    if (!getNextArg(arg1))
    {
      cmd_port->println("No parameter 1");
      return;
    }
    
    y = arg1.toInt();
    if(y>3 || y<0)
    {
        Display(0, 0, 0, "                    ");//最多可顯示20字元 
        Display(0, 0, 1, "                    ");
        Display(0, 0, 2, "                    ");
        Display(0, 0, 3, "                    ");
    }
    else 
	    Display(0, 0, y, "                    ");
}


uint8_t UserCommWorkindex = 0;

uint32_t UserCommandTimeCnt = 0;

void UserCommand_Task(void)
{
  int i, incomingBytes, ret, cmdPortIndex;
  char data[2] = {0};

  switch(UserCommWorkindex)
  {
    case 0:
    {
      
      if(cmd_port->available())
      {
        g_inputBuffer = &g_inputBuffer0;
        UserCommWorkindex ++;
        UserCommandTimeCnt = millis();
      }
      break;
    }
    case 1:
    {
      if((millis() - UserCommandTimeCnt) > 50)
        UserCommWorkindex ++;
      break;
    }
    case 2:
    {
      if ( incomingBytes = cmd_port->available() )
      {

      cmd_port->println("cmd_port datalen: " + String(incomingBytes));

      for ( i = 0; i < incomingBytes; i++ )
      {
        ret = cmd_port->read();
        if ( (ret >= 0x20) && (ret <= 0x7E) || (ret == 0x0D) || (ret == 0x0A) )
        {
        data[0] = (char)ret;
        (*g_inputBuffer) += data;
        if (g_echoOn)
        {
          if ( (data[0] != 0x0D) && (data[0] != 0x0A) )
          cmd_port->write(data);
        }
        }
        else if (ret == 0x08)
        {
        if (g_inputBuffer->length())
        {
          g_inputBuffer->remove(g_inputBuffer->length() - 1);
          if (g_echoOn)
          {
          data[0] = 0x08;
          cmd_port->write(data);
          }
        }
        }
      }
      if (g_inputBuffer->indexOf('\r') == -1)
      {
        if (g_inputBuffer->indexOf('\n') == -1)
        return;
      }
      g_inputBuffer->trim();
      while (g_inputBuffer->indexOf('\r') != -1)
        g_inputBuffer->remove(g_inputBuffer->indexOf('\r'), 1);
      while (g_inputBuffer->indexOf('\n') != -1)
        g_inputBuffer->remove(g_inputBuffer->indexOf('\n'), 1);
      while (g_inputBuffer->indexOf("  ") != -1)
        g_inputBuffer->remove(g_inputBuffer->indexOf("  "), 1);
    
      cmd_port->println();
    
      if (g_inputBuffer->length())
      {
        g_arg.remove(0);
        if (g_inputBuffer->indexOf(" ") == -1)
        g_cmd = (*g_inputBuffer);
        else
        {
        g_cmd = g_inputBuffer->substring(0, g_inputBuffer->indexOf(" "));
        g_arg = g_inputBuffer->substring(g_inputBuffer->indexOf(" ") + 1);
        }
        for (i = 0; i < (sizeof(g_cmdFunc) / sizeof(CMD)); i++)
        {
        //if(g_cmd==g_cmdFunc[i].cmd)
        if (g_cmd.equalsIgnoreCase(g_cmdFunc[i].cmd))
        {
          g_cmdFunc[i].func();
          cmd_port->print("ARDUINO>");
          break;
        }
        else if (i == (sizeof(g_cmdFunc) / sizeof(CMD) - 1))
        {
          cmd_port->println("bad command !!");
          cmd_port->print("ARDUINO>");
        }
        }
        *g_inputBuffer = "";
      }
      else
      {
        cmd_port->print("ARDUINO>");
      }
      UserCommWorkindex = 0;
      break;
    }
  }

  }
}


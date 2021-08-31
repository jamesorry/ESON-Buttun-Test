/*
2021/08/18
宜笙按鈕測試
控制板為6IO
只有兩個按鈕
1.調整速度(1秒來回幾次?)
2.調整次數
3.開始執行次數
LCD顯示
*/
#include "Arduino.h"
#include "UserCommand.h"
#include "Display.h"
#include <Wire.h>
#include "Timer.h"
#include "MainProcess.h"
#include "hmi.h"
#include "EEPROM_Function.h"

#if 0
#include "gpio_button.h"
#define MY_BUTTON_PIN   8
// 定义按钮对象，指定按钮的GPIO口
GpioButton myButton(MY_BUTTON_PIN);
#endif

HardwareSerial *cmd_port;
uint8_t LCD_Address = 0x00;
extern MainDataStruct maindata;
extern RuntimeStatus runtimedata;

void setup() {
	cmd_port = &CMD_PORT;
	cmd_port->begin(CMD_PORT_BR);
    READ_EEPROM();
	TimerInit(1, 10000);
    Wire.begin();
    if(ScanLCD_Address())
	    Display_Init(0, LCD_Address);
	Display(0, 0, 0, "ESON                ");//最多可顯示20字元 
	Display(0, 0, 1, "Ver:" + String(VERSTR) + "      ");
    MainProcess_Init();
    cmd_port->println("End of setup.");

#if 0
    // 绑定按钮事件处理
    myButton.BindBtnPress([](){
        Serial.println("Led Button Press Event\r\n\tSetup Bind Event.");
        //digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN));
    });

    // 绑定长按事件处理（长按判定为1500ms）
    myButton.BindBtnLongPress([](){
        Serial.println("Led Button Long Press Event\r\n\tSetup Bind Event.");
        //digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN));
    }, 1500);
#endif

    buzzerPlay(1000);
    
}

void loop() {
	UserCommand_Task();
    MainProcess_Task();
    if(runtimedata.UpdateEEPROM)
	{
		runtimedata.UpdateEEPROM = false;
		WRITE_EEPROM(); //maindata內的值都會寫到EEPROM
	}    
}

bool ScanLCD_Address()
{
    bool result = false;
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
            LCD_Address = address;
            nDevices++;
            break;
        }
        else if (error==4)
        {
            Serial.print("Unknow error at address 0x");
            if (address<16)
            Serial.print("0");
            Serial.println(address,HEX);
        }    
    }
    if (nDevices == 0){
        Serial.println("No I2C devices found\n");
        result = false;
    }
    else{
        result = true;
        Serial.println("done\n");
    }
    return result;
}
ISR(TIMER1_COMPA_vect)
{
    MainPorcess_Timer();
}

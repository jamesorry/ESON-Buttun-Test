#include "MainProcess.h"
#include <Adafruit_MCP23017.h>
#include "hmi.h"
#include "Timer.h"
#include "Display.h"

extern HardwareSerial *cmd_port;
MainDataStruct maindata;
RuntimeStatus runtimedata;
DigitalIO digitalio;
Adafruit_MCP23017 extio[EXTIO_NUM];
#define MainProcess_Debug 0

uint16_t	WaitTimer = 0;
uint16_t	FreeTimer = 0;
uint16_t AddSubDelayTimer = 0;	//次數用長壓增減時間
uint16_t AddPressTimer = 0;
uint16_t SubPressTimer = 0;
void MainPorcess_Timer()
{
	if(WaitTimer < 0xFF00)
		WaitTimer += TIMER_INTERVAL_MS;
	if(FreeTimer < 0xFF00)
		FreeTimer += TIMER_INTERVAL_MS;
	if(AddPressTimer < 0xFF00)
		AddPressTimer += TIMER_INTERVAL_MS;
    
	if(SubPressTimer < 0xFF00)
		SubPressTimer += TIMER_INTERVAL_MS;
    
    if(AddSubDelayTimer < 0xFF00)
		AddSubDelayTimer += TIMER_INTERVAL_MS;
}

void MainProcess_ReCheckEEPROMValue()
{
	if((maindata.HMI_ID < 0) || (maindata.HMI_ID > 128))
	{
		maindata.HMI_ID = 0;
		runtimedata.UpdateEEPROM = true;
	}
	if((maindata.TotalTimes < 0) || (maindata.TotalTimes > 1000))
	{
		maindata.TotalTimes = 5;
		runtimedata.UpdateEEPROM = true;
	}
    if((maindata.Speed < 1) || (maindata.Speed > 10))
	{
		maindata.Speed = 1;
		runtimedata.UpdateEEPROM = true;
	}
}

void MainProcess_Init()
{
	int i,j;
	runtimedata.UpdateEEPROM = false;
	MainProcess_ReCheckEEPROMValue();
	for(i=0; i<INPUT_6_NUMBER*6; i++)
	{
		pinMode(InputPin[i], INPUT);
	}
	for(i=0; i<OUTPUT_6_NUMBER*6; i++)
	{
		pinMode(OutputPin[i], OUTPUT);	
	}
	for(i=0; i<OUTPUT_6_NUMBER*6; i++)
		digitalWrite(OutputPin[i], OUTPUT_NONE_ACTIVE);
    pinMode(BUZZ, OUTPUT);
    runtimedata.Workindex[workindex_RunMode] = RunMode_Stop;
    calculateDelayTime();
}
void calculateDelayTime()
{
    runtimedata.single_delay_time = 500/(maindata.Speed*2);
    cmd_port->println("runtimedata.single_delay_time: " + String(runtimedata.single_delay_time));
}
void setOutput(uint8_t index, uint8_t hl)
{
	if(index < (OUTPUT_6_NUMBER*6))
	{
		digitalWrite(OutputPin[index], hl);
	}
}

uint8_t getInput(uint8_t index)
{
	int readvalue;
	if(index < (INPUT_6_NUMBER*6))
	{
		readvalue = analogRead(InputPin[index]);
        if(readvalue > 128)
            return 1;
        else
            return 0;
	}
}
long checkspeed = 0;
int AddSpeedMode = 0;
int SubSpeedMode = 0;
int AddTimesMode = 0;
int SubTimesMode = 0;
int preworkindex_RunMode = -1;
void MainProcess_Task()  // This is a task.
{
    if(preworkindex_RunMode != runtimedata.Workindex[workindex_RunMode]){
        preworkindex_RunMode = runtimedata.Workindex[workindex_RunMode];
#if MainProcess_Debug
        cmd_port->println("RunMode: " + String(preworkindex_RunMode));
#endif
    }
    switch(runtimedata.Workindex[workindex_RunMode])
    {
        case RunMode_Stop:
            Page_Start();
            runtimedata.Workindex[workindex_RunMode] = RunMode_Normal;
            break;
        case RunMode_Normal:
            if(getInput(INPUT_START_BTN) && !getInput(INPUT_SET_BTN)){
                runtimedata.Workindex[workindex_StartProcess] = 0;
                runtimedata.NowTimes = 0;
                runtimedata.NeedStopTest = false;
                calculateDelayTime();
                runtimedata.Workindex[workindex_RunMode] = RunMode_Start;
//                Display(0, 0, 0, "Now Times: " + String(runtimedata.NowTimes) + "      ");
            }
            else if(getInput(INPUT_SET_BTN)){
                if(getInput(INPUT_START_BTN)){
                    runtimedata.chooseMode = 0;
                    Page_ChooseMode(runtimedata.chooseMode);
                    runtimedata.Workindex[workindex_RunMode] = RunMode_ChooseMode;
                }
            }
            break;
        case RunMode_Start:
            if(StartProcess()){
                runtimedata.Workindex[workindex_RunMode] = RunMode_Stop;
            }
            break;
        case RunMode_StartStop:
            if(getInput(INPUT_START_BTN)){
                runtimedata.NeedStopTest = false;
                runtimedata.Workindex[workindex_StartProcess] = 0;
                runtimedata.Workindex[workindex_RunMode] = RunMode_Start;
            }
            
            if(getInput(INPUT_SET_BTN)){ //歸零
                if(runtimedata.NeedStopTest){
                    runtimedata.NowTimes = 0;
                    runtimedata.NeedStopTest = false;
                    runtimedata.Workindex[workindex_RunMode] = RunMode_Stop;
                }
            }
            break;
        case RunMode_ChooseMode:
            ChooseModeProcess();
            if(!getInput(INPUT_SET_BTN)){
                switch(runtimedata.chooseMode){
                    case 0:
                        Display(0, 0, 0, "Setting Times        ");
                        Display(0, 0, 1, "Times: " + String(maindata.TotalTimes) + "      ");
                        runtimedata.Workindex[workindex_RunMode] = RunMode_SetTimes;
                        AddTimesMode = 0;
                        SubTimesMode = 0;
                        FreeTimer = 0;
                        break;
                    case 1:
                        Display(0, 0, 0, "Setting Speed        ");
                        Display(0, 0, 1, "Speed: " + String(maindata.Speed) + "/sec       ");
                        runtimedata.Workindex[workindex_RunMode] = RunMode_SetSpeed;
                        AddSpeedMode = 0;
                        SubSpeedMode = 0;
                        FreeTimer = 0;
                        break;
                }
            }
            break;
        case RunMode_SetTimes:
            SetTimesProcess();
            break;        
        case RunMode_SetSpeed:
            SetSpeedProcess();
            break;
        case RunMode_SetOK:
            Display(0, 0, 0, "Setting OK        ");
            Display(0, 0, 1, "Release all buttun");
            if(!getInput(INPUT_START_BTN) && !getInput(INPUT_SET_BTN)){
                runtimedata.UpdateEEPROM = true;
                runtimedata.Workindex[workindex_RunMode] = RunMode_Stop;
            }
            break;
        case RunMode_CheckSpeed:
            for(int i = 0; i<=5; i++){
                setOutput(i, 1);
            }
            checkspeed = millis();
            runtimedata.Workindex[workindex_RunMode] = 100;
            break;
        case 100:
            if(getInput(1)){
                cmd_port->println("Check speed:" + String(millis() - checkspeed));
                runtimedata.Workindex[workindex_RunMode] = RunMode_Stop;
            
                for(int i = 0; i<=5; i++){
                    setOutput(i, 0);
                }
            }
            break;
    }
}

int preAddTimesMode = -1;
int preSubTimesMode = -1;
uint8_t preAddBtnState_times = 0;
uint8_t preSubBtnState_times = 0;
void SetTimesProcess()
{
    uint8_t subbtn = getInput(INPUT_START_BTN);
    uint8_t addbtn = getInput(INPUT_SET_BTN);
//    if(addbtn && subbtn){
//        if(WaitTimer > 1000){
//            runtimedata.Workindex[workindex_RunMode] = RunMode_SetOK;
//        }
//    }
    if((addbtn && !preAddBtnState_times)){
        FreeTimer = 0;
        if(subbtn){
            if(WaitTimer > 1500){
                runtimedata.Workindex[workindex_RunMode] = RunMode_SetOK;
            }
        }
        else{
            if(preAddTimesMode != AddTimesMode)
            {
                preAddTimesMode = AddTimesMode;
#if MainProcess_Debug
                cmd_port->println("AddTimesMode: " + String(preAddTimesMode));
#endif
            }
            switch(AddTimesMode)
            {
                case 0:
                    if(AddPressTimer >= 2000)
                    {
                        AddTimesMode += 10;
                    }
                    if(AddSubDelayTimer > 500){ //debug時可修改
                        maindata.TotalTimes += 1;
                        AddSubDelayTimer = 0;
                    }
                    if(maindata.TotalTimes >= 1000) maindata.TotalTimes = 1000;
                    Display(0, 0, 1, "Times: " + String(maindata.TotalTimes) + "      ");
                    break;
                case 10:
                    if(AddPressTimer >= 3000)
                    {
                        AddTimesMode += 10;
                    }
                    maindata.TotalTimes += 5;
                    if(maindata.TotalTimes >= 1000) maindata.TotalTimes = 1000;
                    Display(0, 0, 1, "Times: " + String(maindata.TotalTimes) + "      ");
                    break;
                case 20:
                    if(AddPressTimer >= 4000)
                    {
                        AddTimesMode += 10;
                    }
                    maindata.TotalTimes += 10;
                    if(maindata.TotalTimes >= 1000) maindata.TotalTimes = 1000;
                    Display(0, 0, 1, "Times: " + String(maindata.TotalTimes) + "      ");
                    break;
                case 30:
                    maindata.TotalTimes += 100;
                    if(maindata.TotalTimes >= 1000) maindata.TotalTimes = 1000;
                    Display(0, 0, 1, "Times: " + String(maindata.TotalTimes) + "      ");
                    break;
            }
        }
    }
    if((subbtn && !preSubBtnState_times)){
        FreeTimer = 0;
        if(addbtn){
            if(WaitTimer > 1500){
                runtimedata.Workindex[workindex_RunMode] = RunMode_SetOK;
            }
        }
        else{
            if(preSubTimesMode != SubTimesMode)
            {
                preSubTimesMode = SubTimesMode;
#if MainProcess_Debug
                cmd_port->println("SubTimesMode: " + String(preSubTimesMode));
#endif
            }
            switch(SubTimesMode)
            {
                case 0:
                    if(SubPressTimer >= 2000)
                    {
                        SubTimesMode += 10;
                    }
                    if(AddSubDelayTimer > 500){ //debug時可修改
                        maindata.TotalTimes -= 1;
                        AddSubDelayTimer = 0;
                    }
                    if(maindata.TotalTimes <= 1) maindata.TotalTimes = 1;
                    Display(0, 0, 1, "Times: " + String(maindata.TotalTimes) + "      ");
                    break;
                case 10:
                    if(SubPressTimer >= 3000)
                    {
                        SubTimesMode += 10;
                    }
                    maindata.TotalTimes -= 5;
                    if(maindata.TotalTimes <= 1) maindata.TotalTimes = 1;
                    Display(0, 0, 1, "Times: " + String(maindata.TotalTimes) + "      ");
                    break;
                case 20:
                    if(SubPressTimer >= 4000)
                    {
                        SubTimesMode += 10;
                    }
                    maindata.TotalTimes -= 10;
                    if(maindata.TotalTimes <= 1) maindata.TotalTimes = 1;
                    Display(0, 0, 1, "Times: " + String(maindata.TotalTimes) + "      ");
                    break;
                case 30:
                    maindata.TotalTimes -= 100;
                    if(maindata.TotalTimes <= 1) maindata.TotalTimes = 1;
                    Display(0, 0, 1, "Times: " + String(maindata.TotalTimes) + "      ");
                    break;
            }
        }
    }
    {
        if(FreeTimer>2000){ //調整空閒時間
            runtimedata.Workindex[workindex_RunMode] = RunMode_SetOK;
        }
    }
    if(!getInput(INPUT_SET_BTN)){
        preAddBtnState_times = addbtn;
        AddPressTimer = 0;
        AddTimesMode = 0;
        WaitTimer = 0;
    }
    if(!getInput(INPUT_START_BTN)){
        preSubBtnState_times = subbtn;
        SubPressTimer = 0;
        SubTimesMode = 0;
        WaitTimer = 0;
    }
}

int preAddSpeedMode = -1;
int preSubSpeedMode = -1;
uint8_t preAddBtnState_speed = 0;
uint8_t preSubBtnState_speed = 0;
void SetSpeedProcess()
{
    uint8_t subbtn = getInput(INPUT_START_BTN);
    uint8_t addbtn = getInput(INPUT_SET_BTN);
//    if(addbtn && subbtn){
//        if(WaitTimer > 1000){
//            runtimedata.Workindex[workindex_RunMode] = RunMode_SetOK;
//        }
//    }
    if((addbtn && !preAddBtnState_speed)){
        FreeTimer = 0;
        if(subbtn){
            if(WaitTimer > 1500){
                runtimedata.Workindex[workindex_RunMode] = RunMode_SetOK;
            }
        }
        else{
            if(preAddSpeedMode != AddSpeedMode)
            {
                preAddSpeedMode = AddSpeedMode;
#if MainProcess_Debug
                cmd_port->println("AddSpeedMode: " + String(preAddSpeedMode));
#endif
            }
            switch(AddSpeedMode)
            {
                case 0:
                    if(AddPressTimer >= 2000)
                    {
                        AddSpeedMode += 10;
                    }
                    if(AddSubDelayTimer > 500){ //debug時可修改
                        maindata.Speed += 1;
                        AddSubDelayTimer = 0;
                    }
                    if(maindata.Speed >= 10) maindata.Speed = 10;
                    Display(0, 0, 1, "Speed: " + String(maindata.Speed) + "/sec       ");
                    break;
                case 10:
                    maindata.Speed += 5;
                    if(maindata.Speed >= 10) maindata.Speed = 10;
                    Display(0, 0, 1, "Speed: " + String(maindata.Speed) + "/sec       ");
                    break;
            }
        }
    }
    if((subbtn && !preSubBtnState_speed)){
        FreeTimer = 0;
        if(addbtn){
            if(WaitTimer > 1500){
                runtimedata.Workindex[workindex_RunMode] = RunMode_SetOK;
            }
        }
        else{
            if(preSubSpeedMode != SubSpeedMode)
            {
                preSubSpeedMode = SubSpeedMode;
#if MainProcess_Debug
                cmd_port->println("SubSpeedMode: " + String(preSubSpeedMode));
#endif
            }
            switch(SubSpeedMode)
            {
                case 0:
                    if(SubPressTimer >= 2000)
                    {
                        SubSpeedMode += 10;
                    }
                    if(AddSubDelayTimer > 500){ //debug時可修改
                        maindata.Speed -= 1;
                        AddSubDelayTimer = 0;
                    }
                    if(maindata.Speed <= 1) maindata.Speed = 1;
                    Display(0, 0, 1, "Speed: " + String(maindata.Speed) + "/sec       ");
                    break;
                case 10:
                    maindata.Speed -= 5;
                    if(maindata.Speed <= 1) maindata.Speed = 1;
                    Display(0, 0, 1, "Speed: " + String(maindata.Speed) + "/sec       ");
                    break;
            }
        }
    }
    {
        if(FreeTimer>2000){ //調整空閒時間
            runtimedata.Workindex[workindex_RunMode] = RunMode_SetOK;
        }
    }
    if(!getInput(INPUT_SET_BTN)){
        preAddBtnState_speed = addbtn;
        AddPressTimer = 0;
        AddSpeedMode = 0;
        WaitTimer = 0;
    }
    if(!getInput(INPUT_START_BTN)){
        preSubBtnState_speed = subbtn;
        SubPressTimer = 0;
        SubSpeedMode = 0;
        WaitTimer = 0;
    }
}


uint8_t preChooseBtnState = 0;
void ChooseModeProcess()
{
    uint8_t choosebtn = getInput(INPUT_START_BTN);
    if((choosebtn && !preChooseBtnState) && getInput(INPUT_SET_BTN)){
        if(runtimedata.chooseMode == 0){
            runtimedata.chooseMode = 1;
        }
        else if(runtimedata.chooseMode == 1){
            runtimedata.chooseMode = 0;
        }
        Page_ChooseMode(runtimedata.chooseMode);
    }
    preChooseBtnState = choosebtn;
}

int preworkindex_StartProcess = -1;
long checktime = 0;
bool StartProcess()
{
    bool result = false;
    if(preworkindex_StartProcess != runtimedata.Workindex[workindex_StartProcess]){
        preworkindex_StartProcess = runtimedata.Workindex[workindex_StartProcess];
#if MainProcess_Debug
        cmd_port->println("RunMode: " + String(preworkindex_StartProcess));
#endif
    }
    switch(runtimedata.Workindex[workindex_StartProcess])
    {
#if 0
        if(runtimedata.test){
            for(int test = 0; test<runtimedata.test_count; test++){
                for(int i = 0; i<=5; i++){
                    setOutput(i, 1);
                }
                delay(runtimedata.test_time);
                for(int i = 0; i<=5; i++){
                    setOutput(i, 0);
                }
                delay(runtimedata.test_time);
            }
            runtimedata.test=false;
        }
#endif
        case 0:
            if(!getInput(INPUT_START_BTN)){ //需要停止
                runtimedata.Workindex[workindex_StartProcess] += 5;
            }
            break;
        case 5:
            if(!runtimedata.NeedStopTest){
                if(runtimedata.NowTimes < maindata.TotalTimes){
                    if(getInput(INPUT_START_BTN)){ //需要停止
                        runtimedata.NeedStopTest = true;
                    }
                    Display(0, 0, 0, "Now Times: " + String(runtimedata.NowTimes) + "      ");
                    runtimedata.Workindex[workindex_StartProcess] += 5;
                }
                else{ //已經達到次數
//                  buzzerPlay(100);
                    cmd_port->println("Now Times:" + String(runtimedata.NowTimes));
                    cmd_port->println("Total Times:" + String(maindata.TotalTimes));
                    Display(0, 0, 0, "Now Times: " + String(runtimedata.NowTimes) + "      ");
                    runtimedata.Workindex[workindex_StartProcess] = 1000;
                }
            }
            else{
                if(!getInput(INPUT_START_BTN))
                    runtimedata.Workindex[workindex_RunMode] = RunMode_StartStop;
            }
            break;
        case 10:
            for(int i = 0; i<=5; i++){
                setOutput(i, 1);
            }
            checktime = millis();
            runtimedata.Workindex[workindex_StartProcess] += 10;
            break;
        case 20:
            if(millis() - checktime > runtimedata.single_delay_time)
                runtimedata.Workindex[workindex_StartProcess] += 10;
            break;
        case 30:
            for(int i = 0; i<=5; i++){
                setOutput(i, 0);
            }
            checktime = millis();
            runtimedata.Workindex[workindex_StartProcess] += 10;
            break;
        case 40:
            if(millis() - checktime > runtimedata.single_delay_time){
                runtimedata.NowTimes ++;
                runtimedata.Workindex[workindex_StartProcess] += 10;
            }
            break;
        case 50:
            Display(0, 0, 0, "Now Times: " + String(runtimedata.NowTimes) + "      ");
            runtimedata.Workindex[workindex_StartProcess] = 5;
            break;
            
        default:
            result = true;
            break;
    }
    return result;
}
void Page_Start()
{
//  Display(0, 0, 0, "                    ");
    Display(0, 0, 0, "Times: " + String(maindata.TotalTimes) + "      ");
    Display(0, 0, 1, "Speed: " + String(maindata.Speed) + "/sec       ");
}

void Page_ChooseMode(uint8_t mode)
{
    switch(mode){
        case 0:
            Display(0, 0, 0, "Choose setting       ");
            Display(0, 0, 1, "Times                ");
            break;
        case 1:
            Display(0, 0, 0, "Choose setting       ");
            Display(0, 0, 1, "Speed                ");
            break;
    }
}

void buzzerPlay(int playMS)
{
  digitalWrite(BUZZ, HIGH);
  delay(playMS);
  digitalWrite(BUZZ, LOW);
}



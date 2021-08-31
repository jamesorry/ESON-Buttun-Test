#ifndef _GPIO_BUTTON_H_
#define _GPIO_BUTTON_H_
#include <Arduino.h>

#define        DEF_ELIMINATING_JITTER_MS        20                // 默认消抖延时
#define DEF_LONG_PRESS_WAIT_MS  1000                // 默认长按延时

class GpioButton {
    public:
                    // 构造函数，定义端口号，回调函数，初始化默认值
        GpioButton(uint8_t gpio_pin, void(*btn_press_event)()=nullptr) :
            GpioPin(gpio_pin), 
            ButtonPressEvent(btn_press_event), 
            LongPressWaitMS(DEF_LONG_PRESS_WAIT_MS), 
            ButtonLongPressEvent(nullptr),
            key_down_millis(0),
            action_done(false) {
                pinMode(GpioPin, INPUT_PULLUP);
                digitalWrite(GpioPin, HIGH);
            };
        // 绑定按键回调函数
        void BindBtnPress(void(*btn_press_event)()) {
            ButtonPressEvent = btn_press_event;
        };
        // 绑定长按事件回调函数和长按的判定时长
        bool BindBtnLongPress(void(*btn_long_press_event)(), uint16_t wait_ms=DEF_LONG_PRESS_WAIT_MS) {
            if(wait_ms < DEF_LONG_PRESS_WAIT_MS) return false;
            ButtonLongPressEvent = btn_long_press_event;
            LongPressWaitMS = wait_ms;
            return true;
        };
        // 轮询函数
        void loop(){
            // 读取端口状态
            uint8_t pin_val = digitalRead(GpioPin);
            // 获取当前系统时间
            uint32_t now_millis = millis();
            // 计算按下时点到当前经过的毫秒数
            uint32_t pass_millis = now_millis - key_down_millis;
            // 记录按键按下时点的系统时间，清楚动作执行标志
            if(pin_val == LOW && key_down_millis == 0) {
                key_down_millis = now_millis;
                action_done = false;
            }
            // 按键按下状态如果超过长按判定时长时，调用长按事件回调（如果已绑定长按事件回调）
            else if(pin_val == LOW && ButtonLongPressEvent != nullptr && pass_millis > LongPressWaitMS && !action_done) {
                            // 防止重复调用，设置执行标志
                action_done = true;
                // 调用回调方法
                ButtonLongPressEvent();
            }
            // 按键释放状态
            else if(pin_val == HIGH) {
                            // 如果是按键释放瞬间，按下时长超过消抖间隔时长，且尚未执行过回调函数
                if(!action_done && key_down_millis > 0 && pass_millis > DEF_ELIMINATING_JITTER_MS && ButtonPressEvent != nullptr) {
                                // 设置执行标志
                    action_done = true;
                    // 调用回调方法
                    ButtonPressEvent();
                }
                // 清空按下时点的值
                key_down_millis = 0;
            }
        };
    protected:
        uint8_t GpioPin;
        void (*ButtonPressEvent)();
        uint16_t LongPressWaitMS;
        void (*ButtonLongPressEvent)();
        uint32_t key_down_millis;
        bool action_done;
};

#endif


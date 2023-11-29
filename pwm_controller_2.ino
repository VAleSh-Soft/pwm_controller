/**
 * @file pwm_controller_2.ino
 * @author Vladimir Shatalov (valesh-soft@yandex.ru)
 * @brief Управление нагревателем с помощью ШИМ; текущий уровень мощности индицируется четырьмя светодиодами;
 *        управление уровнем мощности одной кнопкой
 * @version 1.0
 * @date 29.11.2023
 *
 * @copyright Copyright (c) 2023
 *
 */
#include <shButton.h>

const uint8_t led_1_pin = 3;
const uint8_t led_2_pin = 4;
const uint8_t led_3_pin = 5;
const uint8_t led_4_pin = 6;

const uint8_t pwm_out = 10;

const uint8_t btn_pin = 7;

class Heater
{
private:
  uint8_t pwm_data = 0;
  uint8_t pwm_pin = pwm_out;
  uint8_t led_1 = led_1_pin;
  uint8_t led_2 = led_2_pin;
  uint8_t led_3 = led_3_pin;
  uint8_t led_4 = led_4_pin;

  void set_led_state()
  {
    digitalWrite(led_1, pwm_data >= 60);
    digitalWrite(led_2, pwm_data >= 120);
    digitalWrite(led_3, pwm_data >= 180);
    digitalWrite(led_4, pwm_data >= 240);
  }

public:
  Heater()
  {
    pinMode(led_1, OUTPUT);
    pinMode(led_2, OUTPUT);
    pinMode(led_3, OUTPUT);
    pinMode(led_4, OUTPUT);
    pinMode(pwm_pin, OUTPUT);
  }

  void set_pwm_data(uint8_t _data) { pwm_data = _data; }
  uint8_t get_pwm_data() { return pwm_data; }

  void tick()
  {
    static uint8_t old_data = 0;
    if (old_data != pwm_data)
    {
      old_data = pwm_data;
      analogWrite(pwm_pin, pwm_data);
      set_led_state();
    }
  }
};

shButton btn(btn_pin);

Heater heater;

void check_button()
{
  uint8_t x;

  switch (btn.getButtonState())
  {
  case BTN_ONECLICK: // при однократном клике мощность нагревателя увеличивается на один пункт по кругу;
    x = heater.get_pwm_data();
    x += 60;
    if (x > 240)
    {
      x = 60;
    }
    break;
  case BTN_LONGCLICK: // при удержании кнопки в течение двух секунд нагреватель отключается
    x = 0;
  }
  heater.set_pwm_data(0);
}

void setup()
{
  btn.setVirtualClickOn();
  btn.setLongClickMode(LCM_ONLYONCE);
  btn.setTimeoutOfLongClick(2000);
}

void loop()
{
  check_button();
  heater.tick();
}

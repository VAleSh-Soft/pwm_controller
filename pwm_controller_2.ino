/**
 * @file pwm_controller_2.ino
 * @author Vladimir Shatalov (valesh-soft@yandex.ru)
 * @brief Управление нагревателем с помощью ШИМ и управлением одной кнопкой;
 *
 * Управление:
 *   - нагреватель имеет четыре уровня мощности: pwm_data == {100, 150, 200, 250} т.е. примерно 39%, 59%, 78% и 98%;
 *   - изменение уровня выполняется одной кнопкой; перебор уровней закольцован - при максимальном уровне клик кнопкой включает минимальный уровень;
 *   - удержание кнопки нажатой в течение 1 секунды отключает нагреватель;
 *
 * Индикация:
 *   - текущий уровень мощности индицируется линейкой из четырех светодиодов;
 *   - первый светодиод линейки двухцветный, красный цвет зажигается, если нагреватель отключен (pwm_data == 0);
 *
 * @version 1.3
 * @date 17.12.2023
 *
 * @copyright Copyright (c) 2023
 *
 */
#define __ARDUINO__ defined(__AVR_ATmega328P__) || defined(__AVR_ATmega328PB__) || defined(__AVR_ATmega328__) || defined(__AVR_ATmega168__) || defined(__AVR_ATmega168P__)
#define __DIGISPARK__ defined(__AVR_ATtiny85__) || defined(__AVR_ATtiny45__)

#include <shButton.h>
#include <EEPROM.h>

const uint16_t eeprom_index = 10;

#if __ARDUINO__
const uint8_t led_1_pin = 7;
const uint8_t led_2_pin = 6;
const uint8_t led_3_pin = 5;
const uint8_t led_4_pin = 4;
const uint8_t led_off_pin = 8;

const uint8_t pwm_out = 10;

const uint8_t btn_pin = 3;
#elif __DIGISPARK__
const uint8_t led_1_pin = 1;
const uint8_t led_off_pin = 0;

const uint8_t pwm_out = 4;

const uint8_t btn_pin = 3;
#endif

class Heater
{
private:
  uint8_t pwm_data = 0;
  uint8_t pwm_pin = pwm_out;
  uint8_t led_1 = led_1_pin;
#if __ARDUINO__
  uint8_t led_2 = led_2_pin;
  uint8_t led_3 = led_3_pin;
  uint8_t led_4 = led_4_pin;
#endif
  uint8_t led_off = led_off_pin;

  void set_led_state()
  {
    digitalWrite(led_off, (pwm_data == 0));
    digitalWrite(led_1, (pwm_data >= 100));
#if __ARDUINO__
    digitalWrite(led_2, (pwm_data >= 150));
    digitalWrite(led_3, (pwm_data >= 200));
    digitalWrite(led_4, (pwm_data >= 250));
#endif
  }

public:
  Heater()
  {
    pinMode(led_1, OUTPUT);
#if __ARDUINO__
    pinMode(led_2, OUTPUT);
    pinMode(led_3, OUTPUT);
    pinMode(led_4, OUTPUT);
#endif
    pinMode(led_off, OUTPUT);
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
      if (pwm_data > 0)
      {
        EEPROM.update(eeprom_index, pwm_data);
      }
      Serial.print("pwm_data: ");
      Serial.println(pwm_data);
      EEPROM.update(eeprom_index + 1, (bool)pwm_data);
    }
    set_led_state();
  }
};

shButton btn(btn_pin);

Heater heater;

// получение и валидация сохраненного в EEPROM значения ШИМ
uint8_t get_start_pwm()
{
  uint8_t x = EEPROM.read(eeprom_index);
  if (x < 100 || x > 250)
  // если в памяти записано некорректное значение, устанавливаем минимальный уровень ШИМ
  {
    x = 100;
    EEPROM.update(eeprom_index, x);
  }
  return (x);
}

// опрос кнопки
void check_button()
{
  uint8_t x = heater.get_pwm_data();

  switch (btn.getButtonState())
  {
  case BTN_ONECLICK: // при однократном клике мощность нагревателя изменяется на один пункт по кругу;
    if (x == 0)
    {
      // если ШИМ отключен, запустить нагреватель с сохраненным значением
      x = get_start_pwm();
    }
    else
    {
      // иначе изменить текущий уровень ШИМ
      x += 50;
      if (x < 100)
      {
        x = 100;
      }
    }
    heater.set_pwm_data(x);
    break;
  case BTN_LONGCLICK: // при удержании кнопки в течение одной секунды нагреватель отключается или включается
    if (x == 0)
    {
      // если ШИМ отключен, запустить нагреватель с сохраненным значением
      x = get_start_pwm();
    }
    else
    {
      // иначе отключить нагреватель
      x = 0;
    }
    heater.set_pwm_data(x);
  }
}

void setup()
{
  Serial.begin(9600);

// увеличиваем частоту ШИМ до 31372.55 Гц
#if __ARDUINO__
  TCCR1B = TCCR1B & B11111000 | B00000001; // на пинах D9 и D10 (timer1) до 31372.55 Гц
#elif __DIGISPARK__
  TCCR1 = TCCR1 & B11110000 | B00000001; // на пине PB4 (timer1) до 3968.25 Гц
#endif

  btn.setVirtualClickOn();
  btn.setLongClickMode(LCM_ONLYONCE);
  btn.setTimeoutOfLongClick(1000);

  // запускаем нагреватель с сохраненным уровнем ШИМ
  if ((bool)EEPROM.read(eeprom_index + 1))
  {
    heater.set_pwm_data(get_start_pwm());
  }

  Serial.println("Device started");
}

void loop()
{
  check_button();
  heater.tick();
}

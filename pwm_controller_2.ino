/**
 * @file pwm_controller_2.ino
 * @author Vladimir Shatalov (valesh-soft@yandex.ru)
 * @brief Управление нагревателем с помощью ШИМ и управлением одной кнопкой;
 *
 * Управление:
 *   - нагреватель имеет четыре уровня мощности;
 *   - изменение уровня выполняется кликом кнопкой; перебор уровней закольцован - при максимальном уровне клик кнопкой включает минимальный уровень;
 *   - удержание кнопки нажатой в течение 2 секунд отключает нагреватель;
 *
 * Индикация:
 *   - текущий уровень мощности индицируется линейкой из четырех светодиодов;
 *   - первый светодиод линейки двухцветный, красный цвет зажигается, если нагреватель отключен (pwm_data == 0);
 *
 * @version 1.0
 * @date 29.11.2023
 *
 * @copyright Copyright (c) 2023
 *
 */
#include <shButton.h>
#include <EEPROM.h>

const uint16_t eeprom_index = 10;

const uint8_t led_1_pin = 4;
const uint8_t led_2_pin = 5;
const uint8_t led_3_pin = 6;
const uint8_t led_4_pin = 7;
const uint8_t led_off_pin = 8;

const uint8_t pwm_out = 10;

const uint8_t btn_pin = 3;

// считывание сохраненного значения ШИМ из EEPROM
uint8_t read_pwm_data() { return (EEPROM.read(eeprom_index)); }

// сохранение нового значения ШИМ в EEPROM
void write_pwm_data(uint8_t _data) { EEPROM.update(eeprom_index, _data); }

// считывание флага запуска ШИМ
bool read_on_off_pwm_state() { return ((bool)EEPROM.read(eeprom_index + 1)); }

// запись флага запуска ШИМ
void write_on_off_pwm_state(bool _flag) { EEPROM.update(eeprom_index + 1, _flag); }

class Heater
{
private:
  uint8_t pwm_data = 0;
  uint8_t pwm_pin = pwm_out;
  uint8_t led_1 = led_1_pin;
  uint8_t led_2 = led_2_pin;
  uint8_t led_3 = led_3_pin;
  uint8_t led_4 = led_4_pin;
  uint8_t led_off = led_off_pin;

  void set_led_state()
  {
    digitalWrite(led_off, (pwm_data == 0));
    digitalWrite(led_1, (pwm_data >= 60));
    digitalWrite(led_2, (pwm_data >= 120));
    digitalWrite(led_3, (pwm_data >= 180));
    digitalWrite(led_4, (pwm_data >= 240));
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
      if (pwm_data > 0)
      {
        write_pwm_data(pwm_data);
      }
      write_on_off_pwm_state((bool)pwm_data);
    }
  }
};

shButton btn(btn_pin);

Heater heater;

// опрос кнопки
void check_button()
{
  switch (btn.getButtonState())
  {
  case BTN_ONECLICK: // при однократном клике мощность нагревателя изменяется на один пункт по кругу;
    uint8_t x;
    x = heater.get_pwm_data();
    if (x == 0)
    {
      // если ШИМ отключен, запустить нагреватель с сохраненным значением
      x = read_pwm_data();
      if (x == 0 || x > 240)
      {
        x = 60;
      }
    }
    else
    {
      // иначе изменить текущий уровень ШИМ
      x += 60;
      if (x > 240)
      {
        x = 60;
      }
    }
    heater.set_pwm_data(x);
    break;
  case BTN_LONGCLICK: // при удержании кнопки в течение двух секунд нагреватель отключается
    heater.set_pwm_data(0);
  }
}

void setup()
{
  btn.setVirtualClickOn();
  btn.setLongClickMode(LCM_ONLYONCE);
  btn.setTimeoutOfLongClick(2000);

  // если в памяти записано некорректное значение (0xFF или 0x00), устанавливаем минимальный уровень
  if (read_pwm_data() > 240 || read_pwm_data() == 0)
  {
    write_pwm_data(60);
  }
  // запускаем нагреватель с сохраненным уровнем ШИМ
  if (read_on_off_pwm_state())
  {
    heater.set_pwm_data(read_pwm_data());
  }
}

void loop()
{
  check_button();
  heater.tick();
}

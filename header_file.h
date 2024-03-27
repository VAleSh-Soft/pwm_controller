#pragma once

#if defined(__AVR_ATmega328P__) || defined(__AVR_ATmega328PB__) || defined(__AVR_ATmega328__) || defined(__AVR_ATmega168__) || defined(__AVR_ATmega168P__)
#define __ARDUINO__ 1
#else
#define __ARDUINO__ 0
#endif

#if defined(__AVR_ATtiny85__) || defined(__AVR_ATtiny45__)
#define __DIGISPARK__ 1
#else
#define __DIGISPARK__ 0
#endif

#include <Arduino.h>
#include <EEPROM.h>

// ==== Настройки ====================================

#if __ARDUINO__
// Вывод отладочной информации в Serial; используется только для Ардуино
#define LOG_ON

#endif

// использовать две кнопки для управления регулятором
#define USE_TWO_BUTTONS

// таблица управления нагревателем, линейная зависимость: примерно 39%, 59%, 78% и 98%
// uint8_t pwm_data_table[] = {0, 100, 150, 200, 250};
// таблица управления светодиодной лентой, нелинейная зависимость: примерно 12%, 24%, 49% и 98%
uint8_t pwm_data_table[] = {0, 30, 60, 125, 250};

#ifdef USE_TWO_BUTTONS
// закольцевать перебор значений ШИМ кнопками
bool loop_data_of_pwm = true;

#endif

// плавный запуск ШИМ
bool smooth_start = false;

// ===================================================

const uint16_t eeprom_index = 10;

// ===================================================

// получение и валидация сохраненного в EEPROM текущего значения ШИМ, возвращает индекс значения в массиве pwm_data_table
uint8_t get_start_pwm();

// опрос кнопок
void check_button();

// ============================================================================

// настройка пинов для подключения кнопок, светодиодов и выхода ШИМ для Arduino
#if __ARDUINO__
const uint8_t led_1_pin = 7;
const uint8_t led_2_pin = 6;
const uint8_t led_3_pin = 5;
const uint8_t led_4_pin = 4;
const uint8_t led_off_pin = 8;

const uint8_t pwm_out_pin = 10;

const uint8_t btn_up_pin = 3;
const uint8_t btn_down_pin = 2;

// настройка пинов для подключения кнопок, светодиодов и выхода ШИМ для Attiny45/85
#elif __DIGISPARK__
const uint8_t led_1_pin = 1;
const uint8_t led_off_pin = 0;

const uint8_t pwm_out_pin = 4;

const uint8_t btn_up_pin = 2;
const uint8_t btn_down_pin = 3;

#endif

// ===================================================

#ifdef LOG_ON

#define LOG_PRINT(x) Serial.print(x)
#define LOG_PRINTLN(x) Serial.println(x)

#else

#define LOG_PRINT(x)
#define LOG_PRINTLN(x)

#endif

// ===================================================

class PWM_Controller
{
private:
  uint8_t pwm_data_index = 0;
  uint8_t pwm_pin = pwm_out_pin;
  uint8_t led_1 = led_1_pin;
  bool smooth_start_on = false;   // если true, значит в данный момент происходит плавный запуск нагрузки
  bool smooth_start_flag = false; // если true, используется плавный запуск нагрузки (например, для плавного запуска источника света), иначе нагрузка сразу стартует с заданной мощностью
#if __ARDUINO__
  uint8_t led_2 = led_2_pin;
  uint8_t led_3 = led_3_pin;
  uint8_t led_4 = led_4_pin;
#endif
  uint8_t led_off = led_off_pin;

  void set_led_state()
  {
    static uint32_t timer = millis();

    // состояние светодиодов обновляется 20 раз в секунду
    if (millis() - timer >= 50)
    {
      timer += 50;
      digitalWrite(led_off, (pwm_data_index == 0));
      digitalWrite(led_1, (pwm_data_index >= 1));
#if __ARDUINO__
      digitalWrite(led_2, (pwm_data_index >= 2));
      digitalWrite(led_3, (pwm_data_index >= 3));
      digitalWrite(led_4, (pwm_data_index == 4));
#endif
    }
  }

  void smooth_start_pwm()
  {
    const uint32_t interval = 10; // наращиваем значение ШИМ каждые 10 мс
    const uint8_t increment = 5;  // шаг прироста ШИМ

    static uint32_t timer = millis();

    static uint8_t _data = 0;

    if (millis() - timer >= interval)
    {
      timer = millis();
      _data += increment;
      if ((_data < pwm_data_table[pwm_data_index]) && (_data != increment - 1) && (pwm_data_index != 0))
      {
        analogWrite(pwm_pin, _data);
        LOG_PRINT("pwm_data: ");
        LOG_PRINTLN(_data);
      }
      else
      {
        _data = 0;
        analogWrite(pwm_pin, pwm_data_table[pwm_data_index]);
        smooth_start_on = false;
        LOG_PRINT("pwm_data: ");
        LOG_PRINTLN(pwm_data_table[pwm_data_index]);
      }
    }
  }

public:
  PWM_Controller()
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

  // установка значения ШИМ (в метод передается индекс значения в массиве pwm_data_table)
  void set_pwm_data(uint8_t _data) { pwm_data_index = _data; }
  // получение текущего значения ШИМ (возвращает индекс значения в массиве pwm_data_table)
  uint8_t get_pwm_data() { return pwm_data_index; }

  // включение/выключение режима плавного пуска нагрузки
  void set_smooth_flag(bool _flag) { smooth_start_flag = _flag; }

  // получение текущего состояния режима плавного пуска нагрузки
  bool get_smooth_flag() { return smooth_start_flag; }

  // запуск нагрузки
  void starting_pwm()
  {
    set_pwm_data(get_start_pwm());
    if (smooth_start_flag)
    {
      smooth_start_on = true;
    }
  }

  void tick()
  {
    static uint8_t old_data_index = 0;

    if (smooth_start_on)
    {
      smooth_start_pwm();
    }
    else if (old_data_index != pwm_data_index)
    {
      old_data_index = pwm_data_index;
      analogWrite(pwm_pin, pwm_data_table[pwm_data_index]);
      if (pwm_data_index > 0)
      {
        EEPROM.update(eeprom_index, pwm_data_index);
      }
      LOG_PRINT("pwm_data: ");
      LOG_PRINTLN(pwm_data_table[pwm_data_index]);
      EEPROM.update(eeprom_index + 1, (bool)pwm_data_index);
    }
    set_led_state();
  }
};

// ===================================================

uint8_t get_start_pwm()
{
  uint8_t x = EEPROM.read(eeprom_index);
  if (x >= 5 || x == 0)
  // если в памяти записано некорректное значение, устанавливаем минимальный уровень ШИМ
  {
    x = 1;
    EEPROM.update(eeprom_index, x);
  }
  return (x);
}

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

// таблица управления нагревателем, линейная зависимость: например 39%, 59%, 78% и 98%
// uint8_t const pwm_data_table[] = {100, 150, 200, 250};
// таблица управления светодиодной лентой, нелинейная зависимость: например 12%, 24%, 49% и 98%
uint8_t const pwm_data_table[] = {30, 60, 125, 255};

#ifdef USE_TWO_BUTTONS
// закольцевать перебор значений ШИМ кнопками
constexpr bool LOOP_DATA_OF_PWM = false;

#endif

// плавный запуск ШИМ
constexpr bool SMOOTH_START = true;

// ===================================================

constexpr uint16_t EEPROM_INDEX = 10;

// ============================================================================

// настройка пинов для подключения кнопок, светодиодов и выхода ШИМ для Arduino
#if __ARDUINO__
constexpr uint8_t LED_1_PIN = 7;
constexpr uint8_t LED_2_PIN = 6;
constexpr uint8_t LED_3_PIN = 5;
constexpr uint8_t LED_4_PIN = 4;
constexpr uint8_t LED_OFF_PIN = 8;

constexpr uint8_t PWM_OUT_PIN = 10;

constexpr uint8_t BTN_UP_PIN = 3;
#ifdef USE_TWO_BUTTONS
constexpr uint8_t BTN_DOWN_PIN = 2;
#endif

// настройка пинов для подключения кнопок, светодиодов и выхода ШИМ для Attiny45/85
#elif __DIGISPARK__
constexpr uint8_t LED_1_PIN = 1;
constexpr uint8_t LED_OFF_PIN = 0;

constexpr uint8_t PWM_OUT_PIN = 4;

constexpr uint8_t BTN_UP_PIN = 2;
#ifdef USE_TWO_BUTTONS
constexpr uint8_t BTN_DOWN_PIN = 3;
#endif

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

// получение и валидация сохраненного в EEPROM текущего значения ШИМ, возвращает индекс значения в массиве pwm_data_table
uint8_t get_start_pwm();

// опрос кнопок
void check_button();

// ===================================================

class PWM_Controller
{
private:
  uint8_t pwm_data_index = 0;
  uint8_t pwm_pin = PWM_OUT_PIN;
  uint8_t led_1 = LED_1_PIN;
  bool pwm_on_flag = false;
  bool smooth_start_on = false;   // если true, значит в данный момент происходит плавный запуск нагрузки
  bool smooth_start_flag = false; // если true, используется плавный запуск нагрузки (например, для плавного запуска источника света), иначе нагрузка сразу стартует с заданной мощностью
#if __ARDUINO__
  uint8_t led_2 = LED_2_PIN;
  uint8_t led_3 = LED_3_PIN;
  uint8_t led_4 = LED_4_PIN;
#endif
  uint8_t led_off = LED_OFF_PIN;

  void set_led_state();
  void smooth_start_pwm();

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
  void set_pwm_data(uint8_t _data);
  // получение текущего значения ШИМ (возвращает индекс значения в массиве pwm_data_table)
  uint8_t get_pwm_data();

  bool get_pwm_on_flag();

  void power_on_off(bool on_flag);

  // включение/выключение режима плавного пуска нагрузки
  void set_smooth_flag(bool _flag);

  // получение текущего состояния режима плавного пуска нагрузки
  bool get_smooth_flag();

  // запуск нагрузки
  void starting_pwm();

  void tick();
};

// ==== private ======================================

void PWM_Controller::smooth_start_pwm()
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

void PWM_Controller::set_led_state()
{
  static uint32_t timer = millis();

  // состояние светодиодов обновляется 20 раз в секунду
  if (millis() - timer >= 50)
  {
    timer += 50;
    digitalWrite(led_off, !pwm_on_flag);
    digitalWrite(led_1, (pwm_data_index >= 0 && pwm_on_flag));
#if __ARDUINO__
    digitalWrite(led_2, (pwm_data_index >= 1 && pwm_on_flag));
    digitalWrite(led_3, (pwm_data_index >= 2 && pwm_on_flag));
    digitalWrite(led_4, (pwm_data_index == 3 && pwm_on_flag));
#endif
  }
}

// ==== public =======================================

void PWM_Controller::set_pwm_data(uint8_t _data) { pwm_data_index = _data; }
// получение текущего значения ШИМ (возвращает индекс значения в массиве pwm_data_table)
uint8_t PWM_Controller::get_pwm_data() { return pwm_data_index; }

bool PWM_Controller::get_pwm_on_flag() { return pwm_on_flag; }

void PWM_Controller::power_on_off(bool on_flag)
{
  pwm_on_flag = on_flag;
  EEPROM.update(EEPROM_INDEX + 1, (uint8_t)pwm_on_flag);
}

void PWM_Controller::set_smooth_flag(bool _flag) { smooth_start_flag = _flag; }

// получение текущего состояния режима плавного пуска нагрузки
bool PWM_Controller::get_smooth_flag() { return smooth_start_flag; }

void PWM_Controller::starting_pwm()
{
  set_pwm_data(get_start_pwm());
  power_on_off(true);
  if (smooth_start_flag)
  {
    smooth_start_on = true;
  }
}

void PWM_Controller::tick()
{
  static uint8_t old_data_index = 0;
  static bool old_pwm_on_flag = false;

  set_led_state();

  if (smooth_start_on)
  {
    smooth_start_pwm();
    return;
  }

  if (old_pwm_on_flag != pwm_on_flag)
  {
    old_pwm_on_flag = pwm_on_flag;

    if (!pwm_on_flag)
    {
      analogWrite(pwm_pin, 0);
      LOG_PRINTLN("power OFF");
    }
    else
    {
      analogWrite(pwm_pin, pwm_data_table[pwm_data_index]);
      LOG_PRINTLN("power ON");
      LOG_PRINT("pwm_data: ");
      LOG_PRINTLN(pwm_data_table[pwm_data_index]);
    }
    return;
  }

  if (old_data_index != pwm_data_index)
  {
    old_data_index = pwm_data_index;
    EEPROM.update(EEPROM_INDEX, pwm_data_index);
    if (pwm_on_flag)
    {
      analogWrite(pwm_pin, pwm_data_table[pwm_data_index]);
      LOG_PRINT("pwm_data: ");
      LOG_PRINTLN(pwm_data_table[pwm_data_index]);
    }
  }
}

// ===================================================

uint8_t get_start_pwm()
{
  uint8_t x = EEPROM.read(EEPROM_INDEX);
  if (x >= sizeof(pwm_data_table))
  // если в памяти записано некорректное значение, устанавливаем минимальный уровень ШИМ
  {
    x = 0;
    EEPROM.update(EEPROM_INDEX, x);
  }
  return (x);
}

/**
 * @file pwm_controller_2.ino
 * @author Vladimir Shatalov (valesh-soft@yandex.ru)
 * @brief Управление нагрузкой с помощью ШИМ;
 *
 * Контроллеры:
 *   - прибор может быть собран как на классической Arduino с применением
 *   контроллеров Atmega168/328, так и на контроллерах Attiny45/85, например
 *   Digispark;
 *   - при использовании контроллеров Attiny45/85 в Arduino IDE используется
 *   пакет- ATTinyCore  http://drazzy.com/package_drazzy.com_index.json
 *   - пины для подключения кнопок, светодиодов и вывода ШИМ определяются
 *   в блоках __ARDUINO__ и __DIGISPARK__
 *
 * Управление:
 *   - ШИМ-контроллер имеет четыре уровня мощности; величины заполнения ШИМ
 *   задаются в массиве pwm_data_table; для примера имеются два варианта
 *   массива: с линейной зависимостью - для управления, например, нагревателем,
 *   и с нелинейной зависимостью - для управления, например, светодиодной
 *   лентой; нелинейность связана с нелинейным восприятием уровней освещенности
 *   зрением человека;
 *   - изменение уровня может выполняться как одной кнопкой (btn_up), так и
 *   двумя (btn_up + btn_down); перебор уровней закольцован;
 *   - удержание любой кнопки нажатой в течение 1 секунды отключает нагрузку,
 *   включение - клик любой кнопкой; при включении восстанавливается последний
 *   уровень мощности;
 *
 * Индикация:
 *   - текущий уровень мощности индицируется линейкой из четырех светодиодов
 *   (для Arduino) или одним (для Attiny45/85);
 *   - первый светодиод линейки двухцветный, красный цвет зажигается, если
 *   нагрузка отключена;
 *
 * Дополнительно:
 *   - при использовании Arduino для уменьшения размера кода можно отключить
 *   вывод отладочной информации в Serial, для этого нужно закомментировать
 *   строку #define LOG_ON; для Attiny45/85 Serial в любом случае не 
 *   используется;
 *
 * @version 1.6
 * @date 25.01.2024
 *
 * @copyright Copyright (c) 2023-2024
 *
 */
#define __ARDUINO__ defined(__AVR_ATmega328P__) || defined(__AVR_ATmega328PB__) || defined(__AVR_ATmega328__) || defined(__AVR_ATmega168__) || defined(__AVR_ATmega168P__)
#define __DIGISPARK__ defined(__AVR_ATtiny85__) || defined(__AVR_ATtiny45__)

#include <shButton.h>
#include <EEPROM.h>

#if __ARDUINO__

#define LOG_ON

#endif

const uint16_t eeprom_index = 10;

// ==== Таблица данных для заполнения ШИМ в зависимости от уровня мощности ====

// таблица управления нагревателем, линейная зависимость: примерно 39%, 59%, 78% и 98%
// uint8_t pwm_data_table[] = {0, 100, 150, 200, 250};

// таблица управления светодиодной лентой, нелинейная зависимость: примерно 12%, 24%, 49% и 98%
uint8_t pwm_data_table[] = {0, 30, 60, 125, 250};

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
#if __ARDUINO__
  uint8_t led_2 = led_2_pin;
  uint8_t led_3 = led_3_pin;
  uint8_t led_4 = led_4_pin;
#endif
  uint8_t led_off = led_off_pin;

  void set_led_state()
  {
    digitalWrite(led_off, (pwm_data_index == 0));
    digitalWrite(led_1, (pwm_data_index >= 1));
#if __ARDUINO__
    digitalWrite(led_2, (pwm_data_index >= 2));
    digitalWrite(led_3, (pwm_data_index >= 3));
    digitalWrite(led_4, (pwm_data_index == 4));
#endif
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

  void set_pwm_data(uint8_t _data) { pwm_data_index = _data; }

  uint8_t get_pwm_data() { return pwm_data_index; }

  void tick()
  {
    static uint8_t old_data_index = 0;

    if (old_data_index != pwm_data_index)
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

shButton btn_up(btn_up_pin);
shButton btn_down(btn_down_pin);

PWM_Controller pwm_controller;

// ===================================================

// получение и валидация сохраненного в EEPROM значения ШИМ
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

// опрос кнопки
void check_button()
{
  uint8_t x = pwm_controller.get_pwm_data();

  btn_up.getButtonState();
  btn_down.getButtonState();

  // при удержании кнопки в течение одной секунды нагрузка отключается или включается
  if (btn_up.getLastState() == BTN_LONGCLICK || btn_down.getLastState() == BTN_LONGCLICK)
  {
    if (x == 0)
    {
      // если ШИМ отключен, запустить нагрузку с сохраненным значением
      x = get_start_pwm();
    }
    else
    {
      // иначе отключить нагрузку
      x = 0;
    }
    pwm_controller.set_pwm_data(x);
  }
  // при однократном клике мощность нагрузки изменяется на один пункт по кругу в сторону, определенную нажатой кнопкой
  else if (btn_up.getLastState() == BTN_ONECLICK || btn_down.getLastState() == BTN_ONECLICK)
  {
    if (x == 0)
    {
      // если ШИМ отключен, запустить нагрузку с сохраненным значением
      x = get_start_pwm();
    }
    else
    {
      // иначе изменить текущий уровень ШИМ
      (btn_up.getLastState() == BTN_ONECLICK) ? x++ : x--;
      if (x >= 5)
      {
        x = 1;
      }
      else if (x == 0)
      {
        x = 4;
      }
    }
    pwm_controller.set_pwm_data(x);
  }
}

void setup()
{
#ifdef LOG_ON
  Serial.begin(9600);
#endif

// увеличиваем частоту ШИМ
#if __ARDUINO__
  TCCR1B = TCCR1B & B11111000 | B00000001; // на пинах D9 и D10 (timer1) до 31372.55 Гц
#elif __DIGISPARK__
  TCCR1 = TCCR1 & B11110000 | B00000001; // на пине PB4 (timer1) до 32.2 кГц
#endif
  // настраиваем кнопки
  btn_up.setVirtualClickOn();
  btn_up.setLongClickMode(LCM_ONLYONCE);
  btn_up.setTimeoutOfLongClick(1000);
  btn_down.setVirtualClickOn();
  btn_down.setLongClickMode(LCM_ONLYONCE);
  btn_down.setTimeoutOfLongClick(1000);

  // запускаем нагрузку с сохраненным уровнем ШИМ
  if ((bool)EEPROM.read(eeprom_index + 1))
  {
    pwm_controller.set_pwm_data(get_start_pwm());
  }

  LOG_PRINTLN("Device started");
}

void loop()
{
  check_button();
  pwm_controller.tick();
}

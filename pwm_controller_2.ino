/**
 * @file pwm_controller_2.ino
 * @author Vladimir Shatalov (valesh-soft@yandex.ru)
 * @brief Управление нагрузкой с помощью ШИМ;
 *
 * @version 1.7.5
 * @date 30.05.2026
 *
 * @copyright Copyright (c) 2023-2026
 *
 */

#include <EEPROM.h>
#include "header_file.h"
#include "pc_button.h"

pcButton btn_up(BTN_UP_PIN);
#ifdef USE_TWO_BUTTONS
pcButton btn_down(BTN_DOWN_PIN);
#endif

PWM_Controller pwm_controller;

// ===================================================

void check_button()
{
  uint8_t x = pwm_controller.get_pwm_data();

  btn_up.getButtonState();
#ifdef USE_TWO_BUTTONS
  btn_down.getButtonState();

  // при удержании нажатой любой кнопки в течение одной секунды нагрузка отключается или включается
  if (btn_up.getLastState() == BTN_LONGCLICK || btn_down.getLastState() == BTN_LONGCLICK)
#else
  // при удержании нажатой кнопки в течение одной секунды нагрузка отключается или включается
  if (btn_up.getLastState() == BTN_LONGCLICK)
#endif
  {
    if (!pwm_controller.get_pwm_on_flag())
    {
      // если ШИМ отключен, запустить нагрузку с сохраненным значением
      pwm_controller.starting_pwm();
    }
    else
    {
      // иначе отключить нагрузку
      pwm_controller.power_on_off(false);
    }
  }
  // при однократном клике мощность нагрузки изменяется на один пункт в сторону, определенную нажатой кнопкой
#ifdef USE_TWO_BUTTONS
  else if (btn_up.getLastState() == BTN_ONECLICK || btn_down.getLastState() == BTN_ONECLICK)
#else
  else if (btn_up.getLastState() == BTN_ONECLICK)
#endif
  {
    if (!pwm_controller.get_pwm_on_flag())
    {
      // если ШИМ отключен, запустить нагрузку с сохраненным в последний раз значением
      pwm_controller.starting_pwm();
    }
    else
    {
      // иначе изменить текущий уровень ШИМ
      (btn_up.getLastState() == BTN_ONECLICK) ? x++ : x--;

      // при выходе за пределы диапазона устанавливаем значение в зависимости от направления изменения
      // учитывая, закольцован или нет перебор значений ШИМ
      if (x >= sizeof(pwm_data_table))
        if (btn_up.getLastState() == BTN_ONECLICK)
        {
#ifdef USE_TWO_BUTTONS
          x = (LOOP_DATA_OF_PWM) ? 0 : (sizeof(pwm_data_table) - 1);
#else
          x = 0;
#endif
        }
        else
        {
#ifdef USE_TWO_BUTTONS
          x = (LOOP_DATA_OF_PWM) ? (sizeof(pwm_data_table) - 1) : 0;
#else
          x = (sizeof(pwm_data_table) - 1);
#endif
        }
      pwm_controller.set_pwm_data(x);
    }
  }
}

void setup()
{
#if LOG_ON > 0
  Serial.begin(115200);
#endif

// увеличиваем максимальную частоту ШИМ
#if __ARDUINO__
  TCCR1B = TCCR1B & B11111000 | B00000001; // на пинах D9 и D10 (timer1) до 31372.55 Гц
#elif __DIGISPARK__
  TCCR1 = TCCR1 & B11110000 | B00000001; // на пине PB4 (timer1) до 32.2 кГц
#endif

  // запускаем нагрузку с сохраненным уровнем ШИМ
  if ((bool)EEPROM.read(EEPROM_INDEX + 1))
  {
    pwm_controller.starting_pwm();
  }

  LOG_PRINTLN("Device started");
}

void loop()
{
  check_button();
  pwm_controller.tick();
}

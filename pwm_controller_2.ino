/**
 * @file pwm_controller_2.ino
 * @author Vladimir Shatalov (valesh-soft@yandex.ru)
 * @brief Управление нагрузкой с помощью ШИМ;
 *
 * @version 1.7.3
 * @date 29.05.2026
 *
 * @copyright Copyright (c) 2023-2026
 *
 */

#include <shButton.h> // https://github.com/VAleSh-Soft/shButton
#include <EEPROM.h>
#include "header_file.h"

shButton btn_up(BTN_UP_PIN);
#ifdef USE_TWO_BUTTONS
shButton btn_down(BTN_DOWN_PIN);
#endif

PWM_Controller pwm_controller;

// ===================================================

void check_button()
{
  int8_t x = pwm_controller.get_pwm_data();

  btn_up.getButtonState();
#ifdef USE_TWO_BUTTONS
  btn_down.getButtonState();

  // при удержании любой кнопки в течение одной секунды нагрузка отключается или включается
  if (btn_up.getLastState() == BTN_LONGCLICK || btn_down.getLastState() == BTN_LONGCLICK)
#else
  // при удержании кнопки в течение одной секунды нагрузка отключается или включается
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
#ifdef LOG_ON
  Serial.begin(9600);
#endif

// увеличиваем частоту максимальную ШИМ
#if __ARDUINO__
  TCCR1B = TCCR1B & B11111000 | B00000001; // на пинах D9 и D10 (timer1) до 31372.55 Гц
#elif __DIGISPARK__
  TCCR1 = TCCR1 & B11110000 | B00000001; // на пине PB4 (timer1) до 32.2 кГц
#endif
  // настраиваем кнопки
  btn_up.setVirtualClickOn();
  btn_up.setLongClickMode(LCM_ONLYONCE);
  btn_up.setTimeoutOfLongClick(1000);
#ifdef USE_TWO_BUTTONS
  btn_down.setVirtualClickOn();
  btn_down.setLongClickMode(LCM_ONLYONCE);
  btn_down.setTimeoutOfLongClick(1000);
#endif

  pwm_controller.set_smooth_flag(SMOOTH_START);

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

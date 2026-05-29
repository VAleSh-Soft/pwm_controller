/**
 * @file pwm_controller_2.ino
 * @author Vladimir Shatalov (valesh-soft@yandex.ru)
 * @brief Управление нагрузкой с помощью ШИМ;
 *
 * @version 1.7.1
 * @date 27.03.2024
 *
 * @copyright Copyright (c) 2023-2024
 *
 */

#include <shButton.h> // https://github.com/VAleSh-Soft/shButton
#include <EEPROM.h>
#include "header_file.h"

shButton btn_up(btn_up_pin);
#ifdef USE_TWO_BUTTONS
shButton btn_down(btn_down_pin);
#endif

PWM_Controller pwm_controller;

// ===================================================

void check_button()
{
  uint8_t x = pwm_controller.get_pwm_data();

  btn_up.getButtonState();
#ifdef USE_TWO_BUTTONS
  btn_down.getButtonState();

  // при удержании кнопки в течение одной секунды нагрузка отключается или включается
  if (btn_up.getLastState() == BTN_LONGCLICK || btn_down.getLastState() == BTN_LONGCLICK)
#else
  // при удержании кнопки в течение одной секунды нагрузка отключается или включается
  if (btn_up.getLastState() == BTN_LONGCLICK)
#endif
  {
    if (x == 0)
    {
      // если ШИМ отключен, запустить нагрузку с сохраненным значением
      pwm_controller.starting_pwm();
    }
    else
    {
      // иначе отключить нагрузку
      pwm_controller.set_pwm_data(0);
    }
  }
  // при однократном клике мощность нагрузки изменяется на один пункт по кругу в сторону, определенную нажатой кнопкой
#ifdef USE_TWO_BUTTONS
  else if (btn_up.getLastState() == BTN_ONECLICK || btn_down.getLastState() == BTN_ONECLICK)
#else
  else if (btn_up.getLastState() == BTN_ONECLICK)
#endif
  {
    if (x == 0)
    {
      // если ШИМ отключен, запустить нагрузку с сохраненным значением
      pwm_controller.starting_pwm();
    }
    else
    {
      // иначе изменить текущий уровень ШИМ
      (btn_up.getLastState() == BTN_ONECLICK) ? x++ : x--;
      if (x >= sizeof(pwm_data_table))
      {
#ifdef USE_TWO_BUTTONS
        x = (loop_data_of_pwm) ? 1 : (sizeof(pwm_data_table) - 1);
#else
        x = 1;
#endif
      }
      else if (x == 0)
      {
#ifdef USE_TWO_BUTTONS
        x = (loop_data_of_pwm) ? (sizeof(pwm_data_table) - 1) : 1;
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
#ifdef USE_TWO_BUTTONS
  btn_down.setVirtualClickOn();
  btn_down.setLongClickMode(LCM_ONLYONCE);
  btn_down.setTimeoutOfLongClick(1000);
#endif

  pwm_controller.set_smooth_flag(smooth_start);

  // запускаем нагрузку с сохраненным уровнем ШИМ
  if ((bool)EEPROM.read(eeprom_index + 1))
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

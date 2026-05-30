#pragma.once

#include <shButton.h> // https://github.com/VAleSh-Soft/shButton

class pcButton : public shButton
{
public:
  pcButton(uint8_t _pin) : shButton(_pin)
  {
    shButton::setVirtualClickOn();
    shButton::setLongClickMode(LCM_ONLYONCE);
    shButton::setTimeoutOfLongClick(1000);
  }
};


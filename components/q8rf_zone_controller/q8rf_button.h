#pragma once

#include "esphome/core/component.h"
#include "esphome/components/button/button.h"
#include "q8rf_controller.h"

namespace esphome
{
  namespace q8rf
  {

    class Q8RFButton : public button::Button, public Component
    {
      static const uint8_t Q8RF_MSG_CMD_PAIR = 0x00;

    protected:
      uint16_t q8rf_device_id_ = 0;
      uint16_t q8rf_zone_id_ = 0;
      uint8_t msg_pair_[45];
      Q8RFController *controller_;

    public:
      void set_q8rf_device_id(uint16_t id);
      void set_q8rf_zone_id(uint16_t id);
      void set_controller(Q8RFController *controller);

      void press_action() override;
      void setup() override;
    };

  } // namespace q8rf
} // namespace esphome

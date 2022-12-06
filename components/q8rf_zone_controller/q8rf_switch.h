#pragma once

#include "esphome/core/component.h"
#include "esphome/components/switch/switch.h"
#include "q8rf_controller.h"

namespace esphome
{
  namespace q8rf
  {

    class Q8RFSwitch : public switch_::Switch, public PollingComponent
    {
    protected:
      uint16_t q8rf_device_id_ = 0;
      uint16_t q8rf_zone_id_ = 0;
      Q8RFController *controller_;
      uint8_t msg_heat_off_[45];
      uint8_t msg_heat_on_[45];
      bool state_ = false;
      uint8_t pending_msg_ = 0;
      unsigned long last_msg_time_ = 0;
      unsigned long last_turn_on_time_ = 0;
      bool initialized_ = false;
      uint32_t q8rf_resend_interval_ = 600000;
      uint32_t q8rf_turn_on_watchdog_interval_ = 0;

      static const uint8_t Q7RF_MSG_CMD_TURN_ON_HEATING = 0xFF;
      static const uint8_t Q7RF_MSG_CMD_TURN_OFF_HEATING = 0x0F;

    public:
      Q8RFSwitch() : PollingComponent(1000) {}
      void set_q8rf_device_id(uint16_t id);
      void set_q8rf_zone_id(uint16_t id);
      void set_q8rf_turn_on_watchdog_interval(uint32_t interval);
      void set_q8rf_resend_interval(uint32_t interval);
      void set_state(bool state);
      void set_controller(Q8RFController *controller);

      void setup() override;
      void write_state(bool state) override;
      void dump_config() override;
      void update() override;
    };

  } // namespace q8rf
} // namespace esphome

#pragma once

#include "esphome/core/component.h"
#include "esphome/components/spi/spi.h"
#include "esphome/components/api/custom_api_device.h"

#ifndef q8rf_controller_h
#define q8rf_controller_h
namespace esphome
{
  namespace q8rf
  {

    static const uint8_t MSG_NONE = 0;
    static const uint8_t MSG_HEAT_ON = 1;
    static const uint8_t MSG_HEAT_OFF = 2;
    static const uint8_t MSG_PAIR = 3;

    class Q8RFController : public api::CustomAPIDevice,
                           public Component,
                           public spi::SPIDevice<spi::BIT_ORDER_MSB_FIRST, spi::CLOCK_POLARITY_LOW, spi::CLOCK_PHASE_LEADING,
                                                 spi::DATA_RATE_1KHZ>
    {
    protected:
      bool initialized_ = false;

      uint8_t pending_msg_ = MSG_NONE;
      unsigned long last_msg_time_ = 0;
      unsigned long last_turn_on_time_ = 0;
      uint8_t msg_errors_ = 0;

    private:
      bool reset_cc();
      void send_cc_cmd(uint8_t cmd);
      void read_cc_register(uint8_t reg, uint8_t *value);
      void read_cc_config_register(uint8_t reg, uint8_t *value);
      void write_cc_register(uint8_t reg, uint8_t *value, size_t length);
      void write_cc_config_register(uint8_t reg, uint8_t value);
      bool send_cc_data(uint8_t *data, size_t length);

    public:
      void compile_msg(uint16_t device_id, uint8_t zone_id, uint8_t cmd, uint8_t *msg);
      void setup() override;
      void dump_config() override;
      bool send_msg(uint8_t *msg);
      void on_scan(int  q8rf_zone_id, int  q8rf_device_id_from, int  q8rf_device_id_to, int  interval);
    };

  } // namespace q7rf
} // namespace esphome
#endif
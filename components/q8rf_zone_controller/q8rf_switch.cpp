#include "esphome/core/log.h"
#include "q8rf_switch.h"

namespace esphome
{
    namespace q8rf
    {

        static const char *TAG = "q8rf.switch";

        unsigned long elapsed(unsigned long since, unsigned long now)
        {
            if (since > now)
            {
                // millis() overflows every ~50 days
                return (ULONG_MAX - since) + now;
            }
            else
            {
                return now - since;
            }
        }

        void Q8RFSwitch::setup()
        {
            controller_->compile_msg(this->q8rf_device_id_, this->q8rf_zone_id_, Q7RF_MSG_CMD_TURN_ON_HEATING, this->msg_heat_on_);
            controller_->compile_msg(this->q8rf_device_id_, this->q8rf_zone_id_, Q7RF_MSG_CMD_TURN_OFF_HEATING, this->msg_heat_off_);

            this->initialized_ = true;
        }

        void Q8RFSwitch::write_state(bool state)
        {

            ESP_LOGCONFIG(TAG, "write_state: %s", state ? "true" : "false");

            if (!this->initialized_)
            {
                this->setup();
            }

            if (this->initialized_)
            {

                if (state)
                {
                    ESP_LOGCONFIG(TAG, "Send message ON device: 0x%04x zone: %d ", this->q8rf_device_id_, this->q8rf_zone_id_);

                    controller_->send_msg(msg_heat_on_);
                    this->last_turn_on_time_ = millis();
                }
                else
                {
                    ESP_LOGI(TAG, "Send message OFF device: 0x%04x zone: %d ", this->q8rf_device_id_, this->q8rf_zone_id_);

                    controller_->send_msg(msg_heat_off_);
                }

                if (this->state_ != state)
                {
                    this->set_state(state);
                }
            }
            this->last_msg_time_ = millis();
        }

        void Q8RFSwitch::set_state(bool state)
        {
            this->state_ = state;
            this->publish_state(state);
        }
        void Q8RFSwitch::set_controller(Q8RFController *controller)
        {
            this->controller_ = controller;
        }

        void Q8RFSwitch::dump_config()
        {
            ESP_LOGCONFIG(TAG, "  Q8RF Device ID: 0x%04x", this->q8rf_device_id_);
            ESP_LOGCONFIG(TAG, "  Q8RF Zone ID: %d", this->q8rf_zone_id_);
        }

        void Q8RFSwitch::update()
        {
            if (this->initialized_)
            {

                unsigned long now = millis();

                if (this->state_ && this->q8rf_turn_on_watchdog_interval_ > 0 &&
                    elapsed(this->last_turn_on_time_, now) > this->q8rf_turn_on_watchdog_interval_)
                {
                    ESP_LOGD(TAG, "Turn on watch dog triggered, turning off furnace.");
                    this->set_state(false);
                    controller_->send_msg(msg_heat_off_);
                    return;
                }

                // Check if we have to resend current state by now
                if (elapsed(this->last_msg_time_, now) > this->q8rf_resend_interval_)
                {
                    ESP_LOGD(TAG, "Resending last state.");
                    this->write_state(state_);
                }
            }
        }

        void Q8RFSwitch::set_q8rf_device_id(uint16_t id) { this->q8rf_device_id_ = id; }
        void Q8RFSwitch::set_q8rf_zone_id(uint16_t id) { this->q8rf_zone_id_ = id; }
        void Q8RFSwitch::set_q8rf_turn_on_watchdog_interval(uint32_t interval) { this->q8rf_turn_on_watchdog_interval_ = interval; }
        void Q8RFSwitch::set_q8rf_resend_interval(uint32_t interval) { this->q8rf_resend_interval_ = interval; }

    } // namespace q8rf
} // namespace esphome

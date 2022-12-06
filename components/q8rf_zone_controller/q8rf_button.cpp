#include "esphome/core/log.h"
#include "q8rf_button.h"

namespace esphome
{
    namespace q8rf
    {

        static const char *TAG = "q8rf.button";

        void Q8RFButton::setup()
        {
            controller_->compile_msg(this->q8rf_device_id_, this->q8rf_zone_id_, Q8RF_MSG_CMD_PAIR, this->msg_pair_);
            this->initialized_ = true;
        }
        void Q8RFButton::press_action()
        {

            if (!this->initialized_)
            {
                this->setup();
            }

            if (this->initialized_)
            {
                ESP_LOGI(TAG, "Send message PAIR device: 0x%04x zone: %d ", this->q8rf_device_id_, this->q8rf_zone_id_);
                controller_->send_msg(msg_pair_);
            }
        }

        void Q8RFButton::set_q8rf_device_id(uint16_t id)
        {
            this->q8rf_device_id_ = id;
        }
        void Q8RFButton::set_q8rf_zone_id(uint16_t id)
        {
            this->q8rf_zone_id_ = id;
        }
        void Q8RFButton::set_controller(Q8RFController *controller)
        {
            this->controller_ = controller;
        }

        void Q8RFButton::dump_config()
        {
            ESP_LOGCONFIG(TAG, "  Q8RF Device ID: 0x%04x", this->q8rf_device_id_);
            ESP_LOGCONFIG(TAG, "  Q8RF Zone ID: %d", this->q8rf_zone_id_);
        }

    } // namespace q8rf
} // namespace esphome

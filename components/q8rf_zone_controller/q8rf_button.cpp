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
                controller_->send_msg(msg_pair_);
            }

            ESP_LOGCONFIG(TAG, "press_action");
        }

        void Q8RFButton::set_q8rf_device_id(uint16_t id) { this->q8rf_device_id_ = id; }
        void Q8RFButton::set_q8rf_zone_id(uint16_t id) { this->q8rf_zone_id_ = id; }
        void Q8RFButton::set_controller(Q8RFController *controller) { this->controller_ = controller; }

    } // namespace q8rf
} // namespace esphome

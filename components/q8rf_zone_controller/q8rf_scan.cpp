

#include "esphome/core/component.h"
#include "esphome/components/spi/spi.h"
#include "esphome/components/api/custom_api_device.h"
#include "esphome.h"
#include "q8rf_controller.h"

namespace esphome
{

  static const char *TAG = "Q8RFController.component";
  static const uint8_t Q8RF_MSG_CMD_PAIR = 0x00;

  class Q8RFScan : public api::CustomAPIDevice, public Component
  {
  public:
    q8rf::Q8RFController *controller_;

    void
    setup() override
    {
      register_service(&Q8RFScan::on_scan, "scan",
                       {"q8rf_zone_id", "q8rf_device_id_from", "q8rf_device_id_to", "interval"});
    }

    void on_scan(uint16_t q8rf_zone_id, uint16_t q8rf_device_id_from, uint16_t q8rf_device_id_to, uint16_t interval)
    {

      for (uint16_t i = q8rf_device_id_from; i < q8rf_device_id_to; i += 1)
      {

        uint8_t msg_pair[45];
        controller_->compile_msg(i, q8rf_zone_id, Q8RF_MSG_CMD_PAIR, msg_pair);
        controller_->send_msg(msg_pair);
        ESP_LOGI(TAG, "Send message PAIR device: 0x%04x zone: %d ", i, q8rf_zone_id);
        delay(interval);
      }
    }
  };
}
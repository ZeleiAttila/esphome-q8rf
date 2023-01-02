#include "esphome/core/log.h"
#include "q8rf_controller.h"
#include <stdint.h>

namespace esphome
{
  namespace q8rf
  {

    static const char *TAG = "Q8RFController.component";

    static const char *Q8RF_PREAMBLE_DATA = "111000111";
    static const char *Q8RF_ZERO_BIT_DATA = "011";
    static const char *Q8RF_ONE_BIT_DATA = "001";
    static const char *Q8RF_GAP_DATA = "000";

    static const uint8_t Q8RF_MSG_CMD_PAIR = 0x00;

    static const uint8_t CMD_SRES = 0x30;
    static const uint8_t CMD_STX = 0x35;
    static const uint8_t CMD_SIDLE = 0x36;
    static const uint8_t CMD_SFRX = 0x3a;
    static const uint8_t CMD_SFTX = 0x3b;

    static const uint8_t REG_PARTNUM = 0xf0;
    static const uint8_t REG_VERSION = 0xf1;

    static const uint8_t CREG_FIFOTHR = 0x03;
    static const uint8_t CREG_PKTLEN = 0x06;
    static const uint8_t CREG_PKTCTRL1 = 0x07;
    static const uint8_t CREG_PKTCTRL0 = 0x08;
    static const uint8_t CREG_FREQ2 = 0x0d;
    static const uint8_t CREG_FREQ1 = 0x0e;
    static const uint8_t CREG_FREQ0 = 0x0f;
    static const uint8_t CREG_MDMCFG4 = 0x10;
    static const uint8_t CREG_MDMCFG3 = 0x11;
    static const uint8_t CREG_MDMCFG2 = 0x12;
    static const uint8_t CREG_MDMCFG1 = 0x13;
    static const uint8_t CREG_MDMCFG0 = 0x14;
    static const uint8_t CREG_MCSM0 = 0x18;
    static const uint8_t CREG_FOCCFG = 0x19;
    static const uint8_t CREG_FREND0 = 0x22;

    static const uint8_t SREG_MARCSTATE = 0xf5;

    static const uint8_t CREG_PATABLE_BURST_WRITE = 0x7e;
    static const uint8_t CREG_PATABLE_BURST_READ = 0xfe;
    static const uint8_t CFIFO_TX_BURST = 0x7f;

    static const uint8_t MARCSTATE_TX = 0x13;
    static const uint8_t MARCSTATE_TX_END = 0x14;
    static const uint8_t MARCSTATE_RXTX_SWITCH = 0x15;

    static const uint8_t MSG_SEND_ERRORS_RESET_LIMIT = 3;

    /* Each symbol takes 220us. Computherm/Delta Q7RF uses PWM modulation.
       Every data bit is encoded as 3 bit inside the buffer.
       001 = 1, 011 = 0, 111000111 = preamble */
    static const uint8_t Q8RF_REG_CONFIG[] = {
        CREG_FIFOTHR, 0x00,  // TX FIFO length = 61, others default
        CREG_PKTLEN, 0x3d,   // 61 byte packets
        CREG_PKTCTRL1, 0x00, // Disable RSSI/LQ payload sending, no address check
        CREG_PKTCTRL0, 0x01, // variable packet length, no CRC calculation
        CREG_FREQ2, 0x21,    // FREQ2, FREQ=0x216544 => 2.188.612 * (26 MHz OSC / 2^16) ~= 868.285 MHz
        CREG_FREQ1, 0x65,    // ^FREQ1
        CREG_FREQ0, 0x44,    // ^FREQ0
        CREG_MDMCFG4, 0xf7,  // baud exponent = 7 (lower 4 bits)
        CREG_MDMCFG3, 0x6B,  // baud mantissa = 107 -> (((256+107) * 2^7)/2^28) * 26 MHz OSC = 4.5 kBaud
        CREG_MDMCFG2, 0x30,  // DC filter on, ASK/OOK modulation, no manchester coding, no preamble/sync
        CREG_MDMCFG1, 0x00,  // no FEC, channel spacing exponent = 0 (last two bit)
        CREG_MDMCFG0, 0xf8,  // channel spacing mantissa = 248 -> 6.000.000 / 2^18 * (256 + 248) * 2^0 ~= 50kHz
        CREG_MCSM0, 0x10,    // autocalibrate synthesizer when switching from IDLE to RX/TX state
        CREG_FOCCFG, 0x00,   // ASK/OOK has no frequency offset compensation
        CREG_FREND0, 0x11    // ASK/OOK PATABLE (power level) settings = up to index 1
    };

    // 0xc0 = +12dB max power setting
    static const uint8_t Q8RF_PA_TABLE[] = {0x00, 0xc0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

    void encode_bits(uint16_t byte, uint8_t pad_to_length, char **dest)
    {
      char binary[9];
      itoa(byte, binary, 2);
      int binary_len = strlen(binary);

      if (binary_len < pad_to_length)
      {
        for (int p = 0; p < pad_to_length - binary_len; p++)
        {
          strncpy(*dest, Q8RF_ZERO_BIT_DATA, strlen(Q8RF_ZERO_BIT_DATA));
          *dest += strlen(Q8RF_ZERO_BIT_DATA);
        }
      }

      for (int b = 0; b < binary_len; b++)
      {
        strncpy(*dest, binary[b] == '1' ? Q8RF_ONE_BIT_DATA : Q8RF_ZERO_BIT_DATA, strlen(Q8RF_ONE_BIT_DATA));
        *dest += strlen(Q8RF_ZERO_BIT_DATA);
      }
    }

    void Q8RFController::compile_msg(uint16_t device_id, uint8_t zone_id, uint8_t cmd, uint8_t *msg)
    {

      char binary_msg[360];
      char *cursor = binary_msg;

      // Preamble
      char *preamble_start = cursor;
      strncpy(cursor, Q8RF_PREAMBLE_DATA, strlen(Q8RF_PREAMBLE_DATA));
      cursor += strlen(Q8RF_PREAMBLE_DATA);

      char *payload_start = cursor;

      switch (zone_id)
      {
      case 1:
        zone_id = 8;
        break;
      case 2:
        zone_id = 4;
        break;
      case 3:
        zone_id = 12;
        break;
      case 4:
        zone_id = 2;
        break;
      }

      // Command
      encode_bits(device_id, 16, &cursor);
      encode_bits(zone_id, 4, &cursor);
      encode_bits(cmd, 8, &cursor);

      // Repeat the command once more
      strncpy(cursor, payload_start, cursor - payload_start);
      cursor += cursor - payload_start;

      // Add a gap
      strncpy(cursor, Q8RF_GAP_DATA, strlen(Q8RF_GAP_DATA));
      cursor += strlen(Q8RF_GAP_DATA);

      // Repeat the whole burst
      strncpy(cursor, preamble_start, cursor - preamble_start);
      cursor += cursor - preamble_start;

      // Convert msg to bytes
      cursor = binary_msg; // Reset cursor
      uint8_t *cursor_msg = msg;
      char binary_byte[9];
      binary_byte[8] = '\0';
      for (int b = 0; b < 45; b++)
      {
        strncpy(binary_byte, cursor, 8);
        cursor += 8;
        *cursor_msg = strtoul(binary_byte, 0, 2);
        cursor_msg++;
      }

#if ESPHOME_LOG_LEVEL >= ESPHOME_LOG_LEVEL_VERBOSE
      // Assemble debug print
      char debug[91];
      cursor = debug;
      cursor_msg = msg;
      for (int b = 0; b < 45; b++)
      {
        sprintf(cursor, "%x", *cursor_msg);
        cursor += 2;
        cursor_msg++;
      }
      ESP_LOGV(TAG, "Encoded msg: 0x%02x as 0x%s", cmd, debug);
#endif
    }

    bool Q8RFController::reset_cc()
    {
      // Chip reset sequence. CS wiggle (CC1101 manual page 45)
      this->disable();
      delayMicroseconds(5);
      this->enable();
      delayMicroseconds(10);
      this->disable();
      delayMicroseconds(41);

      this->send_cc_cmd(CMD_SRES);
      ESP_LOGD(TAG, "Issued CC1101 reset sequence.");

      // Read part number and version
      uint8_t partnum;
      this->read_cc_register(REG_PARTNUM, &partnum);

      uint8_t version;
      this->read_cc_register(REG_VERSION, &version);

      ESP_LOGI(TAG, "CC1101 found with partnum: %02x and version: %02x", partnum, version);

      // Setup config registers
      uint8_t verify_value;
      for (int i = 0; i < sizeof(Q8RF_REG_CONFIG); i += 2)
      {
        this->write_cc_config_register(Q8RF_REG_CONFIG[i], Q8RF_REG_CONFIG[i + 1]);
        this->read_cc_config_register(Q8RF_REG_CONFIG[i], &verify_value);
        if (verify_value != Q8RF_REG_CONFIG[i + 1])
        {
          ESP_LOGE(TAG, "Failed to write CC1101 config register. reg: %02x write: %02x read: %02x", Q8RF_REG_CONFIG[i],
                   Q8RF_REG_CONFIG[i + 1], verify_value);
          return false;
        }
#if ESPHOME_LOG_LEVEL >= ESPHOME_LOG_LEVEL_VERBOSE
        ESP_LOGV(TAG, "Written CC1101 config register. reg: %02x value: %02x", Q8RF_REG_CONFIG[i], Q8RF_REG_CONFIG[i + 1]);
#endif
      }

      // Write PATable
      uint8_t pa_table[sizeof(Q8RF_PA_TABLE)];
      for (int i = 0; i < sizeof(Q8RF_PA_TABLE); i++)
        pa_table[i] = Q8RF_PA_TABLE[i];

      this->enable();
      this->write_byte(CREG_PATABLE_BURST_WRITE);
      this->write_array(pa_table, sizeof(Q8RF_PA_TABLE));
      this->disable();

      this->enable();
      this->write_byte(CREG_PATABLE_BURST_READ);
      this->read_array(pa_table, sizeof(Q8RF_PA_TABLE));
      this->disable();

      for (int i = 0; i < sizeof(Q8RF_PA_TABLE); i++)
      {
        if (pa_table[i] != Q8RF_PA_TABLE[i])
        {
          ESP_LOGE(TAG, "Failed to write CC1101 PATABLE.");
          return false;
        }
#if ESPHOME_LOG_LEVEL >= ESPHOME_LOG_LEVEL_VERBOSE
        ESP_LOGV(TAG, "Written CC1101 PATABLE[%d]: %02x", i, Q8RF_PA_TABLE[i]);
#endif
      }

      return true;
    }

    void Q8RFController::send_cc_cmd(uint8_t cmd)
    {
      this->enable();
      this->transfer_byte(cmd);
      this->disable();
    }

    void Q8RFController::read_cc_register(uint8_t reg, uint8_t *value)
    {
      this->enable();
      this->transfer_byte(reg);
      *value = this->transfer_byte(0);
      this->disable();
    }

    void Q8RFController::read_cc_config_register(uint8_t reg, uint8_t *value) { this->read_cc_register(reg + 0x80, value); }

    void Q8RFController::write_cc_register(uint8_t reg, uint8_t *value, size_t length)
    {
      this->enable();
      this->transfer_byte(reg);
      this->transfer_array(value, length);
      this->disable();
    }

    void Q8RFController::write_cc_config_register(uint8_t reg, uint8_t value)
    {
      uint8_t arr[1] = {value};
      this->write_cc_register(reg, arr, 1);
    }

    bool Q8RFController::send_cc_data(uint8_t *data, size_t length)
    {
      uint8_t buffer[length];
      for (int i = 0; i < length; i++)
      {
        buffer[i] = *data;
        data++;
      }
      this->send_cc_cmd(CMD_SIDLE);
      this->send_cc_cmd(CMD_SFRX);
      this->send_cc_cmd(CMD_SFTX);

      this->enable();
      this->write_byte(CFIFO_TX_BURST);
      this->write_array(buffer, length);
      this->disable();

      this->send_cc_cmd(CMD_STX);

      uint8_t state;
      this->read_cc_register(SREG_MARCSTATE, &state);
      state &= 0x1f;
      if (state != MARCSTATE_TX && state != MARCSTATE_TX_END && state != MARCSTATE_RXTX_SWITCH)
      {
        ESP_LOGE(TAG, "CC1101 in invalid state after sending, returning to idle. State: 0x%02x", state);
        this->send_cc_cmd(CMD_SIDLE);
        return false;
      }

      return true;
    }

    bool Q8RFController::send_msg(uint8_t *msg)
    {
      bool result = false;
      const char *text_msg = NULL;

      result = this->send_cc_data(msg, 45);

      if (result)
      {
        this->last_msg_time_ = millis();
      }
      else
      {
        this->msg_errors_++;
        if (this->msg_errors_ >= MSG_SEND_ERRORS_RESET_LIMIT)
        {
          ESP_LOGE(TAG, "Multiple message send errors occured, forcing CC1101 reset.");
          this->reset_cc();
          this->msg_errors_ = 0;
        }
      }

      return result;
    }

    void Q8RFController::setup()
    {
      this->spi_setup();
      if (this->reset_cc())
      {
        ESP_LOGI(TAG, "CC1101 initialized.");
      }
      else
      {
        ESP_LOGE(TAG, "Failed to reset CC1101 modem. Check connection.");
        return;
      }

      register_service(&Q8RFController::scan, "scan",
                       {"q8rf_zone_id",
                        "q8rf_device_id_from",
                        "q8rf_device_id_to",
                        "interval"});

      this->initialized_ = true;
    }

    void Q8RFController::dump_config()
    {
      ESP_LOGCONFIG(TAG, "Q8RF:");
      LOG_PIN("CC1101 CS Pin: ", this->cs_);
    }

    void Q8RFController::scan(uint16_t q8rf_zone_id, uint16_t q8rf_device_id_from, uint16_t q8rf_device_id_to, uint16_t interval)
    {

      for (uint16_t i = q8rf_device_id_from; i < q8rf_device_id_to; i += 1)
      {
        uint8_t msg_pair[45];
        this->compile_msg(i, q8rf_zone_id, Q8RF_MSG_CMD_PAIR, msg_pair);
        this->send_msg(msg_pair);
        ESP_LOGI(TAG, "Send message PAIR device: 0x%04x zone: %d ", i, q8rf_zone_id);
        delay(interval);
      }
    }

  } // namespace q7rf
} // namespace esphome

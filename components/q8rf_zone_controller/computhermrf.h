/*
RF Sender and receiver library for Computherm Q8RF wireless thermostat

HW requirements:
- RF Transmitter: HopeRF RFM117W-868S1
- RF Receiver: HopeRF RFM217W-868S1

Note1: please be aware that the mentioned RF modules are not 5V ones.
Note2: maybe other receiver/transmitter modiel are also fine but a few important things needed: 868.35 MHz frequency, ASK OOK modulation, simple 1 pin interface.

Modulation details:

1 tick: 220 us (ideally)

SYNC: (3 x low,) 3 x high, 3 x low, 3 x high
    ___     ___
___|   |___|   |
SYNC length supported: tick x 2.5 ... tick x 3.5 = 550 ... 770 us

0: 2 x low, 1 x high
   _
__| |
0 pulse length: tick x 0.5 ... tick x 1.5 = 110 ... 330 us

1: 1 x low, 2 x high
  __
_|  |
1 pulse length: tick x 1.5 ... tick x 2.5 = 330 ... 550 us

STOP: 6 x low (and after that comes the 3 x low of th next SYNC)

________
STOP length: tick x 7 ... tick x 10 = 1760 ... 2200 us

*/
#ifndef ComputhermRF_h
#define ComputhermRF_h
#include "esphome/core/component.h"
#include "esphome/components/spi/spi.h"
#include "esphome/components/api/custom_api_device.h"

namespace esphome
{
    namespace q8rf
    {

        typedef struct
        {
            // String address;
            // String command;
            unsigned long addr;
            bool on;
        } computhermMessage;

        class ComputhermRF
        {
        public:
            const std::string VERSION = "0.1.3";
            std::vector<uint8_t> bits;
          std::string dest;
            ComputhermRF();
            ComputhermRF(uint8_t inputPin, uint8_t outputPin);
            void setPins(uint8_t inputPin, uint8_t outputPin);
            void startReceiver();
            void stopReceiver();
            bool isDataAvailable();
            // void getData(String &id, bool &on);
            computhermMessage getData();
            // void sendMessage(computhermMessage message);
            void sendMessage(unsigned long address, bool on);
            void pairAddress(unsigned long address);

        private:
            static const uint16_t _TICK_LENGTH;
            static const uint16_t _SHORT_MIN;
            static const uint16_t _LONG_MIN;
            static const uint16_t _SYNC_MIN;
            static const uint16_t _SYNC_MAX;
            static const uint16_t _STOP_MIN;
            static const uint16_t _STOP_MAX;
            static const uint16_t _BUFF_SIZE;
            static const uint16_t _MSG_LENGTH;
            static uint8_t _inputPin;
            static uint8_t _outputPin;
            static volatile bool _avail;
            static volatile uint8_t _buff[];
            static volatile uint8_t _buffEnd;
            static uint8_t _lastBuff[];
            static uint32_t _lastMessageArrived;
            void _wakeUpTransmitter();
            void _sendPulse(uint8_t lowTime, uint8_t highTime);
            void _sendStop();
            void _sendSync();
            void _sendBit(bool bit);
            // void _sendHalfByte(char ch);
            void _sendHalfByte(uint8_t num);
            static bool _isRepeat();
            // char _toHex(uint8_t num);
            // void _sendMessage(String address, bool on, bool normal_padding);
            void _sendMessage(unsigned long address, bool on, bool normal_padding);
        };
    }
}
#endif
// Copyright 2019 denxhun

#include "computhermrf.h"
namespace esphome
{
    namespace q8rf
    {
        const uint16_t ComputhermRF::_TICK_LENGTH = 220;
        const uint16_t ComputhermRF::_SHORT_MIN = _TICK_LENGTH * 0.5; // 110
        const uint16_t ComputhermRF::_LONG_MIN = _TICK_LENGTH * 1.5;  // 330
        const uint16_t ComputhermRF::_SYNC_MIN = _TICK_LENGTH * 2.5;  // 550
        const uint16_t ComputhermRF::_SYNC_MAX = _TICK_LENGTH * 3.5;  // 770
        const uint16_t ComputhermRF::_STOP_MIN = _TICK_LENGTH * 7;    // 1760
        const uint16_t ComputhermRF::_STOP_MAX = _TICK_LENGTH * 10;   // 2200
        const uint16_t ComputhermRF::_BUFF_SIZE = 70;
        const uint16_t ComputhermRF::_MSG_LENGTH = 56;

        uint8_t ComputhermRF::_inputPin;
        uint8_t ComputhermRF::_outputPin;
        volatile bool ComputhermRF::_avail;
        volatile uint8_t ComputhermRF::_buff[_BUFF_SIZE];
        volatile uint8_t ComputhermRF::_buffEnd;
        uint8_t ComputhermRF::_lastBuff[_BUFF_SIZE];
        uint32_t ComputhermRF::_lastMessageArrived;

        ComputhermRF::ComputhermRF()
        {
            ComputhermRF(255, 255);
        }

        ComputhermRF::ComputhermRF(uint8_t inputPin, uint8_t outputPin)
        {
            setPins(inputPin, outputPin);
        }

        void ComputhermRF::setPins(uint8_t inputPin, uint8_t outputPin)
        {
            stopReceiver();
            _inputPin = inputPin;
            _outputPin = outputPin;
            if (_outputPin < 255)
            {
            }
        }

        void ComputhermRF::startReceiver()
        {
            if (_inputPin < 255)
            {
                _avail = false;
                _buffEnd = 0;
                _lastMessageArrived = 0;
            }
        }
        void ComputhermRF::stopReceiver()
        {
        }
        bool ComputhermRF::isDataAvailable()
        {
            return _avail;
        }
        // void ComputhermRF::getData(String &id, bool &on) {
        //     computhermMessage result = getData();
        //     id = result.address;
        //     on = (result.command == "ON");
        // }
        computhermMessage ComputhermRF::getData()
        {
            computhermMessage result;

            result.addr = 0;
            // String a = "     ";
            uint8_t n = 0;
            for (int i = 0; i < 5; i++)
            {
                if (i)
                    result.addr <<= 4;
                for (int j = 0; j < 4; j++)
                {
                    for (int j = 0; j < 4; j++)
                    {
                        if (_buff[i * 4 + j] == 1)
                        {
                            n |= 1 << (3 - j);
                        }
                    }
                    // a[i] = _toHex(n);
                    result.addr |= n & 0xF;
                    n = 0;
                }
            }
            // result.address = a;
            // String c = "";
            if (_buff[20] == 0 && _buff[21] == 0 && _buff[22] == 0 && _buff[23] == 0)
            {
                // c = "ON";
                result.on = true;
            }
            else
            {
                // c = "OFF";
                result.on = false;
            }
            // result.command = c;
            _avail = false;
            return result;
        }
        // void ComputhermRF::sendMessage(computhermMessage message) {
        //   sendMessage(message.address, message.command == "ON");
        // }
        void ComputhermRF::sendMessage(unsigned long address, bool on)
        {
            _sendMessage(address, on, true);
        }
        // void ComputhermRF::_sendMessage(String address, bool on, bool normal_padding) {
        //   if (address.length() != 5)
        //     return;
        //   //!! _sendMessage(address_numeric, on, normal_padding);
        // }
        void ComputhermRF::_sendMessage(unsigned long address, bool on, bool normal_padding)
        {
            _wakeUpTransmitter();
            stopReceiver();
            for (int i = 0; i < 8; i++)
            {
                _sendSync();
                for (int j = 0; j < 2; j++)
                {
                    // 16+8 bits
                    // CHECK if address last halfbyte is 8?
                    for (int k = 0; k < 5; k++)
                    {
                        // 0x56789 -> 5,6,7,8,9
                        uint8_t hb = (address >> (5 - 1 - k) * 4) & 0xF;
                        // _sendHalfByte(address[k]);
                        _sendHalfByte(hb);
                    }

                    // COMMAND: 4+4 bits
                    //(CClib: TURN_ON_HEATING = 0xFF) --> "00" to send
                    //(CClib:TURN_OFF_HEATING = 0x0F) --> "F0" to send
                    //(CClib:PAIR = 0x00) --> "FF" to send
                    if (on)
                    {
                        //_sendHalfByte('0');  // ON
                        _sendHalfByte(0x0); // ON
                    }
                    else
                    {
                        // _sendHalfByte('F');  // OFF
                        _sendHalfByte(0xF); // OFF
                    }
                    if (normal_padding)
                    {
                        // _sendHalfByte('0');  // default padding for ON/OFF - ON
                        _sendHalfByte(0x0); // default padding for ON/OFF - ON
                    }
                    else
                    {
                        // _sendHalfByte('F');  // padding for pairing - OFF
                        _sendHalfByte(0xF); // padding for pairing - OFF
                    }
                }
                _sendStop();
            }
            startReceiver();
        }
        void ComputhermRF::pairAddress(unsigned long address)
        {
            // send FF for pairing
            _sendMessage(address, false, false);
        }

        /*
        In order to make the receiver recognize the transmitter, we need to execute the pairing process.

        Go to Home Assistant's Developer tools → Services and select the service esphome.<node_name>_<switch_name>.pair. Press and hold the M/A button on the receiver until it starts flashing green. Now press Call service in the Services page. The receiver should stop flashing, and the pairing is now complete. The receiver should react now if you try toggling the associated Home Assistant UI switch.

        If you wish to reset and use your original wireless thermostat, once again set the receiver into learning mode with the M/A button, then hold the SET + DAY button on your wireless thermostat until the blinking stops. The receiver only listens to the device currently paired.
        */

        void ComputhermRF::_wakeUpTransmitter()
        {
            //  digitalWrite(_outputPin, HIGH);
            // delayMicroseconds(100);
            // digitalWrite(_outputPin, LOW);
        }
        void ComputhermRF::_sendPulse(uint8_t lowTime, uint8_t highTime)
        {
            // digitalWrite(_outputPin, LOW);
            // delayMicroseconds(lowTime * _TICK_LENGTH);
            // if (highTime > 0) {
            //  digitalWrite(_outputPin, HIGH);
            //  delayMicroseconds(highTime * _TICK_LENGTH);
            // digitalWrite(_outputPin, LOW);
            //}
        }
        void ComputhermRF::_sendStop()
        {
            _sendPulse(6, 0); // 000 000
        }
        void ComputhermRF::_sendSync()
        {
            _sendPulse(3, 3); // 000 111
            _sendPulse(3, 3); // 000 111 //??
        }
        void ComputhermRF::_sendBit(bool bit)
        {
            char *TAG = "Q8RFController.component";
            if (bit)
            {
                std::string bit = "011";
                dest.append(bit);
            }
            else
            {
                std::string bit = "001";
                dest.append(bit);
            }
        }
        // void ComputhermRF::_sendHalfByte(char ch) {
        //   uint8_t num = 0;
        //   if (ch >= '0' && ch <= '9') {
        //     num = ch - '0';
        //   } else {
        //     if (ch >= 'a' && ch <= 'f') {
        //       num = ch - 'a' + 10;
        //     } else {
        //       if (ch >= 'A' && ch <= 'F') {
        //         num = ch - 'A' + 10;
        //       } else {
        //         return;
        //       }
        //     }
        //   }
        //   _sendHalfByte(num);
        // }

        void ComputhermRF::_sendHalfByte(uint8_t num)
        {
            for (int i = 0; i < 4; i++)
            {
                _sendBit(num & 1 << (3 - i));
            }
        }

        bool ComputhermRF::_isRepeat()
        {
            bool result = false;
            return result;
        }
    }
}
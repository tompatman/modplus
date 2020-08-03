
#pragma once

#include <string>
#include <vector>


namespace util {

typedef enum {
    _RS232
    , _RS485
    , _NONE

} ePortConfigType;

// forward declaration
struct tSerialPort {

    tSerialPort()
    {
      port_config_type = ePortConfigType::_NONE;
    }

    char deviceName[100];
    long int baud;
    char parity;
    int data_bit;
    int stop_bit;
    ePortConfigType port_config_type;
};

void initSerialDeviceStruct(tSerialPort *serDev,
                            const std::string &deviceName, int baudRate,
                            int dataBits, char parity, int stopBits, ePortConfigType config_type);

std::vector<std::string> tokenData(const std::string strInData, const std::string strToken);

}

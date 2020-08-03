
#include <string.h>
#include "Util.h"
#include "Poco/Exception.h"

namespace util {

void initSerialDeviceStruct(tSerialPort *serDev,
                            const std::string &deviceName, int baudRate,
                            int dataBits, char parity, int stopBits, ePortConfigType config_type)
{
    memset(serDev->deviceName, 0, sizeof(serDev->deviceName));
    strncpy(serDev->deviceName, deviceName.c_str(), sizeof(serDev->deviceName));

    serDev->baud = baudRate;
    //5, 6, 7, or 8 are valid values
    serDev->data_bit = dataBits;
    if (serDev->data_bit < 5 || serDev->data_bit > 8)
    {
        throw Poco::InvalidArgumentException("Pump device databits must be 5, 6, 7 or 8");
    }
    //N, E, O are valid values
    serDev->parity = parity;
    if (serDev->parity != 'N' && serDev->parity != 'E' && serDev->parity != 'O')
    {
        throw Poco::InvalidArgumentException("Pump device parity must be N, E, or O");
    }
    //1 or 2 are valid values
    serDev->stop_bit = stopBits;
    if (serDev->stop_bit < 1 || serDev->stop_bit > 2)
    {
        throw Poco::InvalidArgumentException("Pump device stop bits must be 1 or 2");
    }

  serDev->port_config_type = config_type;
}

std::vector<std::string> tokenData(const std::string strInData, const std::string strToken)
{
  //std::string strResponse = inData->convertStr();
  std::vector<std::string> vecValues;

  //Parse into data pairs
  char *charTok = strtok((char *) strInData.c_str(), strToken.c_str());
  //inVal = Poco::NumberParser::parseFloat(strDelimitOut);

  while (charTok != NULL)
  {
    vecValues.push_back(std::string(charTok));
    charTok = strtok(NULL, strToken.c_str());

  }

  return (vecValues);
}

}

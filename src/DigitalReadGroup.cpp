
#include "include/DigitalReadGroup.h"

namespace modplus {
DigitalReadGroup::DigitalReadGroup(uint8_t *dataStore, const Poco::UInt16 numRegisters)
        :
        _dataStore(dataStore)
{

  _numRegisters = numRegisters;
}

DigitalReadGroup::~DigitalReadGroup()
{

}

Poco::Int16 DigitalReadGroup::readData(const Poco::UInt16 uiNumRegisters, const Poco::UInt32 uiStartAddress, modbus_t *const ctx)
{

  if (_dataStore == NULL)
  {
    return (-1);
  }
  return (modbus_read_input_bits(ctx, uiStartAddress, uiNumRegisters, &_dataStore[uiStartAddress]));

}

};
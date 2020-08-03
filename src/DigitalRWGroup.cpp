
#include "include/DigitalRWGroup.h"

namespace modplus {
DigitalRWGroup::DigitalRWGroup(uint8_t *dataStore, const Poco::UInt16 numRegisters)
        :
        _dataStore(dataStore)
{

  _numRegisters = numRegisters;
}

DigitalRWGroup::~DigitalRWGroup()
{

}

Poco::Int16 DigitalRWGroup::readData(const Poco::UInt16 uiNumRegisters, const Poco::UInt32 uiStartAddress, modbus_t *const ctx)
{

  if (_dataStore == NULL)
  {
    return (-1);
  }
  return (modbus_read_bits(ctx, uiStartAddress, uiNumRegisters, &_dataStore[uiStartAddress]));

}

};
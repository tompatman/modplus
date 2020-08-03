
#include "include/AnalogRWGroup.h"

namespace modplus {
AnalogRWGroup::AnalogRWGroup(uint16_t *dataStore, const Poco::UInt16 numRegisters)
        :
        _dataStore(dataStore)
{

  _numRegisters = numRegisters;
}

AnalogRWGroup::~AnalogRWGroup()
{

}

Poco::Int16 AnalogRWGroup::readData(const Poco::UInt16 uiNumRegisters, const Poco::UInt32 uiStartAddress, modbus_t *const ctx)
{

  if (_dataStore == NULL)
  {
    return (-1);
  }
  return (modbus_read_registers(ctx, uiStartAddress, uiNumRegisters, &_dataStore[uiStartAddress]));

}

};
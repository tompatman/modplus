
#include "include/AnalogReadGroup.h"

namespace modplus {

AnalogReadGroup::AnalogReadGroup(uint16_t *dataStore, const Poco::UInt16 numRegisters)
        :
        _dataStore(dataStore)
{

  _numRegisters = numRegisters;
}

AnalogReadGroup::~AnalogReadGroup()
{

}

Poco::Int16 AnalogReadGroup::readData(const Poco::UInt16 uiNumRegisters, const Poco::UInt32 uiStartAddress, modbus_t *const ctx)
{

  if (_dataStore == NULL)
  {
    return (-1);
  }

  return (modbus_read_input_registers(ctx, uiStartAddress, uiNumRegisters, &_dataStore[uiStartAddress]));

}

};
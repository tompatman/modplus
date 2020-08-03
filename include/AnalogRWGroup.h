#pragma once

#include "IRegGroup.h"
#include <modbus/modbus.h>
#include <Poco/Types.h>

namespace modplus {

class AnalogRWGroup
        : public IRegGroup {
public:
    AnalogRWGroup(uint16_t *dataStore, const Poco::UInt16 numRegisters);

    ~AnalogRWGroup() override;

    //Virtual implementations
    Poco::Int16 readData(const Poco::UInt16 uiNumRegisters, const Poco::UInt32 uiStartAddress, modbus_t *const ctx) override;

private:
    uint16_t *_dataStore;
};

};


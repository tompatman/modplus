#pragma once

#include "IRegGroup.h"
#include <modbus/modbus.h>
#include <Poco/Types.h>

namespace modplus {

class DigitalRWGroup
        : public IRegGroup {
public:
    DigitalRWGroup(uint8_t *dataStore, const Poco::UInt16 numRegisters);

    ~DigitalRWGroup() override;

    //Virtual implementations
    Poco::Int16 readData(const Poco::UInt16 uiNumRegisters, const Poco::UInt32 uiStartAddress, modbus_t *const ctx) override;

private:
    uint8_t *_dataStore;
};

}
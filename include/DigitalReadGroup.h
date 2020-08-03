
#pragma once

#include "IRegGroup.h"
#include <modbus/modbus.h>
#include <Poco/Types.h>

namespace modplus {

class DigitalReadGroup
        : public IRegGroup {
public:
    DigitalReadGroup(uint8_t *dataStore, const Poco::UInt16 numRegisters);

    ~DigitalReadGroup() override;

    //Virtual implementations
    Poco::Int16 readData(const Poco::UInt16 uiNumRegisters, const Poco::UInt32 uiStartAddress, modbus_t *const ctx) override;

private:
    uint8_t *_dataStore;
};

}
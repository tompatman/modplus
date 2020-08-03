
#pragma once

#include <modbus/modbus.h>
#include <Poco/Types.h>


namespace modplus {
//Interface for polling data from each modbus register group (read digital, rw digital, r analog, rw analog)
class IRegGroup {
public:

    virtual ~IRegGroup()
    {};

    virtual Poco::Int16 readData(const Poco::UInt16 uiNumRegisters, const Poco::UInt32 uiStartAddress, modbus_t *const ctx) = 0;

    Poco::UInt16 getNumRegisters()
    { return (_numRegisters); };

protected:
    Poco::UInt16 _numRegisters = 0;
};

};
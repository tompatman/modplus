
#pragma once

#include "MBRegContainer.h"
#include <Poco/Types.h>

namespace modplus {

template<typename regIDs>
class IMBControllerProc {
public:

    virtual MBRegContainerV2<regIDs> &getRegContainer() = 0;

    virtual void openConnection() = 0;

    virtual void closeConnection() = 0;

    virtual bool isConnected() = 0;

    virtual bool isCommunicating() = 0;

    virtual int getFd() = 0;

    //Write a message to the specified address
    virtual bool writeAnalog(uint16_t *const data, const Poco::UInt16 uiNumRegisters, const Poco::UInt32 mbDataAddress) = 0;

    virtual bool writeDigital(uint8_t *const data, const Poco::UInt16 uiNumRegisters, const Poco::UInt32 uiAddress) = 0;

    //virtual implemetations
    virtual void process() = 0;

    virtual void readRegisterBlock(const modplus::RegGroup::eRegGroup groupType, const Poco::UInt32 startAddress) = 0;

    /**
     *
     * @param groupType The modbus register set to read
     * @param startAddress The starting address to read in the group type
     * @param numRegisters The number of registers to read
     */
    virtual void readRegisterBlock(const modplus::RegGroup::eRegGroup groupType, const Poco::UInt32 startAddress, Poco::UInt16 numRegisters) = 0;

    virtual int getModbusRefreshCount(const modplus::RegGroup::eRegGroup groupType) = 0;

    virtual void incrModbusRefreshCount(const modplus::RegGroup::eRegGroup groupType) = 0;

    /**
     *
     * @param slaveID the target slave identifier. important when communicating with devices on modbus RTU.
     * @return
     */
    virtual int setModbusSlave(int slaveID) = 0;


};

}


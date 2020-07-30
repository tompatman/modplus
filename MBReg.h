/*
 * MBReg.h
 *
 *  Created on: Oct 16, 2013
 *      Author: Administrator
 */

#pragma once

#include "Poco/Types.h"
#include "Poco/Mutex.h"
#include "stdint.h"

namespace PbModbus {

//Holds the standard sunspec string type
typedef struct {
    char value[32] = {0};
} tString;

namespace RegGroup {
typedef enum {
    Invalid = -1
    , AnalogRW
    , AnalogR
    , DigitalRW
    , DigitalR
    , IODescRegister
    , Num
} eRegGroup;
}

template<typename regIDs, typename valType>
class MBReg {

public:
    //The ID is an identifier used internally by the application usually from an enum list
    //pInData: Points to location where read register data is stored
    //pOutData: Points to location where write register data is stored
    MBReg(const regIDs regID, const Poco::UInt32 uiDataAddress, valType *pData, Poco::Mutex &mut, valType defaultValue,
          const RegGroup::eRegGroup groupType = RegGroup::AnalogRW)
            :
            _mut(mut)
            , _groupType(groupType)
            , isScaledType(false)
    {
      _regID = regID;
      _pData = pData;
      *_pData = defaultValue;
      _uiDataAddress = uiDataAddress;
      _bNewData = false;
    };

    virtual ~MBReg()
    {};

    //Override this method when additional action is required, such as sending a message on a queue
    //Then call this base method
    virtual void setValue(valType newData)
    {
      _mut.lock();
      *_pData = newData;
      _bNewData = true;
      _mut.unlock();
    };

    //When the modbus library automatically updates the memory location with new data
    virtual void setValue()
    {
      _mut.lock();
      _bNewData = true;
      _mut.unlock();
    };

    //Can be used to re-initialize a register after a connection breaks;
    virtual void setDefault()
    {};

    //Override this method when additional action is required, such as aggregating additional data prior to providing result.
    virtual valType getValue()
    {
      _bNewData = false;
        if (isScaledValue()) {
            Poco::InvalidAccessException("MBReg::getValue() called on a scaled type.");
        }
      return (*_pData);
    };

    bool isScaledValue()
    { return isScaledType; };

    virtual void setScaledValue(double newData)
    {
      Poco::InvalidAccessException("MBReg::setScaledValue() called on a non-scaled type.");
    }

    virtual double getScaledValue()
    {
      Poco::InvalidAccessException("MBReg::getScaledValue() called on a non-scaled type.");

      return (-1);
    }

    virtual void updateScaledValue()
    {
      Poco::InvalidAccessException("MBReg::updateScaledValue() called on a non-scaled type.");
    }

    virtual double *getScaledDataPointer()
    {
      Poco::InvalidAccessException("MBReg::getScaledReference() called on a non-scaled type.");

      return (nullptr);
    }

    virtual valType *getDataPointer()
    { return _pData; };

    virtual valType &getDataReference()
    { return *_pData; };

    Poco::UInt16 getRegisterAddress()
    { return (_uiDataAddress); };

    RegGroup::eRegGroup getGroupType()
    { return (_groupType); };

    bool isNewData()
    { return (_bNewData); };

    regIDs getRegID()
    { return (_regID); };

public:

    bool _bNewData;
    regIDs _regID;
    valType *_pData;
    Poco::UInt32 _uiDataAddress;
    Poco::Mutex &_mut;
    const RegGroup::eRegGroup _groupType;

protected:
    bool isScaledType;

};

}


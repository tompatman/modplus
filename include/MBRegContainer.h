/*
 * MBRegContainerV2.h
 *
 *  Created on: Aug 24, 2015
 *      Author: service
 */

#pragma once

#include "MBReg.h"
#include <Poco/NumberFormatter.h>
#include <vector>
#include <modbus/modbus.h>
#include <type_traits>
#include "util/IEnumPrinter.h"
#include <map>
#include <util/IEnumPrinter.h>
#include <iostream>

/* This holds vectors of various modbus message types and is essentially the modbus map for
 * an application (master or slave)
 */
namespace modplus {


typedef enum {
    RegType_Invalid = -1
    , RegType_UInt8
    , RegType_UInt16
    , RegType_UInt32
    , RegType_UInt64
    , RegType_Str
    , RegType_FloatSingle
    , RegType_Num
} eRegType;

template<typename ModbusEnumId>
class tMap {
public:
    tMap(ModbusEnumId _id, eRegType _type, Poco::UInt64 _idx, Poco::UInt64 _address)
            :
            id(_id)
            , type(_type)
            , idx(_idx)
            , address(_address)
    {

    }

    /// Need a default constructor to support use with std::map
    tMap()
            :
            id((ModbusEnumId) (-1))
            , type(eRegType::RegType_Invalid)
            , idx(0)
            , address(0)
    {

    }


    ModbusEnumId id;
    eRegType type;
    Poco::UInt64 idx;
    Poco::UInt64 address;

};

//using namespace std::placeholders;  // For `_1` below

template<typename ModbusEnumId>
class MBRegContainer {
public:
    MBRegContainer(const Poco::UInt32 anaReadStartAdd, const Poco::UInt32 numAnaReadAddresses, const Poco::UInt32 anaRWStartAdd, const Poco::UInt32 numAnaRWAddresses,
                   const Poco::UInt32 digReadStartAdd, const Poco::UInt32 numDigitalReadAddresses, const Poco::UInt32 digRWStartAdd, const Poco::UInt32 numDigitalRWAddresses,
                   const Poco::UInt64 numRegIds, util::IEnumPrinter<ModbusEnumId> &enumPrint)
            : _anaReadStartAdd(anaReadStartAdd)
              , _anaRWStartAdd(anaRWStartAdd)
              , _digReadStartAdd(digReadStartAdd)
              , _digRWStartAdd(digRWStartAdd)
              , _numTotalRegs(numAnaReadAddresses + numAnaRWAddresses + numDigitalReadAddresses + numDigitalRWAddresses)
              , _numRegIds(numRegIds)
              , _enumPrint(enumPrint)
    {
      _mbMapping = modbus_mapping_new(numDigitalReadAddresses, numDigitalRWAddresses, numAnaRWAddresses, numAnaReadAddresses);
      if (_mbMapping == nullptr) {
        throw Poco::IOException("Failed to allocate modbus mapping: " + std::string(modbus_strerror(errno)));
      }

    };

    ~MBRegContainer()
    {

      _eMap.clear();
      _addrMap.clear();

      regUI8.clear();
      regUI16.clear();

      regUI32.clear();
      regUI64.clear();

      regStr.clear();

      regFloat.clear();

      modbus_mapping_free(_mbMapping);
    };

    modbus_mapping_t & getModbusMap()
    {
      return(*_mbMapping);
    }

    char *getMBDataPointer(const modplus::RegGroup::eRegGroup type, const Poco::UInt32 regAddress)
    {
      Poco::UInt64 numRegisters = 0;

      ///First validate that a modbus register address has been allocated in memory
      switch (type)
      {
        case modplus::RegGroup::DigitalR:
          numRegisters = _mbMapping->nb_input_bits;
          break;
        case modplus::RegGroup::DigitalRW:
          numRegisters = _mbMapping->nb_bits;
          break;
        case modplus::RegGroup::AnalogR:
          numRegisters = _mbMapping->nb_input_registers;
          break;
        case modplus::RegGroup::AnalogRW:
          numRegisters = _mbMapping->nb_registers;
          break;
        default:
          throw Poco::InvalidArgumentException("Invalid register group");
      }

      if (regAddress >= numRegisters)
      {
        throw Poco::InvalidAccessException("No modbus register memory mapping for type " + Poco::NumberFormatter::format(type) + " address " +
                                           Poco::NumberFormatter::format(regAddress) + " >= " + Poco::NumberFormatter::format(numRegisters) + ".");
      }

      switch( type )
      {
        case modplus::RegGroup::DigitalR:
          return ((char *) &_mbMapping->tab_input_bits[regAddress]);
        case modplus::RegGroup::DigitalRW:
          return ((char *) &_mbMapping->tab_bits[regAddress]);
        case modplus::RegGroup::AnalogR:
          return ((char *) &_mbMapping->tab_input_registers[regAddress]);
        case modplus::RegGroup::AnalogRW:
          return ((char *) &_mbMapping->tab_registers[regAddress]);
        default:
          throw Poco::InvalidArgumentException("Invalid register group");
      }

      return(nullptr);
    }

    void addUInt8(const ModbusEnumId regID, const Poco::UInt64 uiRegAddress, Poco::UInt8 defaultValue, const modplus::RegGroup::eRegGroup groupType = modplus::RegGroup::AnalogRW)
    {
      validateRegIdForAdd(regID);

      _eMap[regID] = tMap<ModbusEnumId>(regID, RegType_UInt8, regUI8.size(), uiRegAddress);
      _addrMap[uiRegAddress] = regID;

      regUI8[regID] = std::unique_ptr<modplus::MBReg<ModbusEnumId, Poco::UInt8> >(
              new modplus::MBReg<ModbusEnumId, Poco::UInt8>(regID, uiRegAddress, (Poco::UInt8 *) getMBDataPointer(groupType, uiRegAddress),
                                                            _mut, defaultValue));
    };

    void addUInt16(const ModbusEnumId regID, const Poco::UInt64 uiRegAddress, Poco::UInt16 defaultValue, const modplus::RegGroup::eRegGroup groupType = modplus::RegGroup::AnalogRW)
    {
      validateRegIdForAdd(regID);

      _eMap[regID] = tMap<ModbusEnumId>(regID, RegType_UInt16, regUI16.size(), uiRegAddress);
      _addrMap[uiRegAddress] = regID;

      regUI16[regID] = std::unique_ptr<modplus::MBReg<ModbusEnumId, Poco::UInt16> >(
              new modplus::MBReg<ModbusEnumId, Poco::UInt16>(regID, uiRegAddress, (Poco::UInt16 *) getMBDataPointer(groupType, uiRegAddress),
                                                             _mut, defaultValue));
    };

    void addUInt16(Poco::UInt16 ui16Default, std::unique_ptr<modplus::MBReg<ModbusEnumId, Poco::UInt16>> newReg)
    {
      validateRegIdForAdd(newReg->getRegID());
      _eMap[newReg->getRegID()] = tMap<ModbusEnumId>(newReg->getRegID(), RegType_UInt16, regUI16.size(), newReg->getRegisterAddress());
      _addrMap[newReg->getRegisterAddress()] = newReg->getRegID();

      regUI16[newReg->getRegID()] = std::move(newReg); /** std::unique_ptr<PbModbus::MBReg<ModbusEnumId, Poco::UInt16> >(
              new PbModbus::MBReg<ModbusEnumId, Poco::UInt16>(newReg->getRegID(), convertRegAddToDataAdd(newReg->getGroupType(), newReg->getRegisterAddress()),
                      (Poco::UInt16 *) getMBDataPointer(newReg->getGroupType(), newReg->getRegisterAddress()), _mut, ui16Default)); */

    };

    void
    addUInt32(const ModbusEnumId regID, const Poco::UInt64 uiRegAddress, Poco::UInt32 defaultValue, const modplus::RegGroup::eRegGroup groupType = modplus::RegGroup::AnalogRW)
    {
      validateRegIdForAdd(regID);

      _eMap[regID] = tMap<ModbusEnumId>(regID, RegType_UInt32, regUI32.size(), uiRegAddress);
      _addrMap[uiRegAddress] = regID;

      regUI32[regID] = std::unique_ptr<modplus::MBReg<ModbusEnumId, Poco::UInt32> >(
              new modplus::MBReg<ModbusEnumId, Poco::UInt32>(regID, uiRegAddress, (Poco::UInt32 *) getMBDataPointer(groupType, uiRegAddress),
                                                             _mut, defaultValue));
    };

    void addUInt32(Poco::UInt32 ui32Default, std::unique_ptr<modplus::MBReg<ModbusEnumId, Poco::UInt32>> newReg)
    {
      validateRegIdForAdd(newReg->getRegID());
      _eMap[newReg->getRegID()] = tMap<ModbusEnumId>(newReg->getRegID(), RegType_UInt32, regUI32.size(), newReg->getRegisterAddress());
      _addrMap[newReg->getRegisterAddress()] = newReg->getRegID();

      regUI32[newReg->getRegID()] = std::move(newReg); /** std::unique_ptr<PbModbus::MBReg<ModbusEnumId, Poco::UInt16> >(
              new PbModbus::MBReg<ModbusEnumId, Poco::UInt16>(newReg->getRegID(), convertRegAddToDataAdd(newReg->getGroupType(), newReg->getRegisterAddress()),
                      (Poco::UInt16 *) getMBDataPointer(newReg->getGroupType(), newReg->getRegisterAddress()), _mut, ui16Default)); */

    };

    void
    addUInt64(const ModbusEnumId regID, const Poco::UInt64 uiRegAddress, Poco::UInt64 defaultValue, const modplus::RegGroup::eRegGroup groupType = modplus::RegGroup::AnalogRW)
    {
      validateRegIdForAdd(regID);

      _eMap[regID] = tMap<ModbusEnumId>(regID, RegType_UInt64, regUI64.size(), uiRegAddress);
      _addrMap[uiRegAddress] = regID;

      regUI64[regID] = std::unique_ptr<modplus::MBReg<ModbusEnumId, Poco::UInt64> >(
              new modplus::MBReg<ModbusEnumId, Poco::UInt64>(regID, uiRegAddress, (Poco::UInt64 *) getMBDataPointer(groupType, uiRegAddress),
                                                             _mut, defaultValue));
    };

    /**
     * A single precision float is stored in the 32 bit register group, the float data must be converted.
     * @param regID
     * @param uiRegAddress
     * @param defaultValue
     * @param groupType
     */
    void addFloatSingle(const ModbusEnumId regID, const Poco::UInt64 uiRegAddress, float defaultValue, const modplus::RegGroup::eRegGroup groupType = modplus::RegGroup::AnalogRW)
    {
      validateRegIdForAdd(regID);

      _eMap[regID] = tMap<ModbusEnumId>(regID, RegType_FloatSingle, regFloat.size(), uiRegAddress);
      _addrMap[uiRegAddress] = regID;

      Poco::UInt32 uiDefault;
      memcpy(&uiDefault, &defaultValue, sizeof(uiDefault));
      regFloat[regID] = std::unique_ptr<modplus::MBReg<ModbusEnumId, float> >(
              new modplus::MBReg<ModbusEnumId, float>(regID, uiRegAddress, (float *) getMBDataPointer(groupType, uiRegAddress),
                                                      _mut, uiDefault));
    };

    void addString(const ModbusEnumId regID, const Poco::UInt64 uiRegAddress, const std::string strDefault)
    {
      modplus::tString tstrDefault;

      if (strDefault.length() > sizeof(tstrDefault.value)) throw Poco::InvalidArgumentException("Default string " + strDefault + " is too large!");
      strncpy(tstrDefault.value, strDefault.c_str(), sizeof(tstrDefault.value));

      validateRegIdForAdd(regID);

      _eMap[regID] = tMap<ModbusEnumId>(regID, RegType_Str, regStr.size(), uiRegAddress);
      _addrMap[uiRegAddress] = regID;

      regStr[regID] = std::unique_ptr<modplus::MBReg<ModbusEnumId, modplus::tString> >(
              new modplus::MBReg<ModbusEnumId, modplus::tString>(regID, uiRegAddress,
                                                                 (modplus::tString *) getMBDataPointer(modplus::RegGroup::AnalogRW, uiRegAddress), _mut, tstrDefault));

    };

    Poco::UInt16 getTotalBlockRegisters()
    { return (regUI8.size() + regUI16.size() + regUI32.size() + regUI64.size() + regStr.size() + regFloat.size()); };

    Poco::UInt16 getTotalAnalogRWRegisters()
    { return (_mbMapping->nb_registers); };

    Poco::UInt16 getTotalAnalogReadRegisters()
    { return (_mbMapping->nb_input_registers); };

    Poco::UInt16 getTotalDigitalRWRegisters()
    { return (_mbMapping->nb_bits); };

    Poco::UInt16 getTotalDigitalReadRegisters()
    { return (_mbMapping->nb_input_bits); };

    void setDefaults()
    {

      for (auto &mbReg : regUI8)
      {
        mbReg.setDefault();
      }

      for (auto &mbReg : regUI16)
      {
        mbReg.setDefault();
      }

      for (auto &mbReg : regUI32)
      {
        mbReg.setDefault();
      }

      for (auto &mbReg : regUI64)
      {
        mbReg.setDefault();
      }

      for (auto &mbReg : regFloat)
      {
        mbReg.setDefault();
      }

    };

    eRegType getRegTypeFromMbRegId(const ModbusEnumId regId)
    {
      if (regId >= 0 && regId <= _highestAddedId)
        return _eMap[regId].type;
      else
        return eRegType::RegType_Invalid;
    }

    eRegType getRegTypeFromAddress(const Poco::UInt64 uiAddress)
    {
      //Find which vector contains the address
      auto it = _addrMap.find(uiAddress);
      if (it == _addrMap.end())
      {
        throw Poco::NotFoundException("Could not find register ID associated with address " + Poco::NumberFormatter::format(uiAddress));
      }

      ModbusEnumId eId = it->second;


      return (_eMap[eId].type);

    };

    ModbusEnumId getRegIdFromAddress(const Poco::UInt64 &uiAddress)
    {
      auto it = _addrMap.find(uiAddress);
      if (it == _addrMap.end())
      {
        ///Don't throw an exception, this could be called frequently with an address search where some address are on a byte boundary with now matching register ID.
        return ((ModbusEnumId) (-1));
      }

      return(it->second);
    }

    Poco::UInt64 getRegisterAddressFromRegID(const ModbusEnumId regId)
    {
      auto it = _eMap.find(regId);
      if (it == _eMap.end())
      {
        throw Poco::NotFoundException("Could not find register ID mapping for " + Poco::NumberFormatter::format(regId));
      }

      return (it->second.address);
    }

    //If bThrow is true an exception will be thrown instead of returning nullptr
    modplus::MBReg<ModbusEnumId, Poco::UInt8> &getReg8(const ModbusEnumId regEnum, const bool bThrow = false)
    {

      //Poco::Int16 listIdx =
      if (_eMap[regEnum].idx >= 0 &&
          _eMap[regEnum].type == RegType_UInt8 && regUI8.size() > _eMap[regEnum].idx)
      {
        return (regUI8[_eMap[regEnum].idx]);
      }

      if (bThrow)
      {
        throw Poco::NotFoundException("Register " + Poco::NumberFormatter::format(regEnum) + " not found.");
      }

      return (nullptr);
    }


    //If bThrow is true an exception will be thrown instead of returning nullptr
    modplus::MBReg<ModbusEnumId, Poco::UInt16> &getReg16(const ModbusEnumId regEnum)
    {
      auto mbReg = regUI16.find(regEnum);
      if (mbReg == regUI16.end()) throw Poco::InvalidAccessException("Invalid register ID " + Poco::NumberFormatter::format(regEnum) + " for regUI16");

      return (*mbReg->second);
    }

    modplus::MBReg<ModbusEnumId, Poco::UInt32> &getReg32(const ModbusEnumId regEnum)
    {
      auto mbReg = regUI32.find(regEnum);
      if (mbReg == regUI32.end()) throw Poco::InvalidAccessException("Invalid register ID " + Poco::NumberFormatter::format(regEnum) + " for regUI32");

      return (*mbReg->second);
    }

    modplus::MBReg<ModbusEnumId, Poco::UInt64> &getReg64(const ModbusEnumId regEnum)
    {
      auto mbReg = regUI64.find(regEnum);
      if (mbReg == regUI64.end()) throw Poco::InvalidAccessException("Invalid register ID " + Poco::NumberFormatter::format(regEnum) + " for regUI64");

      return (*mbReg->second);
    }

    modplus::MBReg<ModbusEnumId, float> &getRegFloat(const ModbusEnumId regEnum)
    {
      auto mbReg = regFloat.find(regEnum);
      if (mbReg == regFloat.end()) throw Poco::InvalidAccessException("Invalid register ID " + Poco::NumberFormatter::format(regEnum) + " for regFloat");

      return (*mbReg->second);
    }

    modplus::MBReg<ModbusEnumId, modplus::tString> &getRegStr(const ModbusEnumId regEnum, const bool bThrow = false)
    {

      auto mbReg = regStr.find(regEnum);
      if (mbReg == regStr.end()) throw Poco::InvalidAccessException("Invalid register ID " + Poco::NumberFormatter::format(regEnum) + " for regStr");

      return (*mbReg->second);
    }


    //If bThrow is true an exception will be thrown instead of returning nullptr
    Poco::UInt8 &getReg8Ref(const ModbusEnumId regEnum)
    {
      auto mbReg = regUI8.find(regEnum);
      if (mbReg == regUI8.end()) throw Poco::InvalidAccessException("Invalid register ID " + Poco::NumberFormatter::format(regEnum) + " for regUI8");

      return (mbReg->second->getDataReference());
    }

    //If bThrow is true an exception will be thrown instead of returning nullptr
    Poco::UInt16 &getReg16Ref(const ModbusEnumId regEnum)
    {

      auto mbReg = regUI16.find(regEnum);
      if (mbReg == regUI16.end()) throw Poco::InvalidAccessException("Invalid register ID " + Poco::NumberFormatter::format(regEnum) + " for regUI16");

      return (mbReg->second->getDataReference());
    }

    Poco::UInt32 &getReg32Ref(const ModbusEnumId regEnum)
    {
      auto mbReg = regUI32.find(regEnum);
      if (mbReg == regUI32.end()) throw Poco::InvalidAccessException("Invalid register ID " + Poco::NumberFormatter::format(regEnum) + " for regUI32");

      return (mbReg->second->getDataReference());
    }

    Poco::UInt64 &getReg64Ref(const ModbusEnumId regEnum)
    {
      auto mbReg = regUI64.find(regEnum);
      if (mbReg == regUI64.end()) throw Poco::InvalidAccessException("Invalid register ID " + Poco::NumberFormatter::format(regEnum) + " for regUI64");

      return (mbReg->second->getDataReference());
    }

    float &getRegFloatRef(const ModbusEnumId regEnum)
    {
      auto mbReg = regFloat.find(regEnum);
      if (mbReg == regFloat.end() || _eMap[regEnum].type != RegType_FloatSingle)
        throw Poco::InvalidAccessException("Invalid register ID " + Poco::NumberFormatter::format(regEnum) + " for regFloat");

      //memcpy(&refVal, mbReg->second->getReference(), sizeof(refVal));

      //Cast as a float * then convert to actual value for reference return
      return (mbReg->second->getDataReference());
    }

    void getRegStrRef(const ModbusEnumId regEnum, modplus::tString &refVal)
    {
      auto mbReg = regStr.find(regEnum);
      if (mbReg == regStr.end()) throw Poco::InvalidAccessException("Invalid register ID " + Poco::NumberFormatter::format(regEnum) + " for regStr");

      refVal = mbReg->second->getDataReference();
    }

    uint16_t *getAnalogRWDataStore()
    { return (_mbMapping->tab_registers); };

    uint16_t *getAnalogReadDataStore()
    { return (_mbMapping->tab_input_registers); };

    uint8_t *getDigitalRWDataStore()
    { return (_mbMapping->tab_bits); };

    uint8_t *getDigitalReadDataStore()
    { return (_mbMapping->tab_input_bits); };

    Poco::UInt32 getAnalogRWStartAddress()
    { return (_anaRWStartAdd); };

    Poco::UInt32 getAnalogReadStartAddress()
    { return (_anaReadStartAdd); };

    Poco::UInt32 getDigitalRWStartAddress()
    { return (_digRWStartAdd); };

    Poco::UInt32 getDigitalReadStartAddress()
    { return (_digReadStartAdd); };

    Poco::UInt64 getLastRegId()
    {
      return (_numRegIds);
    }

    /// These maps use pointers because the registers contain references and can't easily be copied (see copy constructors)
    std::map<ModbusEnumId, std::unique_ptr<modplus::MBReg<ModbusEnumId, Poco::UInt8> > > regUI8;
    std::map<ModbusEnumId, std::unique_ptr<modplus::MBReg<ModbusEnumId, Poco::UInt16> > > regUI16;
    std::map<ModbusEnumId, std::unique_ptr<modplus::MBReg<ModbusEnumId, Poco::UInt32> > > regUI32;
    std::map<ModbusEnumId, std::unique_ptr<modplus::MBReg<ModbusEnumId, Poco::UInt64> > > regUI64;
    std::map<ModbusEnumId, std::unique_ptr<modplus::MBReg<ModbusEnumId, float> > > regFloat;
    std::map<ModbusEnumId, std::unique_ptr<modplus::MBReg<ModbusEnumId, modplus::tString> > > regStr;

private :
    /**
     * Create two maps. One is keyed to enumerated modbus id and references the information structure, including address.
     * The other is keyed to the non group type based address (Coil/Register numbers, not the data addresses for each group type
     */
    std::map<ModbusEnumId, tMap<ModbusEnumId> > _eMap;
    std::map<Poco::UInt64, ModbusEnumId> _addrMap;

    ModbusEnumId _lastAddedId = (ModbusEnumId) -1;
    ModbusEnumId _highestAddedId = (ModbusEnumId) -1;
    modbus_mapping_t *_mbMapping = nullptr;
    const Poco::UInt32 _anaReadStartAdd;
    const Poco::UInt32 _anaRWStartAdd;
    const Poco::UInt32 _digReadStartAdd;
    const Poco::UInt32 _digRWStartAdd;
    const Poco::UInt32 _numTotalRegs;
    const Poco::UInt64 _numRegIds;

    /// Common mutex to use for all modbus register access
    Poco::Mutex _mut;
public:
    Poco::Mutex &getMut()
    {
      return _mut;
    }

private:


    util::IEnumPrinter<ModbusEnumId> &_enumPrint;

public:
    util::IEnumPrinter<ModbusEnumId> &getEnumPrinter() const
    {
      return _enumPrint;
    }

private:

    void validateRegIdForAdd(const ModbusEnumId regID)
    {
      if (_eMap.find(regID) != _eMap.end())
      {
        throw Poco::InvalidAccessException("Register " + Poco::NumberFormatter::format(regID) + " already added!");
      }

      if (regID != (_lastAddedId + 1))
      {
        std::cout << "New register id " + Poco::NumberFormatter::format(regID) + " is not in order.";
      }
      _lastAddedId = regID;
      _highestAddedId = _lastAddedId > _highestAddedId ? _lastAddedId : _highestAddedId;

    }

    void validateRegIdForRead(const ModbusEnumId regID)
    {
      if (_eMap[regID].id == (ModbusEnumId) (-1))
      {
        throw Poco::InvalidAccessException("Register " + Poco::NumberFormatter::format(regID) + " does not exist!");
      }
    }

};
}


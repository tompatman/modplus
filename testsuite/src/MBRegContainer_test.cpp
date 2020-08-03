/*
 * MBRegContainer_test.cpp
 *
 *  Created on: Aug 12, 2015
 *      Author: service
 */

#include "MBReg.h"
#include "MBRegContainer.h"
#include "MBTestReg.h"

using namespace modplus;

#include <gmock/gmock.h>



//zmq::context_t zmq_context(5);
static const Poco::UInt16 TEST_REG16_ADDRESS = 0x1122;
static const Poco::UInt32 TEST_REG32_ADDRESS = 0x3344;
static const Poco::UInt16 TEST_REGSTR_ADDRESS = 0x5566;

static const Poco::UInt16 TEST_UI16_VALUE = 15;
static const Poco::UInt32 TEST_UI32_VALUE = 15;
static const std::string TEST_STR_VALUE = "testString";


////////////////////////////////////////////////////////////////////////////////////////////////
// Workaround to allow signed integers
////////////////////////////////////////////////////////////////////////////////////////////////
typedef MBReg<eTestRegs, Poco::UInt16> Reg_UInt16;

////////////////////////////////////////////////////////////////////////////////////////////////
// Specialization to handle signed integers
////////////////////////////////////////////////////////////////////////////////////////////////
class Reg_Int16
        : public Reg_UInt16 {
public:
    Reg_Int16(const eTestRegs regID, const Poco::UInt16 uiRegAddress, Poco::UInt16 *pData, Poco::Mutex &mut, Poco::UInt16 defaultValue,
              const RegGroup::eRegGroup groupType = RegGroup::AnalogRW)
            : Reg_UInt16(regID, uiRegAddress, pData, mut, defaultValue, groupType)
    {}

    //Override this method when additional action is required, such as sending a message on a queue
    //Then call this base method
    virtual void setIntValue(Poco::Int16 value)
    {
      _mut.lock();
      *_pData = (Poco::UInt16) value;
      _bNewData = true;
      _mut.unlock();
    };

    virtual Poco::Int16 getIntValue()
    {
      _bNewData = false;
      Poco::Int16 value = (Poco::Int16) (*_pData);

      return value;
    };
};


class Reg_Int16_FtoC
        : public Reg_UInt16 {
public:
    Reg_Int16_FtoC(const eTestRegs regID, const Poco::UInt16 uiRegAddress, Poco::UInt16 *pData, Poco::Mutex &mut, Poco::UInt16 defaultValue,
                   const RegGroup::eRegGroup groupType = RegGroup::AnalogRW)
            : Reg_UInt16(regID, uiRegAddress, pData, mut, defaultValue, groupType)
    {}

    // Override of setValue(Poco::UInt16) would be better
    virtual void setIntValue(Poco::Int16 celsius)
    {
      _mut.lock();
      *_pData = (Poco::UInt16) celsiusToFarenheit(celsius);
      _bNewData = true;
      _mut.unlock();
    };

    // Override of etValue() would be better
    virtual Poco::Int16 getIntValue()
    {
      _bNewData = false;
      return farenheitToCelsius((Poco::Int16) (*_pData));
    };

    virtual Poco::Int16 celsiusToFarenheit(Poco::Int16 celsius)
    {
      // Use double to avoid losing precision until final cast
      double fCelsius = celsius;
      double fFarenheit = ((fCelsius * 9.0) / 5.0) + 32.0;
      Poco::Int16 farenheit = (Poco::Int16) fFarenheit;
      return farenheit;
    }

    virtual Poco::Int16 farenheitToCelsius(Poco::Int16 farenheit)
    {
      // Use double to avoid losing precision until final cast
      double fFarenheit = farenheit;
      double fCelsius = ((fFarenheit - 32.0) * 5.0) / 9.0;
      Poco::Int16 celsius = (Poco::Int16) fCelsius;
      return celsius;
    }
};

class MBRegContainer_test
        : public testing::Test {
public:

    void SetUp()
    {
      _mbMapping = modbus_mapping_new(0, 0, 32 + 2 + 1, 0);
      _regList = new MBRegContainer<eTestRegs>(0, 0, 0, 32 + 2 + 1, 0, 0, 0, 0, TestRegs_Num, _ePrinter);
      _regList->addUInt16(TEST_UI16_VALUE, std::unique_ptr<modplus::MBReg<eTestRegs , Poco::UInt16>>( new Reg_Int16_FtoC(TestRegs_RegUint16, TEST_REG16_ADDRESS, &_reg16InStore, _mut, 0)) );
//				_regList->addUInt16(new MBReg <eTestRegs, Poco::UInt16>         (TestRegs_RegUint16, TEST_REG16_ADDRESS, &_reg16InStore,  _mut, 0));
      _regList->addUInt32(TEST_UI32_VALUE, std::unique_ptr<modplus::MBReg<eTestRegs , Poco::UInt32>>( new MBReg<eTestRegs, Poco::UInt32>(TestRegs_RegUint32, TEST_REG32_ADDRESS,
                                                                                                                                         (Poco::UInt32 *) &_reg32InStore, _mut, 0)) );
      //_regList->addString(new MBReg<eTestRegs, tString>(TestRegs_RegStr, TEST_REGSTR_ADDRESS, (tString *) &_strInStore, _mut, _strInStore));

      //Poco::Path configPath("BMCServer.properties");
      //_configFile = new ppcConfig(&configPath, true); /**<Config file loader.*/

      //_app_config = _configFile->getLayeredConfiguration();
    };

    void TearDown()
    {
      delete _regList;
    };


    Poco::Mutex _mut;
    MBRegContainer<eTestRegs> *_regList;
    Poco::UInt16 _reg16InStore;
    Poco::UInt32 _reg32InStore;
    // PbModbus::tString _strInStore;
    Poco::UInt16 _reg16OutStore;
    Poco::UInt32 _reg32OutStore;
    // PbModbus::tString _strOutStore;
    //Unused for testing, dummy data store to instantiate the register container.
    //In practice the MBReg instances would have to reference sections of this memory
    uint16_t dummyStore[100];
    modbus_mapping_t *_mbMapping;
    TestRegPrint _ePrinter;

};

TEST_F(MBRegContainer_test, TotalRegisters)
{
  EXPECT_EQ(3, _regList->getTotalBlockRegisters());
}

TEST_F(MBRegContainer_test, FindRegType)
{

  EXPECT_EQ(modplus::RegType_UInt16, _regList->getRegTypeFromAddress(TEST_REG16_ADDRESS));

  EXPECT_EQ(modplus::RegType_UInt32, _regList->getRegTypeFromAddress(TEST_REG32_ADDRESS));

  EXPECT_EQ(modplus::RegType_Str, _regList->getRegTypeFromAddress(TEST_REGSTR_ADDRESS));

  //EXPECT_THROW(_a_controller = new ProcessCtrl::BMCProcessCtrl("BMCProcessCtrl", _zmq_context, _app_config)
  //              , Poco::RuntimeException);
}

TEST_F(MBRegContainer_test, Reg_Int16_FtoC)
{
  int tolerance = 1;  // Round Integer to Celsius conversions are only accurate within 1 degree

  Reg_Int16 & reg16Ref = (Reg_Int16 &) _regList->getReg16(eTestRegs::TestRegs_RegUint16);

  for (Poco::Int16 setVal = -200; setVal < 1000; setVal++)
  {
    reg16Ref.setIntValue(setVal);
    int16_t readVal = reg16Ref.getIntValue();
    EXPECT_NEAR(setVal, readVal, tolerance);
  }
}

TEST_F(MBRegContainer_test, FindReg32)
{

  MBReg<eTestRegs, Poco::UInt32> & reg32Result = _regList->getReg32(eTestRegs::TestRegs_RegUint32);
  EXPECT_EQ(TEST_REG32_ADDRESS, reg32Result.getRegisterAddress());
}

// TEST_F(MBRegContainer_test, FindRegStr)
// {
//
//   MBReg<eTestRegs, tString> *regStrResult = _regList->locateRegStr(TEST_REGSTR_ADDRESS);
//   EXPECT_EQ(TEST_REGSTR_ADDRESS, regStrResult->getRegisterAddress());
// }

/*
 * MBReg_test.cpp
 *
 *  Created on: Aug 12, 2015
 *      Author: service
 */

#include "MBReg.h"

// using namespace PbModbus;

#include <gmock/gmock.h>

// #include "core/ppcConfig.h"
// #include "core/ppcLogger.h"

using namespace modplus;

typedef enum {
	TestRegs_Invalid = -1,
	TestRegs_Reg1,
	TestRegs_Num
} eTestRegs;

//zmq::context_t zmq_context(5);
static const Poco::UInt16 TEST_REG_ADDRESS = 0;
static const Poco::UInt16 TEST_UI16_VALUE = 15;
static const std::string TEST_STR_VALUE = "testString";

class MBReg_test : public testing::Test
{
public:

    void SetUp()
    {

        //Poco::Path configPath("BMCServer.properties");
        //_configFile = new ppcConfig(&configPath, true); /**<Config file loader.*/

        //_app_config = _configFile->getLayeredConfiguration();
    };

    void TearDown()
    {

    };


    Poco::Mutex _mut;
};


TEST_F(MBReg_test, SetValue)
{
	Poco::UInt16 uiInStore = 0;

    MBReg <eTestRegs, Poco::UInt16> testReg (TestRegs_Reg1, TEST_REG_ADDRESS, &uiInStore, _mut, 0);

    testReg.setValue(TEST_UI16_VALUE);
    EXPECT_EQ(TEST_UI16_VALUE, testReg.getValue());
	//EXPECT_THROW(_a_controller = new ProcessCtrl::BMCProcessCtrl("BMCProcessCtrl", _zmq_context, _app_config)
      //              , Poco::RuntimeException);
}

TEST_F(MBReg_test, DataFlag)
{
	Poco::UInt16 uiInStore = 0;

	MBReg <eTestRegs, Poco::UInt16> testReg (TestRegs_Reg1, TEST_REG_ADDRESS, &uiInStore, _mut, 0);

    testReg.setValue(TEST_UI16_VALUE);
    EXPECT_EQ(true, testReg.isNewData());

    testReg.getValue();
    EXPECT_EQ(false, testReg.isNewData());
	//EXPECT_THROW(_a_controller = new ProcessCtrl::BMCProcessCtrl("BMCProcessCtrl", _zmq_context, _app_config)
      //              , Poco::RuntimeException);
}

TEST_F(MBReg_test, RegID)
{
	Poco::UInt16 uiInStore = 0;
    MBReg <eTestRegs, Poco::UInt16> testReg (TestRegs_Reg1, TEST_REG_ADDRESS, (uint16_t *) &uiInStore, _mut, 0);

    EXPECT_EQ(TEST_REG_ADDRESS, testReg.getRegisterAddress());

}

// TEST_F(MBReg_test, StringType)
// {
// 	PbModbus::tString tstInStore;
//
// 	strncpy(tstInStore.value, TEST_STR_VALUE.c_str(), sizeof(tstInStore.value));
//     MBReg <eTestRegs, PbModbus::tString> testReg (TestRegs_Reg1, TEST_REG_ADDRESS, (PbModbus::tString *) &tstInStore,  _mut, tstInStore);
//
//     EXPECT_STREQ(TEST_STR_VALUE.c_str(), testReg.getValue().value);
//
// }

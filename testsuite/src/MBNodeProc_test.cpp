
#include <modbus.h>
#include "MBReg.h"
#include "MBRegContainer.h"
#include "MBNodeProc.h"

using namespace modplus;

#include <gmock/gmock.h>
#include "util/IEnumPrinter.h"
#include <thread>
#include "MBTestReg.h"


//zmq::context_t zmq_context(5);
static const Poco::UInt16 TEST_REG16_ADDRESS = 0x0;
static const Poco::UInt32 TEST_REG32_ADDRESS = 0x1;
static const Poco::UInt16 TEST_STR_ADDRESS = 0x3;

static const Poco::UInt16 TEST_UI16_VALUE = 0x55ff;
static const Poco::UInt32 TEST_UI32_VALUE = 0x55115511;

static const Poco::UInt16 TEST_NUM_REG = 30;

static const std::string TEST_STR_VALUE = "testString";

class MBRegNodeProc_test
        : public testing::Test {
public:

    void SetUp()
    {

      //build modbus mapping for register memory storage
      _regList = new MBRegContainer<eTestRegs>(0, 0, 0, 32 + 2 + 1, 0, 0,
                                               0, 0, TestRegs_Num, _ePrinter);

      //Ugly recasting of pointer to the right type of register, by default it is uint16 *
      //Tab input register is for write data
      //*((Poco::UInt16 *) (&_mbMapping->tab_registers[TEST_REG16_ADDRESS])) = TEST_UI16_VALUE;

      //memcpy(&_mbMapping->tab_input_registers[TestRegs_RegUint16], TEST_UI16_VALUE, sizeof(TEST_UI16_VALUE));

      _regList->addUInt16(TestRegs_RegUint16, TEST_REG16_ADDRESS, TEST_UI16_VALUE);

      _regList->addUInt32(TestRegs_RegUint32, TEST_REG32_ADDRESS, 0);
      // _regList->addString(
      //         new MBReg<eTestRegs, tString>(TestRegs_RegStr, TEST_STR_ADDRESS, (tString *) &_mbMapping->tab_registers[TEST_STR_ADDRESS], _mut, _strDefault));

      _ctx = modbus_new_tcp("127.0.0.1", 1502);
      if (_ctx == nullptr)
      {
        fprintf(stderr, "Unable to allocate libmodbus context\n");
        exit(-1);
      }

      _mbNode = new MBNodeProc<eTestRegs>("MBTestSlave", 100, *_regList, 1502, "127.0.0.1", LOG_DEBUG);

      modbus_set_debug(_ctx, TRUE);
      modbus_set_response_timeout(_ctx, 30, 0);
      modbus_set_error_recovery(_ctx, (modbus_error_recovery_mode) (MODBUS_ERROR_RECOVERY_LINK |
                                                                    MODBUS_ERROR_RECOVERY_PROTOCOL));

      modbus_get_response_timeout(_ctx, &_old_response_to_sec, &_old_response_to_usec);

      //Start the slave process thread
      // Poco::ThreadPool::defaultPool().start(*_mbNode);
      // Poco::Thread::sleep(100);   // Make sure the ticker is started.
      _nodeThread = new  std::thread(&MBNodeProc<eTestRegs>::process, _mbNode);

      // Do other things...

      // Makes the main thread wait for the new thread to finish execution, therefore blocks its own execution.

      //Poco::Path configPath("BMCServer.properties");
      //_configFile = new ppcConfig(&configPath, true); /**<Config file loader.*/

      //_app_config = _configFile->getLayeredConfiguration();
    };

    void TearDown()
    {
       // ppcRunnable::tellThreadsToStop();              // Send signal to all runnable threads to stop themselves
      sleep(1);
      modbus_close(_ctx);
      // Poco::ThreadPool::defaultPool().joinAll();  // Wait for the runnable threads to stop.
      _mbNode->setDone(true);
      _nodeThread->join();

      delete _nodeThread;
      delete _mbNode;
      modbus_free(_ctx);
      delete _regList;
      // modbus_mapping_free(_mbMapping);
      // delete _evt;
      // delete _zmqContext;
      // delete _log;
    };


    Poco::Int16 connect()
    {
      Poco::Int16 mbStat = modbus_connect(_ctx);
      modbus_get_response_timeout(_ctx, &_new_response_to_sec, &_new_response_to_usec);

      return (mbStat);
    }

    Poco::Mutex _mut;
    MBRegContainer<eTestRegs> *_regList;
    MBNodeProc<eTestRegs> *_mbNode;
    std::thread * _nodeThread;
    TestRegPrint _ePrinter;
    tString _strDefault;
    uint16_t dummyStore[100];


    //Modbus client components
    modbus_t *_ctx;
    uint32_t _ireal;
    uint32_t _old_response_to_sec;
    uint32_t _old_response_to_usec;
    uint32_t _new_response_to_sec;
    uint32_t _new_response_to_usec;
    uint32_t _old_byte_to_sec;
    uint32_t _old_byte_to_usec;

    Poco::Int16 _connectStatus;


};

TEST_F(MBRegNodeProc_test, Connect)
{
  Poco::Thread::sleep(2000);
  EXPECT_NE(-1, connect());
  //Give the server thread time to complete the connection
  Poco::Thread::sleep(2000);

}


TEST_F(MBRegNodeProc_test, HandleReadU16)
{
  uint16_t readReg[TEST_NUM_REG];

  //= (uint16_t *) malloc(TEST_NUM_REG * sizeof(uint16_t));
  memset(&readReg[0], 0, TEST_NUM_REG * sizeof(uint16_t));

  Poco::Thread::sleep(2000);
  EXPECT_NE(-1, connect());

  Poco::Thread::sleep(1000);
  //Send a read request to the slave process
  //Read one 16bit register at the address of TEST_REG16_ADDRESS and store in the correct location of readReg
  EXPECT_EQ(1, modbus_read_registers(_ctx, TEST_REG16_ADDRESS, 1, &readReg[0]));

  //Look for the default stored result
  EXPECT_EQ(TEST_UI16_VALUE, readReg[TEST_REG16_ADDRESS]);

}

TEST_F(MBRegNodeProc_test, HandleWriteU16)
{
  uint16_t writeReg[TEST_NUM_REG];

  //= (uint16_t *) malloc(TEST_NUM_REG * sizeof(uint16_t));
  memset(&writeReg[0], 0, TEST_NUM_REG * sizeof(uint16_t));

  Poco::Thread::sleep(2000);
  EXPECT_NE(-1, connect());

  Poco::Thread::sleep(1000);
  //Send a read request to the slave process
  //Read one 16bit register at the address of TEST_REG16_ADDRESS and store in the correct location of readReg
  EXPECT_EQ(1, modbus_write_registers(_ctx, TEST_REG16_ADDRESS, 1, &writeReg[0]));

  //Give the slave time to receive and process
  Poco::Thread::sleep(200);

  //See if the write method for the associated register type has been called
  EXPECT_NO_THROW(_regList->getReg16(eTestRegs::TestRegs_RegUint16));
  MBReg<eTestRegs, Poco::UInt16> & reg16 = _regList->getReg16(eTestRegs::TestRegs_RegUint16);

  EXPECT_EQ(true, reg16.isNewData());
}

TEST_F(MBRegNodeProc_test, HandleReconnect)
{
  uint16_t writeReg[TEST_NUM_REG];

  //writeReg = (uint16_t *) malloc(TEST_NUM_REG * sizeof(uint16_t));
  memset(&writeReg[0], 0, TEST_NUM_REG * sizeof(uint16_t));

  Poco::Thread::sleep(2000);
  EXPECT_NE(-1, connect());

  //The response timeout is 5 seconds. After which the slave should accept new connections again
  modbus_close(_ctx);
  Poco::Thread::sleep(6000);

  EXPECT_NE(-1, connect());
  EXPECT_EQ(1, modbus_write_registers(_ctx, TEST_REG16_ADDRESS, 1, &writeReg[0]));

}

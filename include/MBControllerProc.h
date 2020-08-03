
#pragma once

#include "IMBControllerProc.h"
#include "MBRegContainer.h"
#include "util/Util.h"
#include "IRegGroup.h"
#include "AnalogRWGroup.h"
#include "AnalogReadGroup.h"
#include "DigitalRWGroup.h"
#include "DigitalReadGroup.h"

// #include "core/ppcRunnable.h"
// #include "ipc/ppShMem.h"
// #include "drivers/IDev.h"

#include <syslog.h>
#include <modbus/modbus.h>
#include <unistd.h>
#include <linux/serial.h>
#include <sys/ioctl.h>

namespace modplus {

template<typename regIDs>
class MBControllerProc
        : public IMBControllerProc<regIDs> {
public:
    /**
     *
     * @param instanceName Name of instance
     * @param updateRatemS Update rate in milliseconds
     * @param regList List class containing modbus register definitions
     * @param strTargetIP Target IP address
     * @param uiPort Target TCP port
     * @param bManualPoll When true, no data will be automatically polled in this thread
     */
    MBControllerProc(const std::string instanceName,
                     MBRegContainerV2<regIDs> &regList, std::string strTargetIP, const Poco::UInt16 uiPort,
                     const Poco::UInt16 responseTimeoutSec = 5, const bool bManualPoll = false, int log_level = LOG_INFO)
            :
            _objName(instanceName)
            , _regContainer(regList)
            , RESPONSE_TIMEOUT_SEC(responseTimeoutSec)
            , _bManualPoll(bManualPoll)
    {


      _bConnected = false;
      _bCommunicating = false;
      _uiPort = uiPort;
      _strTargetIP = strTargetIP;
      strncpy(_serInfo.deviceName, "", sizeof(_serInfo.deviceName));
      _strDevice = strTargetIP;
      _slaveID = -1;
      init(log_level, instanceName);

    }

    /**
     *
     * @param instanceName
     * @param updateRatemS
     * @param regList
     * @param serInfo Serial device information for use with modbus RTU
     * @param evt
     * @param bManualPoll
     */
    MBControllerProc(const std::string instanceName,
                     MBRegContainerV2<regIDs> &regList, const util::tSerialPort serInfo,
                     const Poco::UInt16 responseTimeoutSec = 5, const bool bManualPoll = false, int log_level = LOG_INFO)
            :
            _objName(instanceName)
            , _regContainer(regList)
            , RESPONSE_TIMEOUT_SEC(responseTimeoutSec)
            , _bManualPoll(bManualPoll)
    {

      _bConnected = false;
      _bCommunicating = false;
      _serInfo = serInfo;
      _strTargetIP = "";
      _strDevice = serInfo.deviceName;
      _slaveID = -1;

      init(log_level, instanceName);

    }

    ~MBControllerProc() override
    {
      //modbus_mapping_free(_mbMapping);
      closeConnection();

      for (Poco::UInt16 idx = 0; idx < (Poco::Int16) _regGroup.size(); ++idx)
      {
        delete _regGroup[idx];
      }
      _regGroup.clear();
    };

    void init(const int log_level, const std::string strName)
    {
      setlogmask(LOG_UPTO (log_level));

      openlog(std::string("ModplusController_" + strName).c_str(), LOG_CONS | LOG_PID | LOG_NDELAY, LOG_LOCAL1);

      syslog(LOG_INFO, "Creating Modbus master.");

      _bLogged = false;

      //Add each register group to the vector. If the register group's data store is NULL no data for that group
      //Will be requested
      _regGroup.push_back(new AnalogRWGroup(_regContainer.getAnalogRWDataStore(), _regContainer.getTotalAnalogRWRegisters()));
      _regGroup.push_back(new AnalogReadGroup(_regContainer.getAnalogReadDataStore(), _regContainer.getTotalAnalogReadRegisters()));
      _regGroup.push_back(new DigitalRWGroup(_regContainer.getDigitalRWDataStore(), _regContainer.getTotalDigitalRWRegisters()));
      _regGroup.push_back(new DigitalReadGroup(_regContainer.getDigitalReadDataStore(), _regContainer.getTotalDigitalReadRegisters()));

      _mbRefreshCount[modplus::RegGroup::DigitalR] = 0;
      _mbRefreshCount[modplus::RegGroup::DigitalRW] = 0;
      _mbRefreshCount[modplus::RegGroup::AnalogRW] = 0;
      _mbRefreshCount[modplus::RegGroup::AnalogRW] = 0;

      openConnection();
    };

    virtual MBRegContainerV2<regIDs> & getRegContainer()
    {
      return _regContainer;
    }

    void configRS232()
    {
      syslog(LOG_INFO, "Configuring RS232: dev:%s baud:%ld parity:%c data_bit:%d, stop_bit:%d, port_config_type: %d",
                   _serInfo.deviceName, _serInfo.baud, _serInfo.parity, _serInfo.data_bit, _serInfo.stop_bit, _serInfo.port_config_type);

      if (modbus_rtu_set_serial_mode(_ctx, MODBUS_RTU_RS232) == -1)
      {
        syslog(LOG_ERR, "Failed to set modbus RTU mode! {%s}", modbus_strerror(errno));
      }
    }

    void configRS485()
    {
      syslog(LOG_INFO, "Configuring RS485: dev:%s baud:%ld parity:%c data_bit:%d, stop_bit:%d, port_config_type: %d",
                   _serInfo.deviceName, _serInfo.baud, _serInfo.parity, _serInfo.data_bit, _serInfo.stop_bit, _serInfo.port_config_type);

      #if 0   // TODO: Review with Tom, new code that wasn't tested, which is currently failing.  It worked before doing this, so trying it without.
      if (modbus_rtu_set_serial_mode(_ctx, MODBUS_RTU_RS485) == -1)
      {
        syslog(LOG_ERR, "Failed to set modbus RTU mode! {%s}", modbus_strerror(errno));
        syslog(LOG_INFO, "Failed to configure: dev:%s baud:%ld parity:%c data_bit:%d, stop_bit:%d, isRS485Device: %d",
                     _serInfo.deviceName, _serInfo.baud, _serInfo.parity, _serInfo.data_bit, _serInfo.stop_bit, _serInfo.isRS485);
      }
      #endif

      //Get the modbus fd to further configure
      int fd = modbus_get_socket(_ctx);
      if (fd == -1)
      {
        syslog(LOG_ERR, "Failed to get modbus file descriptor!");
      }

      struct serial_rs485 rs485conf;

      /* Enable RS485 mode: */
      rs485conf.flags = SER_RS485_ENABLED;

      /* Set logical level for RTS pin equal to 1 when sending: */
      rs485conf.flags |= SER_RS485_RTS_ON_SEND;
      /* or, set logical level for RTS pin equal to 0 when sending: */
      //rs485conf.flags &= ~(SER_RS485_RTS_ON_SEND);

      /* Set logical level for RTS pin equal to 1 after sending: */
      //rs485conf.flags |= SER_RS485_RTS_AFTER_SEND;
      /* or, set logical level for RTS pin equal to 0 after sending: */
      rs485conf.flags &= ~(SER_RS485_RTS_AFTER_SEND);

      /* Set rts delay before send, if needed: */
      rs485conf.delay_rts_before_send = 0;

      /* Set rts delay after send, if needed: */
      rs485conf.delay_rts_after_send = 0;

      /* Set this flag if you want to receive data even whilst sending data */
      //rs485conf.flags |= SER_RS485_RX_DURING_TX;

      if (ioctl(fd, TIOCSRS485, &rs485conf) < 0)
      {
        /* Error handling. See errno. */
        syslog(LOG_ERR, "Failed RS-485 config: %s", strerror(errno));
      }
    }

    void openConnection()
    {

      if (_strTargetIP != "")
      {
        _ctx = modbus_new_tcp(_strTargetIP.c_str(), _uiPort);

        if (_ctx == NULL)
        {
          syslog(LOG_ERR, "Cannot get a ctx structure!");
          throw Poco::InvalidAccessException("Cannot get a ctx structure!");
        }

        syslog(LOG_INFO, "Created a MBMaster connection to %s:%d", _strTargetIP.c_str(), _uiPort);
      }
      else
      {
        _ctx = modbus_new_rtu(_serInfo.deviceName, _serInfo.baud, _serInfo.parity, _serInfo.data_bit, _serInfo.stop_bit);

        if (_ctx == NULL)
        {
          syslog(LOG_ERR, "Cannot get a ctx structure!  dev:%s {%s}", _serInfo.deviceName, modbus_strerror(errno));
          throw Poco::InvalidAccessException("Cannot get a ctx structure!");
        }

        syslog(LOG_INFO, "Created a MBMaster connection to %s: %ld %c %d:%d", _serInfo.deviceName, _serInfo.baud, _serInfo.parity, _serInfo.data_bit, _serInfo.stop_bit);

        setModbusSlave(1);
      }

      /**
       * At minimum set a 25 mS timeout. If set to 0 the library appears to default to 500mS
       */
      modbus_set_response_timeout(_ctx, RESPONSE_TIMEOUT_SEC, 0);
      modbus_set_debug(_ctx, FALSE);

    }

    void closeConnection()
    {
      modbus_close(_ctx);
      modbus_free(_ctx);
      _bConnected = false;
      _bCommunicating = false;

    };

    bool isConnected()
    { return (_bConnected); };

    bool isCommunicating()
    { return (_bCommunicating && _bConnected); };

    int getFd()
    {
      return (modbus_get_socket(_ctx));
    };

    //Write a message to the specified address
    bool writeAnalog(uint16_t *const data, const Poco::UInt16 uiNumRegisters, const Poco::UInt32 mbDataAddress)
    {

      if (!_bConnected)
      {
        return false;
      }

      _mutMB.lock();
      if (modbus_write_registers(_ctx, mbDataAddress, uiNumRegisters, data) != uiNumRegisters)
      {
        syslog(LOG_ERR, "DeviceID: %d : Failed to write analog register %d [0x%x]  value:%d", _slaveID, mbDataAddress, mbDataAddress, *data);
        _mutMB.unlock();
        return (false);
      }

      _mutMB.unlock();
      return (true);
    }

    ///Write an analog value based on the register ID
    template<class valType>
    bool writeAnalog(const regIDs regId, valType &writeVal)
    {
      if (!_bConnected)
      {
        return false;
      }
      _mutMB.lock();

      ///The register addresses need to be converted to data adresses for the read/write block, so subtract 40001
      //TODO: -40001 was removed as address offset which would effect inverter control. Redefine inverter control regs
      if (modbus_write_registers(_ctx, (_regContainer.getRegisterAddressFromRegID(regId)), (sizeof(valType) / 2), (const uint16_t *) &writeVal) != (sizeof(valType) / 2))
      {
        syslog(LOG_ERR, "DeviceID: %d : Failed to write analog register %d [0x%x]  value:%d", _slaveID, _regContainer.getRegisterAddressFromRegID(regId),
                     _regContainer.getRegisterAddressFromRegID(regId), writeVal);
        _mutMB.unlock();
        return (false);
      }

      _mutMB.unlock();
      return (true);

    }

    bool writeDigital(uint8_t *const data, const Poco::UInt16 uiNumRegisters, const Poco::UInt32 uiAddress)
    {

      _mutMB.lock();
      if (modbus_write_bits(_ctx, uiAddress, uiNumRegisters, data) != uiNumRegisters)
      {
        syslog(LOG_ERR, "DeviceID: %d : Failed to write digital register 0x%x", _slaveID, uiAddress);
        _mutMB.unlock();
        return (false);

      }

      _mutMB.unlock();
      return (true);

    }

    //virtual implemetations
    void process() override
    {
      int rc;
      syslog(LOG_DEBUG, "MBMasterProcV2::process() devStr:%s connected:%d communicating:%d", _strDevice.c_str(), _bConnected, _bCommunicating);


      if (!_bConnected)
      {

        /**
         * At minimum set a 5 sec timeout. If set to 0 the library appears to default to 500mS
         */
        modbus_set_response_timeout(_ctx, RESPONSE_TIMEOUT_SEC, 0);
        //Re-Create the connection
        if (modbus_connect(_ctx) == -1)
        {
          syslog(LOG_DEBUG, "Connection attempt failed :: %s [%s port %d]: %s", _objName.c_str(), _strDevice.c_str(), _uiPort, modbus_strerror(errno));
          if (!_bLogged)
          {
            syslog(LOG_NOTICE, "Connection failed to %s [%s]", _objName.c_str(), _strDevice.c_str(),
                   modbus_strerror(errno));
            _bLogged = true;
          }

        }
        else
        {
          switch (_serInfo.port_config_type)
          {
            case util::_RS232:
              configRS232();
              break;
            case util::_RS485:
              configRS485();
              break;
            case util::_NONE:
              break;
          }

          syslog(LOG_INFO, "Connection to %s established", _objName.c_str());
          _bConnected = true;
          _bLogged = false;
        }
      }


      try
      {
        //Read all four modbus types if not in a manual polling mode
        if (!_bManualPoll && _bConnected)
        {
          readRegisterBlock(modplus::RegGroup::AnalogR, _regContainer.getAnalogReadStartAddress());
          readRegisterBlock(modplus::RegGroup::AnalogRW, _regContainer.getAnalogRWStartAddress());
          readRegisterBlock(modplus::RegGroup::DigitalR, _regContainer.getDigitalReadStartAddress());
          readRegisterBlock(modplus::RegGroup::DigitalRW, _regContainer.getDigitalRWStartAddress());
        }
      }
      catch (Poco::Exception &e)
      {
        syslog(LOG_ERR, "Exception: %s", e.message().c_str());
      }

      for (auto &&reg : _regContainer.regUI16)   // Scale any registers which require it.
      {

        if (reg.second->isScaledValue())
        {
          reg.second->updateScaledValue();
        }
      }


      //_shMem.Write(_dataStore);
    };

    void readRegisterBlock(const modplus::RegGroup::eRegGroup groupType, const Poco::UInt32 startAddress)
    {

      if (_regGroup[groupType]->getNumRegisters() == 0)
      {
        return;
      }

      Poco::UInt16 numRegisters = (_regGroup[groupType]->getNumRegisters() - startAddress);
      readRegisterBlock(groupType, startAddress, numRegisters);

    };

    std::string getSlaveId()
    {
      std::string slaveId = "Unknown";
      uint8_t buffer[MODBUS_MAX_PDU_LENGTH];
      int rc = modbus_report_slave_id(_ctx, MODBUS_MAX_PDU_LENGTH, buffer);
      if (rc > 1)
      {
        buffer[rc] = 0;
        slaveId = (char *) &buffer[2];
      }
      return slaveId;
    }

    /**
     *
     * @param groupType The modbus register set to read
     * @param startAddress The starting address to read in the group type
     * @param numRegisters The number of registers to read
     */
    void readRegisterBlock(const modplus::RegGroup::eRegGroup groupType, const Poco::UInt32 startAddress, const Poco::UInt16 numRegisters)
    {

      _mutMB.lock();

      Poco::UInt32 refStartAddress = startAddress; //_regGroup[groupType]->getReadOffset();
      Poco::UInt16 registersToRead = numRegisters;
      //
      //Only 256 bytes or 128 registers can be read at once
      Poco::Int16 rc;

      while (registersToRead > 0)
      {
        if (registersToRead >= 125)
        {

          rc = _regGroup[groupType]->readData(125, refStartAddress, _ctx);
        }
        else
        {
          //if( ((registersToRead % 125) + refStartAddress) >  )
          rc = _regGroup[groupType]->readData((Poco::UInt16) (registersToRead % 125), refStartAddress, _ctx);


        }

        syslog(LOG_DEBUG, "readRegisterBlock(group:%d ,startAddr:%d, numRegs:%d) regToRead:%d, startAddr:%d, return:%d",
               groupType, startAddress, numRegisters, registersToRead, refStartAddress, rc);

        if (rc < _regGroup[groupType]->getNumRegisters() && rc > 0)
        {
          syslog(LOG_DEBUG, "Read %d registers (of %d)", rc, registersToRead);
          registersToRead -= rc;
          refStartAddress += rc;
          _bCommunicating = true;
        }
        else if (rc == -1)
        {
          std::string err_msg;

          if (_strTargetIP != "")
          {
            err_msg = std::string("Modbus(") + _strTargetIP + ")";

            closeConnection();      //Only close and re-open the connection if it is modbus tcp and not modbus rtu
            openConnection();
          }
          else
          {
            std::string slaveId = getSlaveId();
            err_msg = std::string("Modbus(slaveId => expected: ") + std::to_string(_slaveID) + ", actual: " + slaveId + ")";
          }

          err_msg += std::string(" errn:{") + modbus_strerror(errno) + "} attempted to read " + std::to_string(registersToRead % 125) +
                     " of " + std::to_string(registersToRead) + " addr: " + std::to_string(refStartAddress) + " type: " + std::to_string(groupType);

          _mutMB.unlock();

          //Throw an exception here as well, so that the master process doesn't continue trying to read all of the register
          //blocks in case the device really has gone offline
          throw Poco::IOException(err_msg);
        }
        else
        {
          incrModbusRefreshCount(groupType);
          syslog(LOG_DEBUG, "Finished reading %d registers in group type %d.", rc, (int) groupType);

          break;
          //_shMem.Write(_dataStore);
        }

      }

      _mutMB.unlock();

    }

    int getModbusRefreshCount(const modplus::RegGroup::eRegGroup groupType)
    {
      return _mbRefreshCount[groupType];
    }

    void incrModbusRefreshCount(const modplus::RegGroup::eRegGroup groupType)
    {
      if (_mbRefreshCount[groupType] == INT_MAX)
      {
        _mbRefreshCount[groupType] = 0;
      }
      _mbRefreshCount[groupType]++;
    }

    /**
     *
     * @param nodeID the target slave identifier. important when communicating with devices on modbus RTU.
     * @return
     */
    int setModbusSlave(int slaveID)
    {
      _slaveID = slaveID;
      Poco::Int16 retCode = (Poco::Int16) modbus_set_slave(_ctx, slaveID);
      syslog(LOG_DEBUG, "Setting slave to %d", slaveID);
      if (retCode == -1)
      {
        syslog(LOG_ERR, "Failed to set slave ID to %d: %s", slaveID, modbus_strerror(errno));
      }

      //Without a sleep here, the next read request sometimes fails causing the connection to reset
      usleep(10000);
      return (retCode);
    }


protected:

    modbus_t *_ctx = nullptr;
    Poco::Int16 _numRegisters;

    //This is a vector that can contain either only the holding analog register list or all four register types
    MBRegContainerV2<regIDs> &_regContainer;
    int _mbRefreshCount[4] = {0};
    bool _bConnected = false;
    bool _bCommunicating = false;
    const Poco::UInt16 RESPONSE_TIMEOUT_SEC;


private:

    const std::string _objName;
    /**
     * Network based modbus connection info.
     */
    std::string _strTargetIP;
    Poco::UInt16 _uiPort = 502;
    /**
     * Stores the name of whatever device is used to make the connection
     */
    std::string _strDevice;
    /**
     * Serial based modbus connetion info
     */
    util::tSerialPort _serInfo;
    Poco::Mutex _mutMB;
    std::vector<IRegGroup *> _regGroup;
    //Points to the register group for
    IRegGroup *_currentRegGroup = nullptr;
    //Inidcate that a particular event has been logged
    bool _bLogged = false;
    int _slaveID = 1;
    /**
     * When set to true, this thread will not automatically read any registers. Reads would have to be done externally
     */
    bool _bManualPoll;

};

}


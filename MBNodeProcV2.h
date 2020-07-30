
#pragma once

#include <modbus/modbus.h>
#include <unistd.h>
#include <comm/Util.h>
#include "MBRegContainerV2.h"
#include "comm/Util.h"
#include "core/ppcRunnable.h"
#include "drivers/IDev.h"
#include "eventDb/EventLogger.h"
#include <linux/serial.h>
#include <sys/ioctl.h>
#include <termios.h>
#include "Poco/Thread.h"

namespace PbModbusV2 {

static const Poco::UInt16 DEFAULT_TIMEOUT_SEC = 5;

template<typename regId>
class MBNodeProcV2
        : public ppcRunnable {
public:
    // For MODBUS/TCPIP
    MBNodeProcV2(std::string instanceName, Poco::Int16 updateRatemS, MBRegContainerV2 <regId> &regList, Poco::UInt16 uiPort, std::string strListenIP, EventLogger &evt,
                 const Poco::UInt32 log_level = LG_INFO)
            : ppcRunnable(instanceName.c_str(), updateRatemS, true), _regList(regList), RESPONSE_TIMEOUT_SEC(DEFAULT_TIMEOUT_SEC), _evt(evt)
              , _mbMapping(regList.getModbusMap())
    {
      //Make the number of digital and analog in/out registers match

      _bConnected = false;
      _uiPort = uiPort;
      _listenIP = strListenIP;
      // Set serInfo to empty (not used in this constructor)
      memset(&_serInfo, 0, sizeof(_serInfo));
      _bListenerSocket = false;

      _log = new ppcLogger(instanceName.c_str());
      _log->setLevel(log_level);

      init();

    };

    // For MODBUS/RTU Serial
    MBNodeProcV2(std::string instanceName, Poco::Int16 updateRatemS, MBRegContainerV2 <regId> &regList, const Comm::tSerialPort serInfo, EventLogger &evt)
            : ppcRunnable(instanceName.c_str(), updateRatemS, true), _regList(regList), RESPONSE_TIMEOUT_SEC(DEFAULT_TIMEOUT_SEC), _evt(evt)
              , _mbMapping(regList.getModbusMap())
    {
      //Make the number of digital and analog in/out registers match
      _bConnected = false;

      // Set up _serInfo
      _serInfo = serInfo;
      _strDevice = serInfo.deviceName;

      // Clear _listenIP (not used in this constructor), to force init() to use RTU logic
      _listenIP = "";
      _bListenerSocket = false;
      _slaveID = -1;

      _log = new ppcLogger(instanceName.c_str());
      _log->setLevel(LG_INFO);

      init();

    };


    virtual ~MBNodeProcV2()
    {
      //modbus_mapping_free(_mbMapping);
      deInit();
      delete _log;
    };

    void init()
    {
      openConnection();
    }


    void configRS232()
    {
      _logRun->log(LG_INFO, "Configuring RS232: dev:%s baud:%ld parity:%c data_bit:%d, stop_bit:%d, port_config_type: %d",
                   _serInfo.deviceName, _serInfo.baud, _serInfo.parity, _serInfo.data_bit, _serInfo.stop_bit, _serInfo.port_config_type);

        int status = modbus_rtu_set_serial_mode(_ctx, MODBUS_RTU_RS232);
        if (status == -1)
        {
            _logRun->log(LG_ERR, "Failed to set modbus RTU mode! {%s}", modbus_strerror(errno));
        }
        else
        {
            _logRun->log(LG_ERR, "modbus_rtu_set_serial_mode succeeded with status: %d", status);
        }
    }

    void configRS485()
    {
      _logRun->log(LG_INFO, "Configuring RS485: dev:%s baud:%ld parity:%c data_bit:%d, stop_bit:%d, port_config_type: %d",
                   _serInfo.deviceName, _serInfo.baud, _serInfo.parity, _serInfo.data_bit, _serInfo.stop_bit, _serInfo.port_config_type);

      if (modbus_rtu_set_serial_mode(_ctx, MODBUS_RTU_RS485) == -1)
      {
        _logRun->log(LG_ERR, "Failed to set modbus RTU mode! {%s}", modbus_strerror(errno));
      }

      //Get the modbus fd to further configure
      int fd = modbus_get_socket(_ctx);
      if (fd == -1)
      {
        _logRun->log(LG_ERR, "Failed to get modbus file descriptor!");
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
        _logRun->log(LG_ERR, "Failed RS-485 config: %s", strerror(errno));
      }
    }

    void openConnection()
    {
      if (_listenIP != "") {
        if (_ctx == NULL) {
          _slaveSocket = -1;
          _ctx = modbus_new_tcp(_listenIP.c_str(), _uiPort);
          modbus_set_response_timeout(_ctx, RESPONSE_TIMEOUT_SEC, 0);

          modbus_set_debug(_ctx, FALSE);

          _log->log(LG_INFO, "Created a MBSlave listening on %s:%d", _listenIP.c_str(), _uiPort);
        }
      }
      else
      {
        if (_ctx == NULL) {
          _slaveSocket = -1;
          _ctx = modbus_new_rtu(_serInfo.deviceName, _serInfo.baud, _serInfo.parity, _serInfo.data_bit, _serInfo.stop_bit);
          if (_ctx) {
            _log->log(LG_INFO, "Created an RTU MBSlave listening on %s, parity: %c, data_bits: %d, stop_bits: %d", _serInfo.deviceName, _serInfo.parity, _serInfo.data_bit, _serInfo.stop_bit);
            int tempSlaveSocket = modbus_get_socket(_ctx);
            if (tempSlaveSocket > -1) {
                _log->log(LG_INFO, "modbus_get_socket(_ctx) SUCCEEDED, socket: %d", tempSlaveSocket);
            } else
            {
                _log->log(LG_INFO, "modbus_get_socket(_ctx) FAILED, socket: %d", tempSlaveSocket);
            }
          }
          else
          {
            _log->log(LG_ERR, "ERROR - could not create RTU MBSlave listening on %s, parity: %c, data_bits: %d, stop_bits: %d", _serInfo.deviceName, _serInfo.parity, _serInfo.data_bit, _serInfo.stop_bit);
          }

          switch (_serInfo.port_config_type)
          {
            case Comm::_RS232:
              configRS232();
              break;
            case Comm::_RS485:
              configRS485();
              break;
            case Comm::_NONE:
              break;
          }

          setModbusSlave(1);

          modbus_set_response_timeout(_ctx, RESPONSE_TIMEOUT_SEC, 0);
          modbus_set_debug(_ctx, FALSE);
        }
      }
    };

    void deInit()
    {
      if (_slaveSocket != -1)
      {
        close(_slaveSocket);
        _bListenerSocket = false;
      }

      modbus_close(_ctx);
      modbus_free(_ctx);
      _ctx = NULL;
      _bConnected = false;
    };

    //virtual implementations
    virtual void process() {
        if (isModbusRTU()) {
            process_modbus_rtu();
        } else {
            process_modbus_tcp();
        }
    }

    virtual void process_modbus_tcp()
    {
        int rc;

        if (!_bConnected)
        {
            _log->log(LG_DEBUG, "Waiting for connection.");
            modbus_set_response_timeout(_ctx, RESPONSE_TIMEOUT_SEC, 0);

            if (!_bListenerSocket)
            {
                _slaveSocket = modbus_tcp_listen(_ctx, 1);
                _bListenerSocket = true;
            }

            fd_set readfds;
            //Wait to accept a connection
            if (_slaveSocket >= 0)
            {

                FD_ZERO(&readfds);
                FD_SET(_slaveSocket, &readfds);

                struct timeval pollTimeout;
                pollTimeout.tv_sec = 2;
                pollTimeout.tv_usec = 0;
                //wait for an activity on the sockets
                int activity = select(_slaveSocket + 1, &readfds, NULL, NULL, &pollTimeout);

                if ((activity < 0) && (errno != EINTR))
                {
                    _log->log(LG_ERR, "Select error");
                }

                //Activity is occurring on the socket
                //Accept a connection
                if (FD_ISSET(_slaveSocket, &readfds))
                {
                    modbus_tcp_accept(_ctx, &_slaveSocket);
                    _log->log(LG_INFO, "--> Received a connection  on %s:%d", _listenIP.c_str(), _uiPort);
                    modbus_set_response_timeout(_ctx, RESPONSE_TIMEOUT_SEC, 0);
                    _bConnected = true;
                }

            }
            else
            {
                _log->log(LG_INFO, "--> Failed to get a listener socket.");
                _bListenerSocket = false;
            }
        }


        if (!_bConnected)
        {
            return;
        }


        //TODO: when a write command is implemented, need to locate and call associated event handler for that message

        //Only attempt to receive data when connected, otherwise a crash will occur
        //Returns request length
        rc = modbus_receive(_ctx, _query);

        if (rc == 0)
        {
            //Note that the response timeout seems to have no effect on the slave
            _log->log(LG_INFO, "Timeout: Received no data.");
        }
        else if (rc > 0)
        {

            /* rc is the query size */
            modbus_reply(_ctx, _query, rc, &_mbMapping);

            try
            {
                handleReq();
            }
            catch (Poco::Exception &e)
            {
                _log->log(LG_ERR, e.message().c_str());
            }


        }
        else if (rc == -1)
        {

            _evt.logEvent(EVENT_NOTICE, "Modbus receive error, resetting link (%s, %s)", _listenIP.c_str(), modbus_strerror(errno));
            deInit();
            init();

        }

    };


    virtual void process_modbus_rtu()
    {
        if (!_bConnected)
        {
            _log->log(LG_DEBUG, "Waiting for connection.");
            modbus_set_response_timeout(_ctx, RESPONSE_TIMEOUT_SEC, 0);

            if (modbus_connect(_ctx) == -1)
            {
                _log->log(LG_INFO, "modbus_connect (status == -1) for serial connection on %s, Error: %s",
                          _strDevice.c_str(), modbus_strerror(errno));
                return;
            }
            else
            {
                _log->log(LG_INFO, "modbus_connect SUCCEEDED for serial connection on %s", _strDevice.c_str());
            }

            _bConnected = true;
        }

        int requestLength = modbus_receive(_ctx, _query);
        if (requestLength == -1)
        {
            _log->log(LG_INFO, "Connection failed (status == -1) for serial connection on %s, Error: %s",
                      _strDevice.c_str(), modbus_strerror(errno));
            return;
        }
        else if (requestLength == 0)
        {
            //Note that the response timeout seems to have no effect on the slave
            _log->log(LG_INFO, "Timeout: Received no data.");
        }
        else if (requestLength > 0)
        {
            // _log->log(LG_INFO, "modbus_receive SUCCEEDED for serial connection on %s, %d, query: %s",
            //        _strDevice.c_str(), requestLength, _query);

            int sentMsgLength = modbus_reply(_ctx, _query, requestLength, &_mbMapping);

            // _log->log(LG_INFO, "Sent modbus message of length: %d", sentMsgLength);
            try
            {
                handleReq();
            }
            catch (Poco::Exception &e)
            {
                _log->log(LG_ERR, e.message().c_str());
            }
        }
    };

    bool is_bConnected() const
    {
      return _bConnected;
    }

    const Poco::UInt16 RESPONSE_TIMEOUT_SEC;

private:

    bool isModbusRTU()
    {
        // If using MODBUS/TCP, we init _serInfo to all 0's.  Thus
        // we can test a char in deviceName to see if it's MODBUS/RTU.
       return (_serInfo.deviceName[0] > 0);
    }

    bool isWriteReq()
    {
      //Modbus tcp byte 8 is the read/write request
      //See http://www.simplymodbus.ca/TCP.htm
      if (_query[7] == 0x10 ||
          _query[7] == 0x06)
      {
        return (true);
      }

      return (false);
    };

    bool isReadReq()
    {
      //Modbus tcp byte 8 is the read/write request
      //See http://www.simplymodbus.ca/TCP.htm
      if (_query[7] == 0x03 ||
          _query[7] == 0x04)
      {
        return (true);
      }

      return (false);
    }


    /**
     *
     * @param slaveID the target slave identifier. important when communicating with devices on modbus RTU.
     * @return
     */
    int setModbusSlave(int slaveID)
    {
      _slaveID = slaveID;
      Poco::Int16 retCode = (Poco::Int16) modbus_set_slave(_ctx, slaveID);
      _logRun->log(LG_DEBUG, "Setting slave to %d", slaveID);
      if (retCode == -1)
      {
        _logRun->log(LG_ERR, "Failed to set slave ID to %d: %s", slaveID, modbus_strerror(errno));
      }

      //Without a sleep here, the next read request sometimes fails causing the connection to reset
      usleep(10000);
      return (retCode);
    }

    //Determine which register is being written so that it's write message handler may be called
    void handleReq()
    {
      //Bytes 9-10
      Poco::UInt32 uiAddress = (_query[8] << 8);
      uiAddress |= _query[9];
      Poco::UInt16 uiNumRegisters = (_query[10] << 8);
      uiNumRegisters |= _query[11];
      Poco::UInt16 uiAddrSearch;

      bool bIsRead = isReadReq();
      bool bIsWrite = isWriteReq();


      _log->log(LG_DEBUG, " handleReq() read:%d write:%d  baseAddr:%d %s #total reg:%d", bIsRead, bIsWrite, uiAddress,
                _regList.getEnumPrinter().getString(_regList.getRegIdFromAddress(uiAddress)).c_str(), uiNumRegisters);

      for (Poco::UInt16 uiCount = 0; uiCount < uiNumRegisters; ++uiCount)
      {

        uiAddrSearch = uiAddress + uiCount;

        ///Not every address will have an associated register
        regId eReg = _regList.getRegIdFromAddress(uiAddrSearch);
        if (eReg == -1)
        {
          continue;
        }

        switch (_regList.getRegTypeFromAddress(uiAddrSearch))
        {
          case RegType_UInt16:
            if (bIsWrite)
            {
              _regList.getReg16(eReg).setValue();
            }
            else if (bIsRead)
            {
              _regList.getReg16(eReg).getValue();
            }

            if (_regList.getReg16(eReg).isScaledValue())
            {
              _regList.getReg16(eReg).updateScaledValue();
            }
            break;

          case RegType_UInt32:
            if (bIsWrite)
            {
              _regList.getReg32(eReg).setValue();
            }
            else if (bIsRead)
            {
              _regList.getReg32(eReg).getValue();
            }
            break;

          case RegType_UInt64:
            if (bIsWrite)
            {
              _regList.getReg64(eReg).setValue();
            }
            else if (bIsRead)
            {
              _regList.getReg64(eReg).getValue();
            }
            break;

          case RegType_Str:
            if (bIsWrite)
            {
              _regList.getRegStr(eReg).setValue();
            }
            else if (bIsRead)
            {
              _regList.getRegStr(eReg).getValue();
            }
            break;

          case RegType_FloatSingle:
            if (bIsWrite)
            {
              _regList.getRegFloat(eReg).setValue();
            }
            else if (bIsRead)
            {
              _regList.getRegFloat(eReg).getValue();
            }
            break;

          case RegType_Invalid:
            //Do nothing if a multi register read request comes in because there will be unregistered addresses in the list
            if (uiNumRegisters == 1)
            {
              throw Poco::InvalidAccessException("Invalid modbus regtype at address " + Poco::NumberFormatter::format(uiAddrSearch) + ", register " +
                                                 _regList.getEnumPrinter().getString(eReg));
            }
            break;

          default:
            throw Poco::InvalidAccessException("Unknown modbus regtype at address " + Poco::NumberFormatter::format(uiAddrSearch) + ", register " +
                                               _regList.getEnumPrinter().getString(eReg));

        }
      }
    }

    modbus_t *_ctx = NULL;
    modbus_mapping_t &_mbMapping;
    Poco::Int16 _numRegisters;
    uint8_t _query[MODBUS_TCP_MAX_ADU_LENGTH];

    int _slaveSocket;
    ppcLogger *_log;
    MBRegContainerV2<regId> &_regList;
    bool _bConnected;
    bool _bListenerSocket;
    Poco::UInt16 _uiPort;
    /**
     * Stores the name of whatever device is used to make the connection
     */
    std::string _strDevice;
    /**
     * Serial based modbus connetion info
     */
    Comm::tSerialPort _serInfo;
    int _slaveID;
    std::string _listenIP;

protected:
    EventLogger &_evt;
};

}


#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/select.h>
#include <fcntl.h>
#include <stdio.h>
#include <termios.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <iostream>
#include <cstdio>
#include <cstring>
#include <string>
#include <stddef.h>

#include "lls_device.h"

namespace {
    const int DefaultTimeoutMs = 1000;
    const int FrameTimeoutMs = 50;
}

REGISTER_BASIC_INT_PROTOCOL("lls", TLLSDevice, TRegisterTypes({{ 0, "default", "value", Float, true }}));

TLLSDevice::TLLSDevice(PDeviceConfig device_config, PPort port, PProtocol protocol)
    : TBasicProtocolSerialDevice<TBasicProtocol<TLLSDevice>>(device_config, port, protocol)
{}


static unsigned char dallas_crc8(const unsigned char * data, const unsigned int size)
{
    unsigned char crc = 0;
    for ( unsigned int i = 0; i < size; ++i )
    {
        unsigned char inbyte = data[i];
        for ( unsigned char j = 0; j < 8; ++j )
        {
            unsigned char mix = (crc ^ inbyte) & 0x01;
            crc >>= 1;
            if ( mix ) crc ^= 0x8C;
            inbyte >>= 1;
        }
    }
    return crc;
}

void TLLSDevice::EndPollCycle()
{
    CmdResultCache.clear();

    TSerialDevice::EndPollCycle();
}

namespace
{
    const size_t RESPONSE_BUF_LEN = 100;
    const size_t REQUEST_LEN = 4;
    const ptrdiff_t HEADER_SZ = 3;
    const uint8_t REQUEST_PREFIX = 0x31;
    const uint8_t RESPONSE_PREFIX = 0x3E;

}

std::vector<uint8_t> TLLSDevice::ExecCommand(uint8_t cmd)
{
    auto it = CmdResultCache.find(cmd);
    if (it != CmdResultCache.end()) {
        return it->second;
    }

    uint8_t buf[RESPONSE_BUF_LEN] = {};
    buf[0] = REQUEST_PREFIX;
    buf[1] = SlaveId;
    buf[2] = cmd;
    buf[3] = dallas_crc8(buf, REQUEST_LEN -1);
    Port()->WriteBytes(buf, REQUEST_LEN);

    int len = Port()->ReadFrame(buf, RESPONSE_BUF_LEN, this->DeviceConfig()->FrameTimeout);
    if (buf[0] != 0x3e) {
        throw TSerialDeviceTransientErrorException("invalid response prefix");
    }
    if (buf[1] != SlaveId) {
        throw TSerialDeviceTransientErrorException("invalid response network address");
    }
    if (buf[2] != cmd) {
        throw TSerialDeviceTransientErrorException("invalid response cmd");
    }
    if (buf[len-1] != dallas_crc8(buf, len-1)) {
        throw TSerialDeviceTransientErrorException("invalid response crc");
    }

    uint8_t* payload = buf + HEADER_SZ;
    std::vector<uint8_t> result = {0};
    result.assign(payload, payload + len - HEADER_SZ);
    return CmdResultCache.insert({cmd, result}).first->second;
}


uint64_t TLLSDevice::ReadRegister(PRegister reg)
{
    Port()->SkipNoise();
    Port()->CheckPortOpen();

    uint8_t cmd = (reg->Address & 0xFF00) >> 8;
    uint8_t offset = (reg->Address & 0x00FF);

    auto result = ExecCommand(cmd);
    int result_buf[8] = {};

    for (int i=0; i< reg->ByteWidth(); ++i) {
        result_buf[i] = result[offset+i];
    }
    
    return (result_buf[3] << 24) | 
           (result_buf[2] << 16) | 
           (result_buf[1] << 8) |
           result_buf[0];
       
}

void TLLSDevice::WriteRegister(PRegister, uint64_t)
{
    throw TSerialDeviceException("LLS protocol: writing register is not supported");
}

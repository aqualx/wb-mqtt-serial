#include <string>
#include "testlog.h"
#include "fake_serial_port.h"
#include "s2k_device.h"
#include "s2k_expectations.h"

class TS2KDeviceTest: public TSerialDeviceTest, public TS2KDeviceExpectations {
protected:
    void SetUp();
    void TearDown();
    PS2KDevice Dev;

    PRegister RelayReg1;
    PRegister RelayReg2;
    PRegister RelayReg3;
    PRegister RelayReg4;
};

void TS2KDeviceTest::SetUp()
{
    TSerialDeviceTest::SetUp();

    Dev = std::make_shared<TS2KDevice>(
        std::make_shared<TDeviceConfig>("s2k", std::to_string(0x01), "s2k"),
        SerialPort,
        TSerialDeviceFactory::GetProtocol("s2k"));

    RelayReg1 = TRegister::Intern(Dev, TRegisterConfig::Create(TS2KDevice::REG_RELAY, 0x01, U8));
    RelayReg2 = TRegister::Intern(Dev, TRegisterConfig::Create(TS2KDevice::REG_RELAY, 0x02, U8));
    RelayReg3 = TRegister::Intern(Dev, TRegisterConfig::Create(TS2KDevice::REG_RELAY, 0x03, U8));
    RelayReg4 = TRegister::Intern(Dev, TRegisterConfig::Create(TS2KDevice::REG_RELAY, 0x04, U8));

    SerialPort->Open();
}

void TS2KDeviceTest::TearDown()
{
    Dev.reset();
    SerialPort->DumpWhatWasRead();
    TSerialDeviceTest::TearDown();
}

TEST_F(TS2KDeviceTest, TestSetRelayState)
{
    EnqueueSetRelayOnResponse();
    Dev->WriteRegister(RelayReg1, 1);

    SerialPort->DumpWhatWasRead();
    EnqueueSetRelayOffResponse();
    Dev->WriteRegister(RelayReg1, 0);
}

class TS2KIntegrationTest: public TSerialDeviceIntegrationTest, public TS2KDeviceExpectations {
protected:
    void SetUp();
    void TearDown();
    const char* ConfigPath() const { return "../config-s2k-test.json"; }
};

void TS2KIntegrationTest::SetUp()
{
    TSerialDeviceIntegrationTest::SetUp();
}

void TS2KIntegrationTest::TearDown()
{
    SerialPort->DumpWhatWasRead();
    TSerialDeviceIntegrationTest::TearDown();
}
TEST_F(TS2KIntegrationTest, Poll)
{
    Observer->SetUp();
    ASSERT_TRUE(!!SerialPort);

    EnqueueReadConfig1();
    EnqueueReadConfig2();
    EnqueueReadConfig3();
    EnqueueReadConfig4();
    EnqueueReadConfig5();
    EnqueueReadConfig6();
    EnqueueReadConfig7();
    EnqueueReadConfig8();

    Note() << "LoopOnce()";
    Observer->LoopOnce();
    SerialPort->DumpWhatWasRead();

    EnqueueSetRelayOnResponse();
    EnqueueSetRelay2On();
    EnqueueSetRelay3On2();

    MQTTClient->DoPublish(true, 0, "/devices/pseudo_s2k/controls/Relay 1/on", "1");
    MQTTClient->DoPublish(true, 0, "/devices/pseudo_s2k/controls/Relay 2/on", "1");
    MQTTClient->DoPublish(true, 0, "/devices/pseudo_s2k/controls/Relay 3/on", "2");
    SerialPort->DumpWhatWasRead();


    EnqueueReadConfig1();
    EnqueueReadConfig2();
    EnqueueReadConfig3();
    EnqueueReadConfig4();
    EnqueueReadConfig5();
    EnqueueReadConfig6();
    EnqueueReadConfig7();
    EnqueueReadConfig8();

    Note() << "LoopOnce()";
    Observer->LoopOnce();
    SerialPort->DumpWhatWasRead();

    SerialPort->Close();
}

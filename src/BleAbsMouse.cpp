#include "BleAbsMouse.h"

#include <NimBLEDevice.h>

#include "sdkconfig.h"

#if defined(CONFIG_ARDUHAL_ESP_LOG)
#include "esp32-hal-log.h"
#define LOG_TAG ""
#else
#include "esp_log.h"
static const char *LOG_TAG = "NimBLEDevice";
#endif

#define LSB(v) ((v >> 8) & 0xff)
#define MSB(v) (v & 0xff)
#define REPORTID_TOUCH        0x01

static const uint8_t _hidReportDescriptor[] = {
      0x05, 0x0d,                    /* USAGE_PAGE (Digitizer) */
      0x09, 0x04,                    /* USAGE (Touch Screen) */
      0xa1, 0x01,                    /* COLLECTION (Application) */
      0x85, REPORTID_TOUCH,          /*    REPORT_ID */

      /* declare a finger collection */
      0x09, 0x20,                    /*   Usage (Stylus) */
      0xA1, 0x00,                    /*   Collection (Physical) */

      /* Declare a finger touch (finger up/down) */
      0x09, 0x42,                    /*     Usage (Tip Switch) */
      0x09, 0x32,                    /*     USAGE (In Range) */
      0x15, 0x00,                    /*     LOGICAL_MINIMUM (0) */
      0x25, 0x01,                    /*     LOGICAL_MAXIMUM (1) */
      0x75, 0x01,                    /*     REPORT_SIZE (1) */
      0x95, 0x02,                    /*     REPORT_COUNT (2) */
      0x81, 0x02,                    /*     INPUT (Data,Var,Abs) */

      /* Declare the remaining 6 bits of the first data byte as constant -> the driver will ignore them */
      0x75, 0x01,                    /*     REPORT_SIZE (1) */
      0x95, 0x06,                    /*     REPORT_COUNT (6) */
      0x81, 0x01,                    /*     INPUT (Cnst,Ary,Abs) */

      /* Define absolute X and Y coordinates of 16 bit each (percent values multiplied with 100) */
      /* http://www.usb.org/developers/hidpage/Hut1_12v2.pdf */
      /* Chapter 16.2 says: "In the Stylus collection a Pointer physical collection will contain the axes reported by the stylus." */
      0x05, 0x01,                    /*     Usage Page (Generic Desktop) */
      0x09, 0x01,                    /*     Usage (Pointer) */
      0xA1, 0x00,                    /*     Collection (Physical) */
      0x09, 0x30,                    /*        Usage (X) */
      0x09, 0x31,                    /*        Usage (Y) */
      0x16, 0x00, 0x00,              /*        Logical Minimum (0) */
      0x26, 0x10, 0x27,              /*        Logical Maximum (10000) */
      0x36, 0x00, 0x00,              /*        Physical Minimum (0) */
      0x46, 0x10, 0x27,              /*        Physical Maximum (10000) */
      0x66, 0x00, 0x00,              /*        UNIT (None) */
      0x75, 0x10,                    /*        Report Size (16), */
      0x95, 0x02,                    /*        Report Count (2), */
      0x81, 0x02,                    /*        Input (Data,Var,Abs) */
      0xc0,                          /*     END_COLLECTION */
      0xc0,                          /*   END_COLLECTION */
      0xc0                           /* END_COLLECTION */
};

BleAbsMouse::BleAbsMouse(std::string deviceName, std::string deviceManufacturer, uint8_t batteryLevel)
    : _buttons(0), hid(0) {
    this->deviceName         = deviceName;
    this->deviceManufacturer = deviceManufacturer;
    this->batteryLevel       = batteryLevel;
    this->isPressed = false;
}

void BleAbsMouse::begin(void) {
    NimBLEDevice::init(deviceName);
    NimBLEServer *pServer = NimBLEDevice::createServer();
    pServer->setCallbacks(this);
    
    hid        = new NimBLEHIDDevice(pServer);
    inputMouse = hid->inputReport(REPORTID_TOUCH);  // <-- input REPORTID from report map

    hid->manufacturer()->setValue(deviceManufacturer);
    // 0x02：PnP的版本ID, 0xe502：产品类别ID, 0xa111：供应商ID, 0x0210：产品版本
    hid->pnp(0x02, 0xe502, 0xa111, 0x0210);
    // 0x00：国家或地区代码, 0x02：HID规范的版本。
    hid->hidInfo(0x00, 0x02);

    BLESecurity *pSecurity = new NimBLESecurity();

    pSecurity->setAuthenticationMode(ESP_LE_AUTH_BOND);

    
    hid->reportMap((uint8_t *)_hidReportDescriptor, sizeof(_hidReportDescriptor));
    hid->startServices();

    NimBLEAdvertising *pAdvertising = pServer->getAdvertising();
    pAdvertising->setAppearance(HID_MOUSE);
    pAdvertising->addServiceUUID(hid->hidService()->getUUID());
    pAdvertising->start();
    hid->setBatteryLevel(batteryLevel);
}

void BleAbsMouse::end(void) {
}

void BleAbsMouse::click(int16_t x, int16_t y)
{
	this->move(x, y);
	this->release();
}

void BleAbsMouse::move(int16_t x, int16_t y)
{
	this->send(3, x, y);
	this->isPressed = true;
}


void BleAbsMouse::release()
{
	this->send(0, 0, 0);
	this->isPressed = false;
}

bool BleAbsMouse::isConnected(void) const {
    return connected;
}

void BleAbsMouse::setBatteryLevel(uint8_t level) {
    this->batteryLevel = level;
    if (hid != 0)
        this->hid->setBatteryLevel(this->batteryLevel);
}

void BleAbsMouse::onConnect(NimBLEServer *pServer) {
    connected = true;
    if (connectCallback) connectCallback();
}

void BleAbsMouse::onDisconnect(NimBLEServer *pServer) {
    connected = false;
    if (disconnectCallback) disconnectCallback();
}

void BleAbsMouse::onConnect(Callback cb) {
    connectCallback = cb;
}

void BleAbsMouse::onDisconnect(Callback cb) {
    disconnectCallback = cb;
}

void BleAbsMouse::send(int state, int16_t x, int16_t y)
{
  if (this->isConnected())
  {
    uint8_t m[5];
    m[0] = state;
    m[1] = MSB(x);
    m[2] = LSB(x);
    m[3] = MSB(y);
    m[4] = LSB(y);
    this->inputMouse->setValue(m, 5);
    this->inputMouse->notify();
  }
}

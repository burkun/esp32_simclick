#ifndef ESP32_BLE_ABS_MOUSE_H
#define ESP32_BLE_ABS_MOUSE_H
#include "sdkconfig.h"
#if defined(CONFIG_BT_ENABLED)

#include <NimBLECharacteristic.h>
#include <NimBLEHIDDevice.h>

/**
* 魔改蓝牙鼠标，支持绝对位置 + 水平移动
*/
class BleAbsMouse : protected NimBLEServerCallbacks {
  public:
    using Callback = std::function<void(void)>;

  public:
    BleAbsMouse(std::string deviceName = "ESP32-Mouse-Abs", std::string deviceManufacturer = "Espressif", uint8_t batteryLevel = 100);
    void begin(void);
    void end(void);
    void click(int16_t x, int16_t y);
    void move(int16_t x, int16_t y);
    void release();          // release LEFT by default
    bool isConnected(void) const;
    void setBatteryLevel(uint8_t level);
    void onConnect(Callback cb);
    void onDisconnect(Callback cb);

  protected:
    void buttons(uint8_t b);
    void rawAction(uint8_t msg[], char msgSize);
    void onConnect(NimBLEServer* pServer);
    void onDisconnect(NimBLEServer* pServer);
    void send(int state, int16_t x, int16_t y);

  protected:
    uint8_t               _buttons;
    NimBLEHIDDevice*      hid;
    NimBLECharacteristic* inputMouse;

    uint8_t     batteryLevel;
    std::string deviceManufacturer;
    std::string deviceName;
    bool        connected = false;
    bool        isPressed;

    Callback connectCallback    = nullptr;
    Callback disconnectCallback = nullptr;
};

#endif  // CONFIG_BT_ENABLED
#endif  // ESP32_BLE_ABS_MOUSE_H
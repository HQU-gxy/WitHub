//
// Created by Kurosu Chan on 2023/10/26.
//

#ifndef WIT_HUB_SCAN_CALLBACK_H
#define WIT_HUB_SCAN_CALLBACK_H

#include <etl/vector.h>
#include <etl/map.h>
#include <etl/flat_map.h>
#include <NimBLEDevice.h>

namespace blue {
// const auto TARGET         = "WT901BLE67";
const auto TARGET         = "WT901BLE67";
const int MAX_DEVICE_NUM  = 8;
const int MAX_SERVICE_NUM = 4;
const int MAX_CHAR_NUM    = 4;
class ScanCallback : public NimBLEScanCallbacks {
  static const int ADDR_SIZE = 6;
  using addr_t               = etl::array<uint8_t, ADDR_SIZE>;
  using device_map_t         = etl::flat_map<addr_t, NimBLEClient *, MAX_DEVICE_NUM>;
  device_map_t devices{};

public:
  void onResult(NimBLEAdvertisedDevice *advertisedDevice) override {
    const auto TAG = "ScanCallback::onResult";
    auto name      = advertisedDevice->getName();
    if (!name.empty()) {
      ESP_LOGI(TAG, "name=%s; addr=%s", name.c_str(), advertisedDevice->getAddress().toString().c_str());
    }
    if (name == TARGET) {
      auto addr_native = advertisedDevice->getAddress().getNative();
      auto addr        = addr_t{};
      std::copy(addr_native, addr_native + ADDR_SIZE, addr.begin());
      ESP_LOGI(TAG, "address=%s", advertisedDevice->getAddress().toString().c_str());
      NimBLEClient *pClient = nullptr;
      if (devices.find(addr) != devices.end()) {
        ESP_LOGI(TAG, "device already exists");
        pClient = devices[addr];
      } else {
        ESP_LOGI(TAG, "device not found, creating a new one");
        pClient       = NimBLEDevice::createClient();
        devices[addr] = pClient;
      }
      auto &client = *pClient;
      if (client.isConnected()) {
        ESP_LOGI(TAG, "client is already connected");
        return;
      }
      auto ok = client.connect(advertisedDevice);
      if (!ok) {
        ESP_LOGE(TAG, "Failed to connect to the device");
        return;
      }
      auto &services = *client.getServices(true);
      for (auto *pService : services) {
        auto &chars = *pService->getCharacteristics();
        for (auto *pChar : chars) {
          ESP_LOGI(TAG, "service=%s, char=%s",
                   pService->getUUID().toString().c_str(),
                   pChar->getUUID().toString().c_str());
        }
      }
    }
  };
};
}

#endif // WIT_HUB_SCAN_CALLBACK_H

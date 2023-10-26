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
const auto TARGET         = "WT901BLE67";
const int MAX_DEVICE_NUM  = 12;
const int MAX_SERVICE_NUM = 4;
const int MAX_CHAR_NUM    = 4;

// who define these UUIDs? so stupid
const char *SERVICE_UUID = "0000ffe5-0000-1000-8000-00805f9a34fb";
const char *READ_CHAR    = "0000ffe4-0000-1000-8000-00805f9a34fb";
const char *WRITE_CHAR   = "0000ffe9-0000-1000-8000-00805f9a34fb";

void print_services_chars(NimBLEClient &client) {
  const auto TAG = "client";
  auto pServices = client.getServices(true);
  if (!pServices) {
    ESP_LOGE(TAG, "Failed to get services");
    return;
  }
  auto &services = *pServices;
  if (services.empty()) {
    ESP_LOGW(TAG, "No services found");
  }
  for (auto *pService : services) {
    auto pChars = pService->getCharacteristics(true);
    if (!pChars) {
      ESP_LOGE(TAG, "Failed to get characteristics");
      return;
    }
    auto &chars = *pChars;
    if (chars.empty()) {
      ESP_LOGW(TAG, "No characteristics found for service %s", pService->getUUID().toString().c_str());
    }
    for (auto *pChar : chars) {
      const auto TAG = "SCAN_RESULT";
      ESP_LOGI(TAG, "service=%s, char=%s [%s%s%s%s%s]",
               pService->getUUID().toString().c_str(),
               pChar->getUUID().toString().c_str(),
               pChar->canNotify() ? "n" : "",
               pChar->canRead() ? "r" : "",
               pChar->canWrite() ? "w" : "",
               pChar->canWriteNoResponse() ? "W" : "",
               pChar->canIndicate() ? "i" : "");
    }
  }
}

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
      ESP_LOGI(TAG, "name=%s; addr=%s; rssi=%d", name.c_str(), advertisedDevice->getAddress().toString().c_str(), advertisedDevice->getRSSI());
    }
    struct ConnectTaskParam {
      TaskHandle_t task_handle;
      std::function<void()> task;
    };

    if (name == TARGET) {
      auto &self          = *this;
      auto nimble_address = advertisedDevice->getAddress();
      auto addr_native    = nimble_address.getNative();
      auto addr           = addr_t{};
      std::copy(addr_native, addr_native + ADDR_SIZE, addr.begin());
      ESP_LOGI(TAG, "try to connect to %s (%s)", name.c_str(), nimble_address.toString().c_str());
      // for some reason the connection would block the scan callback for a long time
      // I have to create a new thread to do the connection
      auto connect_task = [name, addr, nimble_address, &self]() {
        const auto TAG = "connect";
        auto &devices  = self.devices;
        if (name == TARGET) {
          NimBLEClient *pClient = nullptr;
          if (devices.find(addr) != devices.end()) {
            ESP_LOGW(TAG, "device already exists");
            pClient = devices[addr];
          } else {
            ESP_LOGI(TAG, "device not found, creating a new one");
            pClient       = NimBLEDevice::createClient();
            devices[addr] = pClient;
          }
          auto &client = *pClient;
          if (client.isConnected()) {
            ESP_LOGW(TAG, "client is already connected");
            return;
          }
          auto ok = client.connect(nimble_address);
          if (!ok) {
            ESP_LOGE(TAG, "Failed to connect to the device");
            return;
          }
          print_services_chars(client);
          auto pService = client.getService(NimBLEUUID{SERVICE_UUID});
          if (!pService) {
            ESP_LOGE(TAG, "failed to get service");
            return;
          }
          auto pReadChar = pService->getCharacteristic(NimBLEUUID{READ_CHAR});
          if (!pReadChar) {
            ESP_LOGE(TAG, "failed to get read char");
            return;
          }
          auto pWriteChar =
              pService->getCharacteristic(NimBLEUUID{WRITE_CHAR});
          if (!pWriteChar) {
            ESP_LOGE(TAG, "failed to get write char");
            return;
          }
        }
      };
      auto param = new ConnectTaskParam{
          .task_handle = nullptr,
          .task        = connect_task,
      };
      xTaskCreate([](void *param) {
        auto &p = *static_cast<ConnectTaskParam *>(param);
        p.task();
        auto handle = p.task_handle;
        delete &p;
        ESP_LOGI("connecting", "end");
        vTaskDelete(handle);
      },
                  "connect", 4096, param, 5, &param->task_handle);
    }
  };
};
}

#endif // WIT_HUB_SCAN_CALLBACK_H

#include <sdkconfig.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <esp_system.h>
#include <esp_wifi.h>
#include <esp_spi_flash.h>
#include <etl/random.h>
#include <msd/channel.hpp>
#include <memory>
#include <nvs_handle.hpp>
#include <NimBLEDevice.h>
#include <iostream>
#include <utility>
#include "scan_callback.h"

const auto BLE_NAME = "WitHub";

extern "C" [[noreturn]] void app_main();

[[noreturn]] void app_main() {
  const auto TAG = "main";
  /******** BLE init ********/
  BLEDevice::init(BLE_NAME);
  auto &server = *BLEDevice::createServer();
  auto &scan   = *NimBLEDevice::getScan();
  auto pScanCb = new ScanCallback();
  scan.setScanCallbacks(pScanCb);
  scan.setInterval(1349);
  scan.setWindow(449);
  scan.setActiveScan(true);
  constexpr auto scanTime = std::chrono::milliseconds(1500);
  // total time = scan time + sleep time
  constexpr auto scanTotalTime = std::chrono::milliseconds(3000);
  static_assert(scanTotalTime > scanTime);
  ESP_LOGI(TAG, "Initiated");
  for (;;) {
    bool ok = scan.start(scanTime.count(), false);
    if (!ok) {
      ESP_LOGE(TAG, "Failed to start scan");
    }
    vTaskDelay(scanTotalTime.count() / portTICK_PERIOD_MS);
  }
}

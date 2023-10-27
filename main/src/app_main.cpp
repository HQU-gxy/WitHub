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
#include <sstream>
#include <charconv>
#include "scan_callback.h"
#include "wlan_manager.h"

#define stringify_literal(x)     #x
#define stringify_expanded(x)    stringify_literal(x)
#define stringify_with_quotes(x) stringify_expanded(stringify_expanded(x))

#ifndef WLAN_AP_SSID
#define WLAN_AP_SSID default
#endif

#ifndef WLAN_AP_PASSWORD
#define WLAN_AP_PASSWORD default
#endif

// https://learn.microsoft.com/en-us/cpp/preprocessor/stringizing-operator-hash?view=msvc-170
const char *WLAN_SSID     = stringify_expanded(WLAN_AP_SSID);
const char *WLAN_PASSWORD = stringify_expanded(WLAN_AP_PASSWORD);

const auto BLE_NAME = "WitHub";

extern "C" [[noreturn]] void app_main();

[[noreturn]] void app_main() {
  const auto TAG = "main";
  auto ap        = wlan::AP{WLAN_SSID, WLAN_PASSWORD};
  ESP_LOGI(TAG, "ssid=%s; password=%s", ap.ssid.c_str(), ap.password.c_str());
  auto manager = wlan::WlanManager();
  manager.wifi_init();
  manager.mqtt_init();
  manager.start_connect_task();

  /******** Bluetooth LE init ********/
  NimBLEDevice::init(BLE_NAME);
  auto &scan    = *NimBLEDevice::getScan();
  auto &scan_cb = *new blue::ScanCallback();
  scan.setScanCallbacks(&scan_cb);
  scan.setInterval(1349);
  scan.setWindow(449);
  scan.setActiveScan(true);

  /******** Task and callbacks ********/
  scan_cb.on_data = [&manager](const blue::WitDevice &device, uint8_t *data, size_t length) {
    const auto TAG = "on_data";
    auto pub_msg = wlan::MqttPubMsg{
        .topic = "/wit/" + utils::toHex(device.addr.data(), device.addr.size()) + "/data",
        .data  = std::vector<uint8_t>{data, data + length},
    };
    auto ok        = manager.publish(pub_msg);
    if (!ok) {
      ESP_LOGW(TAG, "failed to send data to sub_msg_chan");
    }
  };

  struct PollChanParam {
    TaskHandle_t task_handle;
    std::function<void()> task;
  };

  auto chan      = manager.sub_msg_chan();
  auto poll_task = [chan, &scan_cb]() {
    // the topic should be "/wit/<topic>/control"
    auto TAG         = "poll_task";
    auto parse_topic = [](const std::string &topic) -> etl::optional<etl::array<uint8_t, 6>> {
      std::vector<std::string> tokens;
      std::stringstream ss{topic};
      std::string item;
      while (std::getline(ss, item, '/')) {
        tokens.push_back(item);
      }
      if (tokens.size() < 2) {
        return etl::nullopt;
      }
      auto addr = tokens[1];
      if (addr.size() != 12) {
        return etl::nullopt;
      }
      // https://stackoverflow.com/questions/55455591/hex-string-to-uint8-t-msg
      auto addr_bytes = etl::array<uint8_t, 6>{};
      for (auto i = 0; i < 6; ++i) {
        auto byte_str  = addr.substr(i * 2, 2);
        auto [ptr, ec] = std::from_chars(byte_str.data(), byte_str.data() + byte_str.size(), addr_bytes[i], 16);
        if (ec != std::errc{}) {
          return etl::nullopt;
        }
      }
      return etl::make_optional(addr_bytes);
    };
    for (auto item : *chan) {
      auto payload  = item.data;
      auto addr_opt = parse_topic(item.topic);
      if (!addr_opt.has_value()) {
        ESP_LOGW(TAG, "invalid topic %s", item.topic.c_str());
        continue;
      }
      auto addr = *addr_opt;
      auto ok   = scan_cb.toDevice(addr, payload.data(), payload.size());
      if (!ok) {
        ESP_LOGW(TAG, "%s (%d) to %s failed",
                 utils::toHex(payload.data(), payload.size()).c_str(), payload.size(),
                 utils::toHex(addr.data(), addr.size()).c_str());
      }
    }
  };

  auto param = new PollChanParam{
      .task_handle = nullptr,
      .task        = poll_task,
  };
  xTaskCreate([](void *pvParameters) {
    auto param = static_cast<PollChanParam *>(pvParameters);
    auto &task = param->task;
    task();
    // should be unreachable
    // just in case
    ESP_LOGE("poll_task", "end");
    auto handle = param->task_handle;
    delete param;
    vTaskDelete(handle);
  },
              "poll_task", 4096, param, 1, &param->task_handle);

  /********* Bluetooth LE scan loop *********/
  constexpr auto scanTime = std::chrono::milliseconds(2500);
  // total time = scan time + sleep time
  constexpr auto scanTotalTime = std::chrono::milliseconds(5000);
  static_assert(scanTotalTime > scanTime);
  ESP_LOGI(TAG, "Initiated");
  for (;;) {
    bool ok = scan.start(scanTime.count(), false);
    if (!ok) {
      ESP_LOGW(TAG, "bad scan");
    }
    vTaskDelay(scanTotalTime.count() / portTICK_PERIOD_MS);
  }
}

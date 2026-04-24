#pragma once

#include "esphome/core/automation.h"
#include "esphome/core/component.h"
#include "esphome/components/uart/uart.h"
#include "esphome/components/uart/uart_component.h"

namespace esphome {
namespace tx_ultimate_touch {

const static uint8_t TOUCH_STATE_RELEASE = 1;
const static uint8_t TOUCH_STATE_PRESS = 2;
const static uint8_t TOUCH_STATE_SWIPE = 3;
const static uint8_t TOUCH_STATE_ALL_FIELDS = 11;
const static uint8_t TOUCH_STATE_SWIPE_RIGHT = 12;
const static uint8_t TOUCH_STATE_SWIPE_LEFT = 13;

// The hardware encodes long press by adding LONG_PRESS_X_OFFSET to the position byte.
// Default is 16 (upstream hardcoded value). Configurable for hardware variants.
const static uint8_t LONG_PRESS_X_OFFSET_DEFAULT = 16;

struct TouchPoint {
  int8_t x = -1;
  int8_t state = -1;
};

class TxUltimateTouch : public uart::UARTDevice, public Component {
 public:
  Trigger<TouchPoint> *get_touch_trigger() { return &this->touch_trigger_; }
  Trigger<TouchPoint> *get_release_trigger() { return &this->release_trigger_; }
  Trigger<TouchPoint> *get_swipe_left_trigger() { return &this->swipe_trigger_left_; }
  Trigger<TouchPoint> *get_swipe_right_trigger() { return &this->swipe_trigger_right_; }
  Trigger<TouchPoint> *get_full_touch_release_trigger() { return &this->full_touch_release_trigger_; }
  Trigger<TouchPoint> *get_long_touch_release_trigger() { return &this->long_touch_release_trigger_; }

  void set_uart_component(esphome::uart::UARTComponent *uart_component) {
    this->set_uart_parent(uart_component);
  }

  // Configures the X offset threshold used by the hardware to signal a long press.
  // The touch panel firmware adds this offset to the position byte for long press events.
  void set_long_press_x_offset(uint8_t offset) { this->long_press_x_offset_ = offset; }

  void setup() override;
  void loop() override;
  void dump_config() override;

 protected:
  void send_touch_(TouchPoint tp);
  void handle_touch(int bytes[]);
  TouchPoint get_touch_point(int bytes[]);
  bool is_valid_data(int bytes[]);
  int get_x_touch_position(int bytes[]);
  int get_touch_state(int bytes[]);

  uint8_t long_press_x_offset_{LONG_PRESS_X_OFFSET_DEFAULT};

  Trigger<TouchPoint> touch_trigger_;
  Trigger<TouchPoint> release_trigger_;
  Trigger<TouchPoint> swipe_trigger_left_;
  Trigger<TouchPoint> swipe_trigger_right_;
  Trigger<TouchPoint> full_touch_release_trigger_;
  Trigger<TouchPoint> long_touch_release_trigger_;
};

}  // namespace tx_ultimate_touch
}  // namespace esphome

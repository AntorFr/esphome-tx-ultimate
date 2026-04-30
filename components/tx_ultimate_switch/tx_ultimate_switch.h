#pragma once

#include "esphome/core/component.h"
#include "esphome/core/automation.h"
#include "esphome/components/light/light_state.h"
#include "esphome/components/binary_sensor/binary_sensor.h"
#include "esphome/components/switch/switch.h"
#include "esphome/components/select/select.h"

#include <vector>
#include <string>

// Forward declarations — full definitions included in the .cpp
namespace esphome {
namespace audio { struct AudioFile; }
namespace media_player { class MediaPlayer; }
}

namespace esphome {
namespace tx_ultimate_switch {

class ThemeSelect : public select::Select {
 protected:
  void control(size_t index) override { this->publish_state(index); }
};

// ── Colour (r,g,b ∈ [0,100]) ────────────────────────────────────────────────
struct Color3 {
  uint8_t r{0}, g{0}, b{100};
};

// ── Nightlight display mode ───────────────────────────────────────────────────
enum class NightlightMode : uint8_t {
  OFF    = 0,  ///< LED off (day, away, or bedroom during sleep)
  NORMAL = 1,  ///< Normal nightlight colour+effect from theme
  SLEEP  = 2,  ///< Non-bedroom sleep: dim guide colour from theme
};

// ── Room type ───────────────────────────────────────────────────────────────
enum class RoomType : uint8_t {
  STANDARD = 0,  ///< Normal: nightlight follows night_sensor; sleep_sensor → SLEEP guide
  BEDROOM  = 1,  ///< Bedroom: sleep_sensor active → LED OFF (don’t wake sleeper)
  DARK     = 2,  ///< No windows: nightlight always ON; sleep/away still respected
};

// ── Button position (user-facing label, mirrored if upside_down_) ───────────
enum class ButtonPosition : uint8_t {
  LEFT   = 0,
  CENTER = 1,
  RIGHT  = 2,
};

// ── Switch-relay mode ────────────────────────────────────────────────────────
enum class SwitchRelayMode : uint8_t {
  ALWAYS        = 0,  ///< toggle relay on every press
  NEVER         = 1,  ///< never toggle (HA-only) — also forces relay ON at init
  FALLBACK_HA   = 2,  ///< toggle only when api_connected is OFF
  FALLBACK_WIFI = 3,  ///< toggle only when wifi_connected is OFF
};

// ── One visual theme ─────────────────────────────────────────────────────────
struct Theme {
  std::string name;

  // Resting-state: button LED when relay is ON
  Color3 button_color;
  float  button_brightness{0.7f};

  // Nightlight (normal mode)
  Color3 nightlight_color;
  float  nightlight_brightness{0.2f};
  std::string nightlight_effect{"None"};

  // Sleep guide color (non-bedroom: dim locator while household sleeps)
  Color3 sleep_color;
  float  sleep_brightness{0.1f};
  std::string sleep_effect{"None"};

  // Touch feedback (top LEDs)
  Color3 touch_color;
  float  touch_brightness{1.0f};
  std::string touch_effect{"Scan"};

  // Swipe left
  Color3 swipe_left_color;
  float  swipe_left_brightness{1.0f};
  std::string swipe_left_effect{"None"};

  // Swipe right
  Color3 swipe_right_color;
  float  swipe_right_brightness{1.0f};
  std::string swipe_right_effect{"None"};

  // Long press
  Color3 long_press_color;
  float  long_press_brightness{1.0f};
  std::string long_press_effect{"None"};

  // Multi-touch / full touch
  Color3 multi_touch_color;
  float  multi_touch_brightness{1.0f};
  std::string multi_touch_effect{"Rainbow"};

  // Sound pack name (matched against sound_pack_name set by the user)
  std::string sound_pack{"explore_the_stars"};
};

// ── Sound file pointers (set by Python codegen via set_sound_*) ──────────────
struct SoundPack {
  std::string name;
  audio::AudioFile *click{nullptr};
  audio::AudioFile *long_press{nullptr};
  audio::AudioFile *multi_press{nullptr};
  audio::AudioFile *slide{nullptr};
};

// ── Button config ─────────────────────────────────────────────────────────────
struct ButtonConfig {
  ButtonPosition position{ButtonPosition::LEFT};
  SwitchRelayMode mode{SwitchRelayMode::FALLBACK_HA};
  light::LightState *relay{nullptr};                       // optional, set by codegen
  binary_sensor::BinarySensor *state_sensor{nullptr};      // external state for LED display
  binary_sensor::BinarySensor *press_sensor{nullptr};      // exposed to HA, pulsed on press
  bool last_relay_state{false};                            // tracked in loop() for LED refresh
};

// ── Main component ────────────────────────────────────────────────────────────
class TxUltimateSwitch : public Component {
 public:
  // ── Wiring setters (called by Python codegen) ───────────────────────────────
  void set_upside_down(bool r) { upside_down_ = r; }

  void add_button(ButtonPosition position, SwitchRelayMode mode, light::LightState *relay) {
    ButtonConfig b;
    b.position = position;
    b.mode = mode;
    b.relay = relay;
    buttons_.push_back(b);
  }

  void set_button_state_sensor(uint8_t idx, binary_sensor::BinarySensor *s) {
    if (idx < buttons_.size()) buttons_[idx].state_sensor = s;
  }

  void set_button_press_sensor(uint8_t idx, binary_sensor::BinarySensor *s) {
    if (idx < buttons_.size()) buttons_[idx].press_sensor = s;
  }

  void set_leds(light::LightState *leds) { leds_ = leds; }
  void set_leds_top(light::LightState *l) { leds_top_ = l; }
  void set_leds_nightlight(light::LightState *l) { leds_nightlight_ = l; }
  void set_leds_button(ButtonPosition pos, light::LightState *l) {
    uint8_t idx = static_cast<uint8_t>(pos);
    if (idx < 3) leds_button_[idx] = l;
  }

  void set_vibra(switch_::Switch *s) { vibra_ = s; }
  void set_media_player(media_player::MediaPlayer *mp) { media_player_ = mp; }

  // Nightlight sensors (all optional — nullptr = not configured)
  void set_night_sensor(binary_sensor::BinarySensor *s) { night_sensor_ = s; }
  void set_sleep_sensor(binary_sensor::BinarySensor *s) { sleep_sensor_ = s; }
  void set_away_sensor(binary_sensor::BinarySensor *s) { away_sensor_ = s; }
  // room_type drives nightlight behaviour (see RoomType enum)
  void set_room_type(RoomType rt) { room_type_ = rt; }

  // Select exposed in HA for theme switching
  void set_theme_select(ThemeSelect *s) { theme_select_ = s; }

  // Theme & sound pack registration
  void add_theme(Theme t) { themes_.push_back(std::move(t)); }
  void add_sound_pack(SoundPack sp) { sound_packs_.push_back(std::move(sp)); }
  void set_initial_theme(const std::string &name) { initial_theme_ = name; }

  // Connectivity sensors (drive fallback_ha / fallback_wifi modes)
  void set_api_connected(binary_sensor::BinarySensor *s) { api_connected_ = s; }
  void set_wifi_connected(binary_sensor::BinarySensor *s) { wifi_connected_ = s; }

  // button_on_time in ms
  void set_button_on_time_ms(uint32_t ms) { button_on_time_ms_ = ms; }
  // touch LED display duration in ms

  // ESP-NOW / offline fallback visual indicator
  void enter_fallback_mode();
  void exit_fallback_mode();
  void set_touch_led_duration_ms(uint32_t ms) { touch_led_duration_ms_ = ms; }

  // ── Touch event handlers (called from tx_ultimate_touch automation) ─────────
  void on_touch_press();
  void on_touch_release(int pos);
  void on_swipe_left();
  void on_swipe_right();
  void on_full_touch_release();
  void on_long_touch_release();

  // ── Binary sensor state callbacks (wired by codegen) ────────────────────────
  void on_night_sensor_change() { refresh_nightlight_(); }
  void on_sleep_sensor_change() { refresh_nightlight_(); }
  void on_away_sensor_change() { refresh_nightlight_(); }
  void on_relay_change() { refresh_led_default_(); }
  void on_theme_select_change() { refresh_led_default_(); }

  // Called from `on_boot` automation
  void init_relays();

  // ESPHome lifecycle
  void setup() override;
  void loop() override;
  void dump_config() override;

  // Public for time-based re-sync (called every 5 min from YAML time trigger)
  void refresh_nightlight() { refresh_nightlight_(); }

 protected:
  // ── Internal helpers ─────────────────────────────────────────────────────────
  NightlightMode compute_nightlight_mode_() const;
  void refresh_nightlight_();
  void refresh_led_default_();
  void apply_touch_led_(const Color3 &color, float brightness, const std::string &effect,
                         uint32_t duration_ms);
  void clear_touch_led_();
  void apply_button_led_(uint8_t idx, bool relay_on);
  void apply_nightlight_led_();
  void cancel_touch_led_timer_();
  ButtonConfig *button_at_position_(ButtonPosition pos);

  const Theme *active_theme_() const;
  const SoundPack *active_sound_pack_() const;
  void play_sound_(audio::AudioFile *file);

  bool upside_down_{false};
  std::vector<ButtonConfig> buttons_;

  light::LightState *leds_{nullptr};
  light::LightState *leds_top_{nullptr};
  light::LightState *leds_nightlight_{nullptr};
  light::LightState *leds_button_[3]{nullptr, nullptr, nullptr};

  switch_::Switch *vibra_{nullptr};
  media_player::MediaPlayer *media_player_{nullptr};

  binary_sensor::BinarySensor *night_sensor_{nullptr};
  binary_sensor::BinarySensor *sleep_sensor_{nullptr};
  binary_sensor::BinarySensor *away_sensor_{nullptr};
  binary_sensor::BinarySensor *api_connected_{nullptr};
  binary_sensor::BinarySensor *wifi_connected_{nullptr};

  ThemeSelect *theme_select_{nullptr};

  std::vector<Theme> themes_;
  std::vector<SoundPack> sound_packs_;
  std::string initial_theme_{"Default"};

  NightlightMode nightlight_mode_{NightlightMode::OFF};
  RoomType room_type_{RoomType::STANDARD};
  bool touch_led_active_{false};
  uint32_t touch_led_start_ms_{0};
  uint32_t button_on_time_ms_{500};
  uint32_t touch_led_duration_ms_{6000};
};

}  // namespace tx_ultimate_switch
}  // namespace esphome

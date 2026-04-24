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

// ── Colour (r,g,b ∈ [0,100]) ────────────────────────────────────────────────
struct Color3 {
  uint8_t r{0}, g{0}, b{100};
};

// ── One visual theme ─────────────────────────────────────────────────────────
struct Theme {
  std::string name;

  // Resting-state: button LED when relay is ON
  Color3 button_color;
  float  button_brightness{0.7f};

  // Nightlight
  Color3 nightlight_color;
  float  nightlight_brightness{0.2f};
  std::string nightlight_effect{"None"};

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
  bool switch_relay{true};
  light::LightState *relay{nullptr};  // set by codegen
  bool last_relay_state{false};       // tracked in loop() for LED refresh
};

// ── Main component ────────────────────────────────────────────────────────────
class TxUltimateSwitch : public Component {
 public:
  // ── Wiring setters (called by Python codegen) ───────────────────────────────
  void set_button_count(uint8_t n) { button_count_ = n; }

  void add_button(bool switch_relay, light::LightState *relay) {
    buttons_.push_back({switch_relay, relay});
  }

  void set_leds(light::LightState *leds) { leds_ = leds; }
  void set_leds_top(light::LightState *l) { leds_top_ = l; }
  void set_leds_nightlight(light::LightState *l) { leds_nightlight_ = l; }
  void set_leds_button(uint8_t idx, light::LightState *l) {
    if (idx < 3) leds_button_[idx] = l;
  }

  void set_vibra(switch_::Switch *s) { vibra_ = s; }
  void set_media_player(media_player::MediaPlayer *mp) { media_player_ = mp; }

  // Nightlight sensors (all optional — nullptr = not configured)
  void set_night_sensor(binary_sensor::BinarySensor *s) { night_sensor_ = s; }
  void set_sleep_sensor(binary_sensor::BinarySensor *s) { sleep_sensor_ = s; }
  void set_away_sensor(binary_sensor::BinarySensor *s) { away_sensor_ = s; }

  // Select exposed in HA for theme switching
  void set_theme_select(select::Select *s) { theme_select_ = s; }

  // Theme & sound pack registration
  void add_theme(Theme t) { themes_.push_back(std::move(t)); }
  void add_sound_pack(SoundPack sp) { sound_packs_.push_back(std::move(sp)); }
  void set_initial_theme(const std::string &name) { initial_theme_ = name; }

  // API connected binary sensor (for offline relay control fallback)
  void set_api_connected(binary_sensor::BinarySensor *s) { api_connected_ = s; }

  // button_on_time in ms
  void set_button_on_time_ms(uint32_t ms) { button_on_time_ms_ = ms; }
  // touch LED display duration in ms
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
  bool nightlight_should_be_on_() const;
  void refresh_nightlight_();
  void refresh_led_default_();
  void apply_touch_led_(const Color3 &color, float brightness, const std::string &effect);
  void apply_button_led_(uint8_t idx, bool relay_on);
  void apply_nightlight_led_();
  void cancel_touch_led_timer_();

  const Theme *active_theme_() const;
  const SoundPack *active_sound_pack_() const;
  void play_sound_(audio::AudioFile *file);

  uint8_t button_count_{3};
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

  select::Select *theme_select_{nullptr};

  std::vector<Theme> themes_;
  std::vector<SoundPack> sound_packs_;
  std::string initial_theme_{"Default"};

  bool nightlight_on_{false};
  bool touch_led_active_{false};
  uint32_t touch_led_start_ms_{0};
  uint32_t button_on_time_ms_{500};
  uint32_t touch_led_duration_ms_{6000};
};

}  // namespace tx_ultimate_switch
}  // namespace esphome

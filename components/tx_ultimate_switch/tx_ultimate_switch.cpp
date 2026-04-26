#include "tx_ultimate_switch.h"
#include "esphome/core/log.h"
#include "esphome/core/application.h"
#ifdef USE_AUDIO
#include "esphome/components/audio/audio.h"
#include "esphome/components/media_player/media_player.h"
#ifdef USE_ESP32
#include "esphome/components/speaker/media_player/speaker_media_player.h"
#endif
#endif  // USE_AUDIO

namespace esphome {
namespace tx_ultimate_switch {

static const char *TAG = "tx_ultimate_switch";

// ── Lifecycle ─────────────────────────────────────────────────────────────────

void TxUltimateSwitch::setup() {
  ESP_LOGI(TAG, "TX Ultimate Switch setup (buttons=%d, themes=%d)",
           buttons_.size(), themes_.size());

  // Wire up nightlight sensor callbacks
  if (night_sensor_ != nullptr) {
    night_sensor_->add_on_state_callback([this](bool) { refresh_nightlight_(); });
  }
  if (sleep_sensor_ != nullptr) {
    sleep_sensor_->add_on_state_callback([this](bool) { refresh_nightlight_(); });
  }
  if (away_sensor_ != nullptr) {
    away_sensor_->add_on_state_callback([this](bool) { refresh_nightlight_(); });
  }

  // Wire relay state callbacks
  // (state changes are detected in loop() instead of listener interface)

  // Wire state sensor callbacks (LED follows external state when present)
  for (auto &btn : buttons_) {
    if (btn.state_sensor != nullptr) {
      btn.state_sensor->add_on_state_callback([this](bool) { refresh_led_default_(); });
    }
  }

  // Wire theme select callback
  if (theme_select_ != nullptr) {
    theme_select_->add_on_state_callback([this](size_t) {
      refresh_led_default_();
    });
  }

  // Initial state
  refresh_nightlight_();
}

void TxUltimateSwitch::dump_config() {  ESP_LOGCONFIG(TAG, "TX Ultimate Switch:");
  ESP_LOGCONFIG(TAG, "  Buttons: %d", buttons_.size());
  ESP_LOGCONFIG(TAG, "  Themes: %d", themes_.size());
  ESP_LOGCONFIG(TAG, "  Night sensor: %s", night_sensor_ != nullptr ? "configured" : "not configured");
  ESP_LOGCONFIG(TAG, "  Sleep sensor: %s", sleep_sensor_ != nullptr ? "configured" : "not configured");
  ESP_LOGCONFIG(TAG, "  Away sensor:  %s", away_sensor_  != nullptr ? "configured" : "not configured");
  ESP_LOGCONFIG(TAG, "  Room type:    %s",
                room_type_ == RoomType::BEDROOM ? "bedroom" :
                room_type_ == RoomType::DARK    ? "dark"    : "standard");
}

// ── Loop (relay state change detection) ──────────────────────────────────────

void TxUltimateSwitch::loop() {
  // When the LED reflects the relay (no external state_sensor), poll relay state
  // to detect external changes (e.g., HA toggling the relay light directly).
  for (auto &btn : buttons_) {
    if (btn.relay == nullptr || btn.state_sensor != nullptr) continue;
    bool current = btn.relay->current_values.is_on();
    if (current != btn.last_relay_state) {
      btn.last_relay_state = current;
      refresh_led_default_();
    }
  }
}

// ── Nightlight logic ──────────────────────────────────────────────────────────

NightlightMode TxUltimateSwitch::compute_nightlight_mode_() const {
  // night_sensor is REQUIRED — if not configured or not active, no nightlight
  if (night_sensor_ == nullptr || !night_sensor_->state)
    return NightlightMode::OFF;

  // away_sensor: nightlight off when nobody is home
  if (away_sensor_ != nullptr && away_sensor_->state)
    return NightlightMode::OFF;

  // sleep_sensor: someone in the household is asleep
  if (sleep_sensor_ != nullptr && sleep_sensor_->state) {
    // Bedroom → turn off completely so sleeper isn’t disturbed
    // Hallway/other → dim guide colour to help navigation without waking anyone
    return (room_type_ == RoomType::BEDROOM) ? NightlightMode::OFF : NightlightMode::SLEEP;
  }

  return NightlightMode::NORMAL;
}

void TxUltimateSwitch::refresh_nightlight_() {
  NightlightMode mode = compute_nightlight_mode_();
  if (nightlight_mode_ != mode) {
    nightlight_mode_ = mode;
    ESP_LOGD(TAG, "Nightlight mode: %s",
             mode == NightlightMode::OFF   ? "OFF"   :
             mode == NightlightMode::SLEEP ? "SLEEP" : "NORMAL");
  }
  refresh_led_default_();
}

// ── LED state machine ─────────────────────────────────────────────────────────

void TxUltimateSwitch::refresh_led_default_() {
  if (touch_led_active_) return;  // Touch animation has priority, skip

  // Turn off top LEDs (touch/swipe feedback zone). Setting the effect to
  // "None" is required: turn_off() alone keeps the previously-set effect
  // pointer alive on the LightState, so its update() keeps drawing.
  if (leds_top_ != nullptr) {
    auto call = leds_top_->turn_off();
    call.set_effect("None");
    call.set_transition_length(0);
    call.perform();
  }

  // Nightlight zone
  if (nightlight_mode_ != NightlightMode::OFF) {
    apply_nightlight_led_();
  } else {
    if (leds_nightlight_ != nullptr) {
      auto call = leds_nightlight_->turn_off();
      call.set_effect("None");
      call.set_transition_length(0);
      call.perform();
    }
  }

  // Button LEDs — drive each by position; state_sensor wins over relay
  for (auto &btn : buttons_) {
    bool state_on;
    if (btn.state_sensor != nullptr) {
      state_on = btn.state_sensor->state;
    } else if (btn.relay != nullptr) {
      state_on = btn.relay->current_values.is_on();
    } else {
      continue;  // no state source → skip
    }
    apply_button_led_(static_cast<uint8_t>(btn.position), state_on);
  }
}

void TxUltimateSwitch::apply_nightlight_led_() {
  const Theme *t = active_theme_();
  if (t == nullptr || leds_nightlight_ == nullptr) return;

  auto call = leds_nightlight_->turn_on();

  const Color3     &color      = (nightlight_mode_ == NightlightMode::SLEEP) ? t->sleep_color      : t->nightlight_color;
  float             brightness  = (nightlight_mode_ == NightlightMode::SLEEP) ? t->sleep_brightness : t->nightlight_brightness;
  const std::string &effect     = (nightlight_mode_ == NightlightMode::SLEEP) ? t->sleep_effect     : t->nightlight_effect;

  call.set_brightness(brightness);
  call.set_red(color.r / 100.0f);
  call.set_green(color.g / 100.0f);
  call.set_blue(color.b / 100.0f);
  if (!effect.empty() && effect != "None") {
    call.set_effect(effect);
  } else {
    call.set_effect("None");
  }
  call.perform();
}

void TxUltimateSwitch::apply_button_led_(uint8_t idx, bool relay_on) {
  if (idx >= 3 || leds_button_[idx] == nullptr) return;
  const Theme *t = active_theme_();
  if (t == nullptr) return;

  auto call = leds_button_[idx]->turn_on();
  if (relay_on) {
    call.set_brightness(t->button_brightness);
    call.set_red(t->button_color.r / 100.0f);
    call.set_green(t->button_color.g / 100.0f);
    call.set_blue(t->button_color.b / 100.0f);
    call.set_effect("None");
  } else if (nightlight_mode_ != NightlightMode::OFF) {
    // Button OFF + nightlight active: mirror the active nightlight colour on button LED
    const Color3     &nl_color      = (nightlight_mode_ == NightlightMode::SLEEP) ? t->sleep_color      : t->nightlight_color;
    float             nl_brightness  = (nightlight_mode_ == NightlightMode::SLEEP) ? t->sleep_brightness : t->nightlight_brightness;
    const std::string &nl_effect     = (nightlight_mode_ == NightlightMode::SLEEP) ? t->sleep_effect     : t->nightlight_effect;

    call.set_brightness(nl_brightness);
    call.set_red(nl_color.r / 100.0f);
    call.set_green(nl_color.g / 100.0f);
    call.set_blue(nl_color.b / 100.0f);
    if (!nl_effect.empty() && nl_effect != "None") {
      call.set_effect(nl_effect);
    } else {
      call.set_effect("None");
    }
  } else {
    auto off = leds_button_[idx]->turn_off();
    off.set_effect("None");
    off.set_transition_length(0);
    off.perform();
    return;
  }
  call.perform();
}

void TxUltimateSwitch::apply_touch_led_(const Color3 &color, float brightness,
                                         const std::string &effect,
                                         uint32_t duration_ms) {
  if (leds_top_ == nullptr) return;
  auto call = leds_top_->turn_on();
  call.set_brightness(brightness);
  call.set_red(color.r / 100.0f);
  call.set_green(color.g / 100.0f);
  call.set_blue(color.b / 100.0f);
  call.set_transition_length(0);
  if (!effect.empty() && effect != "None") {
    call.set_effect(effect);
  } else {
    call.set_effect("None");
  }
  call.perform();

  touch_led_active_ = true;
  touch_led_start_ms_ = millis();

  // Fallback reset (in case no release/swipe event arrives to clear it sooner).
  App.scheduler.set_timeout(this, "touch_led_reset", duration_ms, [this]() {
    clear_touch_led_();
  });
}

void TxUltimateSwitch::cancel_touch_led_timer_() {
  App.scheduler.cancel_timeout(this, "touch_led_reset");
}

void TxUltimateSwitch::clear_touch_led_() {
  cancel_touch_led_timer_();
  touch_led_active_ = false;
  refresh_led_default_();
}

// ── Touch event handlers ──────────────────────────────────────────────────────

void TxUltimateSwitch::on_touch_press() {
  const Theme *t = active_theme_();
  if (t == nullptr) return;
  apply_touch_led_(t->touch_color, t->touch_brightness, t->touch_effect,
                    touch_led_duration_ms_);
  ESP_LOGD(TAG, "Touch press");
}

void TxUltimateSwitch::on_touch_release(int pos) {
  // Cancel the touch effect started by on_touch_press immediately on release
  // (legacy behaviour: the touchfield BS chain triggered refresh_led_default).
  clear_touch_led_();

  // Vibration
  if (vibra_ != nullptr) vibra_->turn_on();

  // Sound
  const SoundPack *sp = active_sound_pack_();
  if (sp != nullptr) play_sound_(sp->click);

  if (buttons_.empty()) return;

  // Route to a button by computing the target position from touch x.
  if (upside_down_) pos = 10 - pos;

  ButtonPosition target_pos;
  size_t n = buttons_.size();
  if (n == 1) {
    // Single button: full surface routes to it regardless of declared position.
    target_pos = buttons_[0].position;
  } else if (n == 2) {
    // 50/50 split, ordered by position (left < center < right).
    ButtonPosition p0 = buttons_[0].position;
    ButtonPosition p1 = buttons_[1].position;
    ButtonPosition leftmost  = (static_cast<int>(p0) < static_cast<int>(p1)) ? p0 : p1;
    ButtonPosition rightmost = (static_cast<int>(p0) < static_cast<int>(p1)) ? p1 : p0;
    target_pos = (pos <= 5) ? leftmost : rightmost;
  } else {
    // 3 buttons: strict thirds.
    if      (pos <= 3) target_pos = ButtonPosition::LEFT;
    else if (pos <= 7) target_pos = ButtonPosition::CENTER;
    else               target_pos = ButtonPosition::RIGHT;
  }

  ButtonConfig *btn = button_at_position_(target_pos);
  if (btn == nullptr) {
    ESP_LOGD(TAG, "Release pos=%d -> no button at position %d", pos, (int) target_pos);
    return;
  }

  // Decide whether to toggle the relay locally based on mode.
  bool should_toggle = false;
  switch (btn->mode) {
    case SwitchRelayMode::ALWAYS:
      should_toggle = true;
      break;
    case SwitchRelayMode::NEVER:
      should_toggle = false;
      break;
    case SwitchRelayMode::FALLBACK_HA:
      should_toggle = (api_connected_ == nullptr || !api_connected_->state);
      break;
    case SwitchRelayMode::FALLBACK_WIFI:
      should_toggle = (wifi_connected_ == nullptr || !wifi_connected_->state);
      break;
  }
  if (should_toggle && btn->relay != nullptr) {
    auto call = btn->relay->toggle();
    call.perform();
  }

  // Publish the press sensor (visible to HA) — pulse ON, then OFF after button_on_time_ms.
  if (btn->press_sensor != nullptr) {
    auto *bs = btn->press_sensor;
    bs->publish_state(true);
    // Unique timer id per button so retriggers extend OFF without cross-button interference.
    uint32_t timer_id = static_cast<uint32_t>(btn - buttons_.data());
    App.scheduler.set_timeout(this, timer_id, button_on_time_ms_, [bs]() {
      bs->publish_state(false);
    });
  }

  ESP_LOGD(TAG, "Release pos=%d -> position %d, toggle=%d", pos, (int) target_pos, should_toggle);
}

// Swipe / long / multi: short flash (button_on_time), not the full 6s touch fallback.
void TxUltimateSwitch::on_swipe_left() {
  if (vibra_ != nullptr) vibra_->turn_on();
  const Theme *t = active_theme_();
  const SoundPack *sp = active_sound_pack_();
  if (t != nullptr) apply_touch_led_(t->swipe_left_color, t->swipe_left_brightness,
                                      t->swipe_left_effect, button_on_time_ms_);
  if (sp != nullptr) play_sound_(sp->slide);
  ESP_LOGD(TAG, "Swipe left");
}

void TxUltimateSwitch::on_swipe_right() {
  if (vibra_ != nullptr) vibra_->turn_on();
  const Theme *t = active_theme_();
  const SoundPack *sp = active_sound_pack_();
  if (t != nullptr) apply_touch_led_(t->swipe_right_color, t->swipe_right_brightness,
                                      t->swipe_right_effect, button_on_time_ms_);
  if (sp != nullptr) play_sound_(sp->slide);
  ESP_LOGD(TAG, "Swipe right");
}

void TxUltimateSwitch::on_full_touch_release() {
  if (vibra_ != nullptr) vibra_->turn_on();
  const Theme *t = active_theme_();
  const SoundPack *sp = active_sound_pack_();
  if (t != nullptr) apply_touch_led_(t->multi_touch_color, t->multi_touch_brightness,
                                      t->multi_touch_effect, button_on_time_ms_);
  if (sp != nullptr) play_sound_(sp->multi_press);
  ESP_LOGD(TAG, "Full touch release");
}

void TxUltimateSwitch::on_long_touch_release() {
  if (vibra_ != nullptr) vibra_->turn_on();
  const Theme *t = active_theme_();
  const SoundPack *sp = active_sound_pack_();
  if (t != nullptr) apply_touch_led_(t->long_press_color, t->long_press_brightness,
                                      t->long_press_effect, button_on_time_ms_);
  if (sp != nullptr) play_sound_(sp->long_press);
  ESP_LOGD(TAG, "Long touch release");
}

// ── Fallback mode (ESP-NOW / HA offline visual indicator) ────────────────────

void TxUltimateSwitch::enter_fallback_mode() {
  cancel_touch_led_timer_();
  touch_led_active_ = true;  // Block normal nightlight refresh

  const Theme *t = active_theme_();
  if (t == nullptr || leds_top_ == nullptr) return;

  auto call = leds_top_->turn_on();
  call.set_brightness(t->touch_brightness);
  call.set_red(t->touch_color.r / 100.0f);
  call.set_green(t->touch_color.g / 100.0f);
  call.set_blue(t->touch_color.b / 100.0f);
  if (!t->touch_effect.empty() && t->touch_effect != "None") {
    call.set_effect(t->touch_effect);
  }
  call.perform();
  ESP_LOGD(TAG, "Fallback mode entered");
}

void TxUltimateSwitch::exit_fallback_mode() {
  touch_led_active_ = false;
  cancel_touch_led_timer_();
  refresh_led_default_();
  ESP_LOGD(TAG, "Fallback mode exited");
}

// ── Relay init (called on boot + API reconnect) ───────────────────────────────

void TxUltimateSwitch::init_relays() {
  for (uint8_t i = 0; i < buttons_.size(); i++) {
    auto &btn = buttons_[i];
    if (btn.mode == SwitchRelayMode::NEVER && btn.relay != nullptr) {
      ESP_LOGD(TAG, "init_relays: forcing relay %d ON (mode=NEVER)", i);
      auto call = btn.relay->turn_on();
      call.perform();
    }
  }
}

ButtonConfig *TxUltimateSwitch::button_at_position_(ButtonPosition pos) {
  for (auto &b : buttons_) {
    if (b.position == pos) return &b;
  }
  return nullptr;
}

// ── Theme & sound helpers ─────────────────────────────────────────────────────

const Theme *TxUltimateSwitch::active_theme_() const {
  if (themes_.empty()) return nullptr;

  // If a select is configured, use its current value
  if (theme_select_ != nullptr) {
    const auto &active = theme_select_->current_option();
    for (auto &t : themes_) {
      if (t.name == active) return &t;
    }
  }

  // Fallback: first theme
  return &themes_[0];
}

const SoundPack *TxUltimateSwitch::active_sound_pack_() const {
  const Theme *t = active_theme_();
  if (t == nullptr || sound_packs_.empty()) return nullptr;

  for (auto &sp : sound_packs_) {
    if (sp.name == t->sound_pack) return &sp;
  }
  // Fallback: first pack
  return &sound_packs_[0];
}

#ifdef USE_AUDIO
void TxUltimateSwitch::play_sound_(audio::AudioFile *file) {
  if (file == nullptr || media_player_ == nullptr) return;

  // Don't play during sleep (avoid waking people)
  if (sleep_sensor_ != nullptr && sleep_sensor_->state) return;

#ifdef USE_ESP32
  auto *mp = static_cast<speaker::SpeakerMediaPlayer *>(media_player_);
  mp->make_call()
    .set_command(media_player::MediaPlayerCommand::MEDIA_PLAYER_COMMAND_STOP)
    .set_announcement(true)
    .perform();
  if (media_player_->state != media_player::MediaPlayerState::MEDIA_PLAYER_STATE_ANNOUNCING) {
    mp->play_file(file, true, false);
  }
#endif
}
#else
void TxUltimateSwitch::play_sound_(audio::AudioFile *file) {}
#endif  // USE_AUDIO

}  // namespace tx_ultimate_switch
}  // namespace esphome

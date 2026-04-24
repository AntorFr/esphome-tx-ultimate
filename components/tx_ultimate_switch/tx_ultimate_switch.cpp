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
  ESP_LOGI(TAG, "TX Ultimate Switch setup (button_count=%d, themes=%d)",
           button_count_, themes_.size());

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
  ESP_LOGCONFIG(TAG, "  Button count: %d", button_count_);
  ESP_LOGCONFIG(TAG, "  Themes: %d", themes_.size());
  ESP_LOGCONFIG(TAG, "  Night sensor: %s", night_sensor_ != nullptr ? "configured" : "not configured");
  ESP_LOGCONFIG(TAG, "  Sleep sensor: %s", sleep_sensor_ != nullptr ? "configured" : "not configured");
  ESP_LOGCONFIG(TAG, "  Away sensor:  %s", away_sensor_  != nullptr ? "configured" : "not configured");
}

// ── Loop (relay state change detection) ──────────────────────────────────────

void TxUltimateSwitch::loop() {
  for (uint8_t i = 0; i < button_count_ && i < buttons_.size(); i++) {
    auto &btn = buttons_[i];
    if (btn.relay == nullptr || !btn.switch_relay) continue;
    bool current = btn.relay->current_values.is_on();
    if (current != btn.last_relay_state) {
      btn.last_relay_state = current;
      refresh_led_default_();
    }
  }
}

// ── Nightlight logic ──────────────────────────────────────────────────────────

bool TxUltimateSwitch::nightlight_should_be_on_() const {
  // night_sensor is REQUIRED — if not configured, nightlight is never active
  if (night_sensor_ == nullptr) return false;
  if (!night_sensor_->state) return false;

  // sleep_sensor is optional inhibitor — if configured and ON, nightlight off
  if (sleep_sensor_ != nullptr && sleep_sensor_->state) return false;

  // away_sensor is optional inhibitor — if configured and ON, nightlight off
  if (away_sensor_ != nullptr && away_sensor_->state) return false;

  return true;
}

void TxUltimateSwitch::refresh_nightlight_() {
  bool should_be_on = nightlight_should_be_on_();
  if (nightlight_on_ != should_be_on) {
    nightlight_on_ = should_be_on;
    ESP_LOGD(TAG, "Nightlight: %s", nightlight_on_ ? "ON" : "OFF");
  }
  refresh_led_default_();
}

// ── LED state machine ─────────────────────────────────────────────────────────

void TxUltimateSwitch::refresh_led_default_() {
  if (touch_led_active_) return;  // Touch animation has priority, skip

  // Turn off top LEDs (touch/swipe feedback zone)
  if (leds_top_ != nullptr) {
    auto call = leds_top_->turn_off();
    call.perform();
  }

  // Nightlight zone
  if (nightlight_on_) {
    apply_nightlight_led_();
  } else {
    if (leds_nightlight_ != nullptr) {
      auto call = leds_nightlight_->turn_off();
      call.perform();
    }
  }

  // Button LEDs
  for (uint8_t i = 0; i < button_count_ && i < buttons_.size(); i++) {
    auto &btn = buttons_[i];
    if (!btn.switch_relay || btn.relay == nullptr) continue;
    bool relay_on = btn.relay->current_values.is_on();
    apply_button_led_(i, relay_on);
  }
}

void TxUltimateSwitch::apply_nightlight_led_() {
  const Theme *t = active_theme_();
  if (t == nullptr || leds_nightlight_ == nullptr) return;

  auto call = leds_nightlight_->turn_on();
  call.set_brightness(t->nightlight_brightness);
  call.set_red(t->nightlight_color.r / 100.0f);
  call.set_green(t->nightlight_color.g / 100.0f);
  call.set_blue(t->nightlight_color.b / 100.0f);
  if (!t->nightlight_effect.empty() && t->nightlight_effect != "None") {
    call.set_effect(t->nightlight_effect);
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
  } else if (nightlight_on_) {
    // Button OFF + nightlight active: show nightlight colour on button LED
    call.set_brightness(t->nightlight_brightness);
    call.set_red(t->nightlight_color.r / 100.0f);
    call.set_green(t->nightlight_color.g / 100.0f);
    call.set_blue(t->nightlight_color.b / 100.0f);
    if (!t->nightlight_effect.empty() && t->nightlight_effect != "None") {
      call.set_effect(t->nightlight_effect);
    } else {
      call.set_effect("None");
    }
  } else {
    auto off = leds_button_[idx]->turn_off();
    off.perform();
    return;
  }
  call.perform();
}

void TxUltimateSwitch::apply_touch_led_(const Color3 &color, float brightness,
                                         const std::string &effect) {
  if (leds_top_ == nullptr) return;
  auto call = leds_top_->turn_on();
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

  touch_led_active_ = true;
  touch_led_start_ms_ = millis();

  // Schedule reset via App loop — we register a deferred callback
  App.scheduler.set_timeout(this, "touch_led_reset", touch_led_duration_ms_, [this]() {
    touch_led_active_ = false;
    refresh_led_default_();
  });
}

// ── Touch event handlers ──────────────────────────────────────────────────────

void TxUltimateSwitch::on_touch_press() {
  const Theme *t = active_theme_();
  if (t == nullptr) return;
  apply_touch_led_(t->touch_color, t->touch_brightness, t->touch_effect);
  ESP_LOGD(TAG, "Touch press");
}

void TxUltimateSwitch::on_touch_release(int pos) {
  // Vibration
  if (vibra_ != nullptr) vibra_->turn_on();

  // Sound
  const SoundPack *sp = active_sound_pack_();
  if (sp != nullptr) play_sound_(sp->click);

  // Route to button
  uint8_t btn_idx = 0;
  if (button_count_ == 1) {
    btn_idx = 0;
  } else if (button_count_ == 2) {
    btn_idx = (pos <= 5) ? 0 : 1;
  } else {
    if (pos <= 3) btn_idx = 0;
    else if (pos <= 7) btn_idx = 1;
    else btn_idx = 2;
  }

  if (btn_idx < buttons_.size()) {
    auto &btn = buttons_[btn_idx];
    bool offline = (api_connected_ == nullptr || !api_connected_->state);
    if (btn.switch_relay || offline) {
      if (btn.relay != nullptr) {
        auto call = btn.relay->toggle();
        call.perform();
      }
    }
  }
  ESP_LOGD(TAG, "Release pos=%d -> button %d", pos, btn_idx);
}

void TxUltimateSwitch::on_swipe_left() {
  if (vibra_ != nullptr) vibra_->turn_on();
  const Theme *t = active_theme_();
  const SoundPack *sp = active_sound_pack_();
  if (t != nullptr) apply_touch_led_(t->swipe_left_color, t->swipe_left_brightness, t->swipe_left_effect);
  if (sp != nullptr) play_sound_(sp->slide);
  ESP_LOGD(TAG, "Swipe left");
}

void TxUltimateSwitch::on_swipe_right() {
  if (vibra_ != nullptr) vibra_->turn_on();
  const Theme *t = active_theme_();
  const SoundPack *sp = active_sound_pack_();
  if (t != nullptr) apply_touch_led_(t->swipe_right_color, t->swipe_right_brightness, t->swipe_right_effect);
  if (sp != nullptr) play_sound_(sp->slide);
  ESP_LOGD(TAG, "Swipe right");
}

void TxUltimateSwitch::on_full_touch_release() {
  if (vibra_ != nullptr) vibra_->turn_on();
  const Theme *t = active_theme_();
  const SoundPack *sp = active_sound_pack_();
  if (t != nullptr) apply_touch_led_(t->multi_touch_color, t->multi_touch_brightness, t->multi_touch_effect);
  if (sp != nullptr) play_sound_(sp->multi_press);
  ESP_LOGD(TAG, "Full touch release");
}

void TxUltimateSwitch::on_long_touch_release() {
  if (vibra_ != nullptr) vibra_->turn_on();
  const Theme *t = active_theme_();
  const SoundPack *sp = active_sound_pack_();
  if (t != nullptr) apply_touch_led_(t->long_press_color, t->long_press_brightness, t->long_press_effect);
  if (sp != nullptr) play_sound_(sp->long_press);
  ESP_LOGD(TAG, "Long touch release");
}

// ── Relay init (called on boot + API reconnect) ───────────────────────────────

void TxUltimateSwitch::init_relays() {
  for (uint8_t i = 0; i < buttons_.size(); i++) {
    auto &btn = buttons_[i];
    if (!btn.switch_relay && btn.relay != nullptr) {
      ESP_LOGD(TAG, "init_relays: forcing relay %d ON (not linked to switch)", i);
      auto call = btn.relay->turn_on();
      call.perform();
    }
  }
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

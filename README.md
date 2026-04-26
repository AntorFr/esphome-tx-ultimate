# esphome-tx-ultimate

ESPHome component for the **Sonoff TX Ultimate** touch switch — properly packaged as a reusable ESPHome external component.

## Features

- **`tx_ultimate_touch`** — UART touch panel driver (forked from [SmartHome-yourself](https://github.com/SmartHome-yourself/sonoff-tx-ultimate-for-esphome)) with configurable `long_press_x_offset`
- **`tx_ultimate_switch`** — Main switch component handling:
  - 1, 2 or 3 button routing
  - LED state machine (button ON/OFF, nightlight zone, touch feedback)
  - Nightlight with optional sleep/away inhibitors
  - Theme system (button/nightlight/touch/swipe/long press/multi-touch colours + effects)
  - Sound packs via the ESPHome I2S media player
  - Vibration motor feedback
  - Offline relay control (works without Home Assistant)
- **Hardware package** — `packages/tx_ultimate_hw.yaml` declares all ESP32 hardware (PSRAM, UART, I2S, GPIO, LED strip, speaker pipeline)
- **3 built-in themes** — Default, Christmas, Explore the Stars

---

## Repository structure

```
components/
  tx_ultimate_touch/     # Touch panel UART driver
    __init__.py
    tx_ultimate_touch.h
    tx_ultimate_touch.cpp
  tx_ultimate_switch/    # Main switch component
    __init__.py
    tx_ultimate_switch.h
    tx_ultimate_switch.cpp
packages/
  tx_ultimate_hw.yaml    # Hardware package (include in every device YAML)
  themes/
    default.yaml         # Default theme
    christmas.yaml       # Christmas theme
    explore_the_stars.yaml
example/
  my_switch_3btn.yaml    # Complete 3-button device example
tests/
  test_1_touch.yaml      # Schema test: touch component only
  test_2_switch_no_audio.yaml   # Schema test: switch without audio
  test_3_switch_full.yaml       # Schema test: full config with audio
```

---

## Quick start

### 1. Minimum viable YAML (3 buttons, no sound)

```yaml
esphome:
  name: my-switch

packages:
  hw: github://AntorFr/esphome-tx-ultimate/packages/tx_ultimate_hw.yaml@main
  theme: github://AntorFr/esphome-tx-ultimate/packages/themes/default.yaml@main

wifi:
  ssid: !secret wifi_ssid
  password: !secret wifi_password

api:
ota:
  - platform: esphome

tx_ultimate_switch:
  id: tx
  button_count: 3
  leds: leds
  leds_top: leds_top
  leds_nightlight: leds_nightlight
  leds_buttons:
    right: leds_button_right
    middle: leds_button_middle
    left: leds_button_left
  vibra: vibra
  api_connected: api_connected
  buttons:
    - switch_relay: true
      relay: relay_1
    - switch_relay: true
      relay: relay_2
    - switch_relay: true
      relay: relay_3
  active_theme: "Default"
```

### 2. Full example with nightlight and sound

See [example/my_switch_3btn.yaml](example/my_switch_3btn.yaml).

---

## tx_ultimate_switch configuration

### Required keys

| Key | Type | Description |
|-----|------|-------------|
| `button_count` | int 1–3 | Number of physical buttons |
| `leds` | light id | Full LED strip (28 LEDs) |
| `leds_top` | light id | Top LEDs (touch feedback, LEDs 20–26) |
| `leds_nightlight` | light id | Nightlight zone (LEDs 0–19 + 27) |
| `leds_buttons` | mapping | Per-button LEDs, keyed by position (`right`, `middle`, `left`) |

### Optional keys

| Key | Type | Default | Description |
|-----|------|---------|-------------|
| `buttons` | list | `[]` | Per-button config (see below) |
| `vibra` | switch id | — | Vibration motor switch |
| `media_player` | media_player id | — | I2S speaker media player for sounds |
| `api_connected` | binary_sensor id | — | API connected sensor (for offline fallback) |
| `nightlight` | nightlight block | — | Nightlight sensors |
| `themes` | list | `[]` | Theme definitions |
| `sound_packs` | list | `[]` | Sound pack definitions |
| `active_theme` | string | `"Default"` | Initial theme name |
| `button_on_time` | time | `500ms` | Touch field hold time |
| `touch_led_duration` | time | `6s` | Touch LED feedback duration |

### Button config

| Key | Type | Default | Description |
|-----|------|---------|-------------|
| `switch_relay` | bool | `true` | `true` = button toggles the relay; `false` = relay stays ON, button fires touch event only |
| `relay` | light id | — | Binary output light driving the relay |
| `state_sensor` | binary_sensor id | — | External state source for button LED feedback (useful when `switch_relay: false`) |

```yaml
buttons:
  - switch_relay: true    # true = button controls the relay
    relay: relay_1        # light id of the binary output light
```

When `switch_relay: false`, the relay is forced ON at boot and the button only fires the touch event (use for scenes/scripts via HA automations).

When `switch_relay: false`, you can still get button LED state feedback by providing a `state_sensor` — a binary sensor whose state mirrors the controlled device (e.g. a Hue light polled from Home Assistant):

```yaml
binary_sensor:
  - platform: homeassistant
    id: hue_light_state
    entity_id: light.office_hue   # HA exposes lights as binary sensors (on/off)

tx_ultimate_switch:
  buttons:
    button_1:
      switch_relay: false
      relay: relay_1
      state_sensor: hue_light_state   # drives button LED feedback
```

### Nightlight config

```yaml
nightlight:
  sensor: night_sensor        # required — HA binary sensor that is ON at night
  sleep_sensor: sleep_sensor  # optional — mutes sounds; behaviour depends on room_type
  away_sensor: away_sensor    # optional — disables nightlight when away
  room_type: standard         # standard | bedroom | dark (default: standard)
```

#### `room_type` values

| Value | Nightlight behaviour |
|-------|---------------------|
| `standard` | Nightlight follows `sensor`; when `sleep_sensor` is ON → sleep colour/effect guide |
| `bedroom` | Nightlight follows `sensor`; when `sleep_sensor` is ON → LED off (do not disturb) |
| `dark` | Nightlight always ON (room has no windows); sleep/away sensors still respected |

---

## Theme configuration

Themes can be declared inline or loaded from packages. Each package appends to the `themes` list (ESPHome merges lists across packages).

```yaml
tx_ultimate_switch:
  themes:
    - name: "My Theme"
      button_color: [0, 0, 100]          # [r, g, b] in range 0–100
      button_brightness: 0.7
      nightlight_color: [80, 70, 0]
      nightlight_brightness: 0.2
      nightlight_effect: "None"           # ESPHome light effect name
      touch_color: [0, 100, 100]
      touch_brightness: 1.0
      touch_effect: "Scan"
      swipe_left_color: [0, 100, 0]
      swipe_left_brightness: 1.
      swipe_left_effect: "None"
      swipe_right_color: [100, 0, 70]
      swipe_right_brightness: 1.0
      swipe_right_effect: "None"
      long_press_color: [100, 0, 0]
      long_press_brightness: 1.0
      long_press_effect: "None"
      multi_touch_color: [0, 0, 0]
      multi_touch_brightness: 1.0
      multi_touch_effect: "Rainbow"
      sleep_color: [60, 0, 0]             # nightlight colour in sleep mode
      sleep_brightness: 0.08
      sleep_effect: "None"
      sound: explore_the_stars            # sound pack name
```

### Built-in themes

| Package | Theme name | Nightlight effect | Sound |
|---------|-----------|-------------------|-------|
| `packages/themes/default.yaml` | Default | None | explore_the_stars |
| `packages/themes/christmas.yaml` | Christmas | Christmas | explore_the_stars |
| `packages/themes/explore_the_stars.yaml` | Explore the Stars | Stars | explore_the_stars |

---

## Sound pack configuration

Sound files are declared in the `media_player` `files:` section, then referenced by ID in `sound_packs:`.

```yaml
media_player:
  - platform: speaker
    files:
      - id: snd_click
        file: https://example.com/click.flac

tx_ultimate_switch:
  sound_packs:
    - name: my_pack
      click: snd_click
      long_press: snd_long_press
      multi_press: snd_multi_press
      slide: snd_slide
```

---

## tx_ultimate_touch configuration

```yaml
tx_ultimate_touch:
  id: tx_touch
  uart_id: my_uart
  long_press_x_offset: 16   # optional, default 16
  on_press:
    - lambda: id(tx).on_touch_press();
  on_release:
    - lambda: id(tx).on_touch_release(touch.x);
  on_swipe_left:
    - lambda: id(tx).on_swipe_left();
  on_swipe_right:
    - lambda: id(tx).on_swipe_right();
  on_full_touch_release:
    - lambda: id(tx).on_full_touch_release();
  on_long_touch_release:
    - lambda: id(tx).on_long_touch_release();
```

### `long_press_x_offset`

The TX Ultimate hardware signals a long press by **adding an offset (default: 16) to the position byte** in the UART frame — there is no configurable hold duration. Change `long_press_x_offset` only if you have a hardware variant that uses a different offset.

---

## Hardware pinout (TX Ultimate)

| Signal | GPIO |
|--------|------|
| Relay 1 | GPIO18 |
| Relay 2 | GPIO17 |
| Relay 3 | GPIO27 |
| Relay 4 | GPIO23 |
| LED strip (WS2812) | GPIO13 (28 LEDs) |
| Touch UART TX | GPIO19 |
| Touch UART RX | GPIO22 |
| Vibration motor | GPIO21 |
| PA power | GPIO26 |
| Touch panel power | GPIO5 |
| I2S LRCLK | GPIO4 |
| I2S BCLK | GPIO2 |
| I2S DATA | GPIO15 |

### LED layout

```
Positions 0–19 + 27 → leds_nightlight (bottom/side strip)
Positions 20–26     → leds_top (touch feedback, 7 LEDs)
Position 7          → leds_button_right
Position 9          → leds_button_middle
Position 11         → leds_button_left
```

---

## Development

### Setup

```bash
git clone https://github.com/AntorFr/esphome-tx-ultimate
cd esphome-tx-ultimate
python3 -m venv .venv
.venv/bin/pip install "esphome>=2026.4.0"
```

### Run tests

```bash
# Test touch component schema
.venv/bin/esphome compile --only-generate tests/test_1_touch.yaml

# Test switch without audio
.venv/bin/esphome compile --only-generate tests/test_2_switch_no_audio.yaml

# Test full switch with media player and sound packs
.venv/bin/esphome compile --only-generate tests/test_3_switch_full.yaml
```

### Use local components during development

```yaml
external_components:
  - source: /path/to/esphome-tx-ultimate/components
    components: [tx_ultimate_touch, tx_ultimate_switch]
```

---

## Credits

- Touch panel UART protocol: [SmartHome-yourself/sonoff-tx-ultimate-for-esphome](https://github.com/SmartHome-yourself/sonoff-tx-ultimate-for-esphome)
- Addressable LED effects: [AntorFr/esphome-addressable-effects](https://github.com/AntorFr/esphome-addressable-effects)

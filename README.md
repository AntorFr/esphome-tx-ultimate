# esphome-tx-ultimate

ESPHome external component for the **Sonoff TX Ultimate** touch switch — properly packaged and reusable.

## Features

- **`tx_ultimate_touch`** — UART touch panel driver (forked from [SmartHome-yourself](https://github.com/SmartHome-yourself/sonoff-tx-ultimate-for-esphome)) with configurable `long_press_x_offset`.
- **`tx_ultimate_switch`** — main switch component handling:
  - 1, 2 or 3 buttons routed by **explicit position** (`left` / `center` / `right`)
  - **Inline relay declaration** per button (or reference to an external light) with full `light:binary` options (`name`, `restore_mode`, `icon`, `entity_category`, …)
  - 4 toggle modes per button: `true` / `false` / `fallback_ha` / `fallback_wifi`
  - Per-button **press sensor** exposed to Home Assistant (replaces the legacy `Touchfield 1/2/3` entities, fully configurable)
  - LED state machine (button ON/OFF, nightlight zone, touch feedback)
  - Nightlight with optional sleep/away inhibitors and `room_type` semantics
  - Theme system (button / nightlight / sleep / touch / swipe / long press / multi-touch — colours, brightness, effects)
  - Sound packs via the ESPHome I2S media player
  - Vibration motor feedback
  - Offline relay control (works without Home Assistant or without WiFi)
- **Hardware package** — `packages/tx_ultimate_hw.yaml` declares all ESP32 hardware (PSRAM, UART, I2S, GPIO outputs, LED strip, speaker pipeline, connectivity sensors).
- **3 built-in themes** — Default, Christmas, Explore the Stars.

---

## Repository structure

```
components/
  tx_ultimate_touch/     # Touch panel UART driver
  tx_ultimate_switch/    # Main switch component
packages/
  tx_ultimate_hw.yaml    # Hardware package (include in every device YAML)
  themes/
    default.yaml
    christmas.yaml
    explore_the_stars.yaml
example/
  my_switch_3btn.yaml    # Complete 3-button device example
tests/
  test_1_touch.yaml             # Schema test: touch component only
  test_2_switch_no_audio.yaml   # 3 buttons, no audio
  test_3_switch_full.yaml       # Full config: audio, themes, sensors
  test_4_mixed_relay.yaml       # Mix of inline + external relay declarations
```

---

## Quick start

### Minimal 3-button device

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
  leds: leds
  vibra: vibra
  api_connected: api_connected
  wifi_connected: wifi_connected

  buttons:
    - position: left
      switch_relay: true
      relay:
        id: relay_1
        name: "L1"
        output: l1_output
    - position: center
      switch_relay: true
      relay:
        id: relay_2
        name: "L2"
        output: l2_output
    - position: right
      switch_relay: true
      relay:
        id: relay_3
        name: "L3"
        output: l3_output

  active_theme: "Default"
```

### Full example

See [example/my_switch_3btn.yaml](example/my_switch_3btn.yaml) for a 3-button device with nightlight, audio, themes, and per-button press sensors.

---

## tx_ultimate_switch configuration

### Top-level keys

| Key | Type | Required | Default | Description |
|-----|------|----------|---------|-------------|
| `buttons` | list (1-3) | yes | — | Per-button config (see below) |
| `leds` | light id | yes | — | Full LED strip (28 LEDs) — usually `leds` from the hw package |
| `upside_down` | bool | no | `false` | Set to `true` if the panel is mounted upside-down (mirrors touch x and button-LED positions) |
| `vibra` | switch id | no | — | Vibration motor — usually `vibra` from the hw package |
| `media_player` | media_player id | no | — | I2S speaker media player (required for sounds) |
| `api_connected` | binary_sensor id | no | — | API status sensor — drives `switch_relay: fallback_ha` |
| `wifi_connected` | binary_sensor id | no | — | WiFi connectivity sensor — drives `switch_relay: fallback_wifi` |
| `nightlight` | block | no | — | Nightlight sensors and room type (see below) |
| `themes` | list | no | `[]` | Theme definitions |
| `sound_packs` | list | no | `[]` | Sound packs referenced by themes |
| `active_theme` | string | no | `"Default"` | Initial theme name |
| `theme_select` | block | no | — | Exposes a HA select to change the active theme at runtime (requires at least one theme) |
| `button_on_time` | time | no | `500ms` | Press-sensor pulse width (ON → OFF) |
| `touch_led_duration` | time | no | `6s` | Touch / swipe / multi-touch LED feedback duration |

> The number of buttons is inferred from `len(buttons)` — there is no `button_count` field.

### Button config

```yaml
buttons:
  - position: right                 # left | center | right (required, must be unique per button)
    switch_relay: fallback_ha       # true | false | fallback_ha | fallback_wifi (default: fallback_ha)
    relay:                          # optional: inline declaration OR reference to existing light id
      id: relay_1                   # constrained to relay_1 | relay_2 | relay_3
      name: "L1"
      output: l1_output             # GPIO output from hw package
      restore_mode: RESTORE_DEFAULT_OFF
      icon: mdi:lightbulb
      # ... all light:binary options accepted
    state_sensor: hue_state         # optional: external binary_sensor for LED display (HA-driven state)
    sensor:                         # optional: press sensor exposed to HA (replaces Touchfield N)
      name: "Bouton droit"
      device_class: motion
      icon: mdi:gesture-tap
```

#### `position` and routing

Buttons are routed by their declared `position`, regardless of declaration order:

| Number of buttons | Routing logic |
|-------------------|---------------|
| 1 | Touch surface routes entirely to the single button (its `position` is purely cosmetic) |
| 2 | Surface split 50/50, ordered by position (`left < center < right`) |
| 3 | Strict thirds (`left` / `center` / `right`) |

When `upside_down: true`, the touch x-axis is mirrored so `position` stays user-facing.

#### `switch_relay` modes

| Value | Behaviour on press |
|-------|-------------------|
| `true` | Always toggle the relay locally |
| `false` | Never toggle locally — HA controls the bound device. The relay is **forced ON at boot and on API reconnect** (use this for always-on power supplies, e.g. a Hue bulb wired through the switch). |
| `fallback_ha` | Toggle only when `api_connected` is OFF (HA unreachable) |
| `fallback_wifi` | Toggle only when `wifi_connected` is OFF (no network) |

#### `relay` — inline vs reference

```yaml
# Inline (recommended) — declared inside the button block
relay:
  id: relay_1
  name: "L1"
  output: l1_output
  restore_mode: RESTORE_DEFAULT_ON

# Reference — relay declared elsewhere as a light:binary platform
relay: relay_1
```

Inline relays are auto-injected as `light: - platform: binary` entries during validation. The `id:` is constrained to `relay_1`, `relay_2`, or `relay_3` (mapped to GPIO outputs `l1_output`, `l2_output`, `l3_output` from the hw package). All `light:binary` options are accepted (`name`, `restore_mode`, `icon`, `entity_category`, `effects`, `on_turn_on` triggers, …).

External references coexist freely with inline declarations: handy when another component (espnow, scripts) needs to address the relay by id at the top level.

#### `sensor` — press sensor for HA

The `sensor:` block declares a `binary_sensor` that the component pulses ON for `button_on_time` on every release in the button's zone. This is the new home of the legacy `Touchfield 1 / 2 / 3` entities.

```yaml
sensor:
  name: "Bouton gauche"     # required
  id: btn_left              # optional, auto-generated
  icon: mdi:gesture-tap
  device_class: motion
  entity_category: diagnostic
  internal: false
```

Use it to drive HA automations (e.g. toggle a Hue scene from a `switch_relay: false` button).

#### `state_sensor` — external state for LED display

When the relay is not the source of truth (e.g. it's just powering a Hue bulb that HA controls), provide a `state_sensor` so the button LED reflects the actual device state. Best paired with `internal: true` on the HA-side sensor since HA already knows the state:

```yaml
binary_sensor:
  - platform: homeassistant
    id: hue_state
    entity_id: light.office_hue
    internal: true              # state already lives in HA, no need to re-expose

tx_ultimate_switch:
  buttons:
    - position: right
      switch_relay: false
      relay:
        id: relay_1
        name: "L1"
        output: l1_output
        restore_mode: RESTORE_DEFAULT_ON
      state_sensor: hue_state
      sensor:
        name: "Bouton applique"
```

If `state_sensor` is **not** provided for a button, that button LED is not used for state display and its pixel is automatically merged into `leds_nightlight`.

### Nightlight config

```yaml
nightlight:
  sensor: night_sensor        # required — HA binary sensor that is ON at night
  sleep_sensor: sleep_sensor  # optional — mutes sounds; behaviour depends on room_type
  away_sensor: away_sensor    # optional — disables nightlight when away
  room_type: standard         # standard | bedroom | dark (default: standard)
```

| `room_type` | Behaviour |
|-------------|-----------|
| `standard` | Nightlight follows `sensor`; when `sleep_sensor` is ON → dim guide colour |
| `bedroom` | Nightlight follows `sensor`; when `sleep_sensor` is ON → LED off (do not disturb), and button state display is also muted |
| `dark` | Nightlight always ON (room with no windows); sleep/away still respected |

### Theme select (optional)

If you want to switch themes directly from Home Assistant, add `theme_select` to `tx_ultimate_switch`.
Options are auto-filled from your `themes:` names.

```yaml
tx_ultimate_switch:
  id: tx
  leds: leds
  themes:
    - name: "Default"
    - name: "Christmas"
  active_theme: "Default"      # startup fallback
  theme_select:
    id: tx_theme
    name: "TX Theme"
    icon: mdi:palette
```

---

## Theme configuration

Themes can be inline or loaded from packages. Each package appends to the `themes` list (ESPHome merges lists across packages).

Each theme slot (`button`, `nightlight`, `sleep`, `touch`, `swipe_left`, `swipe_right`, `long_press`, `multi_touch`) takes a `color`, `brightness`, and an optional `effect`.

```yaml
tx_ultimate_switch:
  themes:
    - name: "My Theme"
      button:
        color: [0, 0, 100]        # [r, g, b] in range 0–100
        brightness: 0.7
      nightlight:
        color: [80, 70, 0]
        brightness: 0.2
        effect:
          type: stars             # scan | rainbow | christmas | stars
          probability: 0.3
      sleep:
        color: [60, 0, 0]
        brightness: 0.08
      touch:
        color: [0, 100, 100]
        brightness: 1.0
        effect:
          type: scan
      swipe_left:  { color: [0, 100, 0],  brightness: 1.0 }
      swipe_right: { color: [100, 0, 70], brightness: 1.0 }
      long_press:  { color: [100, 0, 0],  brightness: 1.0 }
      multi_touch:
        color: [0, 0, 0]
        brightness: 1.0
        effect:
          type: rainbow
          speed: 10
          width: 20
      sound: explore_the_stars    # sound pack name
```

### Effect types

| Type | Extra parameters |
|------|------------------|
| `scan` | *(none)* |
| `rainbow` | `speed` (int, default 10), `width` (int, default 20) |
| `christmas` | `blank_size` (int, default 1), `bit_size` (int, default 1) |
| `stars` | `probability` (float 0.0–1.0, default 0.3), `color` ([r,g,b] 0–100) |

### Built-in themes

| Package | Theme name | Nightlight effect | Sound |
|---------|-----------|-------------------|-------|
| `packages/themes/default.yaml` | Default | None | explore_the_stars |
| `packages/themes/christmas.yaml` | Christmas | Christmas | explore_the_stars |
| `packages/themes/explore_the_stars.yaml` | Explore the Stars | Stars | explore_the_stars |

---

## Sound pack configuration

Sound files are declared in the `media_player` `files:` list, then referenced by id in `sound_packs:`.

```yaml
media_player:
  - platform: speaker
    files:
      - id: snd_click
        file: https://example.com/click.flac

tx_ultimate_switch:
  sound_packs:
    - name: my_pack
      click:       snd_click
      long_press:  snd_long_press
      multi_press: snd_multi_press
      slide:       snd_slide
```

A theme references a sound pack by name via its `sound:` field.

---

## tx_ultimate_touch configuration

```yaml
tx_ultimate_touch:
  id: tx_touch
  uart_id: my_uart
  long_press_x_offset: 16   # default; change only for hardware variants
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

The TX Ultimate signals a long press by **adding an offset (default: 16) to the position byte** in the UART frame — there is no configurable hold duration. Override `long_press_x_offset` only for hardware variants that use a different offset.

---

## Hardware (TX Ultimate)

The hw package (`packages/tx_ultimate_hw.yaml`) declares everything board-specific:

- ESP32 board, PSRAM, IDF framework
- UART, I2S audio pipeline
- GPIO outputs `l1_output`, `l2_output`, `l3_output` (relay drivers — referenced by inline relay blocks in `buttons`)
- LED strip (`leds`), vibration motor (`vibra`), PA power, touch panel power
- Connectivity sensors: `api_connected`, `wifi_connected`
- Touch event sensors: `swipe_left`, `swipe_right`, `multi_touch`, `long_press` (exposed to HA)
- Boot init and 5-min nightlight refresh trigger

> Relay light entities (`relay_1`, `relay_2`, `relay_3`) are **not** declared by the hw package — declare them inline inside `tx_ultimate_switch.buttons[].relay`, or as standard `light: - platform: binary` entries if you prefer.

### Pinout

| Signal | GPIO |
|--------|------|
| Relay 1 | GPIO18 (`l1_output`) |
| Relay 2 | GPIO17 (`l2_output`) |
| Relay 3 | GPIO27 (`l3_output`) |
| Relay 4 | GPIO23 (`l4_output`) — *not bound by default; component supports `relay_1..3` only* |
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
Pixels 0–6, 8, 10, 12–27 → leds_nightlight (bottom + side, plus any button pixel without state_sensor)
Pixels 20–26 (or 0–6 if upside_down) → leds_top (touch / swipe / multi-touch feedback)
Pixel 7  → leds_button_left   (or right if upside_down), only if that button has state_sensor
Pixel 9  → leds_button_center, only if that button has state_sensor
Pixel 11 → leds_button_right  (or left if upside_down), only if that button has state_sensor
```

---

## Migration from the old schema

If you're upgrading from a pre-refactor config:

| Old | New |
|-----|-----|
| `button_count: N` | *(removed — derived from `len(buttons)`)* |
| `buttons:` as a `button_1 / button_2 / button_3` mapping | List with explicit `position:` per entry |
| `switch_relay: true` | unchanged (`true`) |
| `switch_relay: false` | unchanged (`false`) — semantics unchanged (no toggle, relay forced ON) |
| Implicit "offline fallback" with `switch_relay: false` | Explicit `switch_relay: fallback_ha` (or `fallback_wifi`) |
| `relay: relay_1` (id ref to external light) | Either keep the ref, or move the relay declaration **inline** inside the button |
| `Touchfield 1/2/3` template binary_sensors at top level | Per-button `sensor:` block |
| Relay declarations in `tx_ultimate_hw.yaml` | Removed — declare inline per button |
| `relay_N_restore_mode` substitution | `restore_mode:` directly inside the inline relay block |

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
# Touch component schema only
.venv/bin/esphome compile --only-generate tests/test_1_touch.yaml

# 3 buttons, no audio (full inline relay declarations)
.venv/bin/esphome compile --only-generate tests/test_2_switch_no_audio.yaml

# Full config: audio, themes, sound packs, per-button press sensors
.venv/bin/esphome compile --only-generate tests/test_3_switch_full.yaml

# Mix of inline + external relay declarations
.venv/bin/esphome compile --only-generate tests/test_4_mixed_relay.yaml
```

For a full C++ build, drop the `--only-generate` flag.

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

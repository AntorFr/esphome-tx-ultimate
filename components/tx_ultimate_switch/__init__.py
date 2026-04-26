import esphome.codegen as cg
import esphome.config_validation as cv
import esphome.final_validate as fv
from esphome.components import light, binary_sensor, switch, select, media_player, audio
from esphome.components.binary import light as binary_light
from esphome.const import CONF_ID, CONF_NAME, CONF_PLATFORM
from esphome import automation
from esphome.core import CORE, ID as CoreID

CODEOWNERS = ["@AntorFr"]
DEPENDENCIES = ["light", "binary_sensor"]
AUTO_LOAD = ["select", "partition"]

MULTI_CONF = True

CONF_TX_ULTIMATE_SWITCH = "tx_ultimate_switch"

# ── Top-level config keys ─────────────────────────────────────────────────────
CONF_BUTTONS           = "buttons"
CONF_LEDS              = "leds"
CONF_VIBRA             = "vibra"
CONF_MEDIA_PLAYER      = "media_player"
CONF_NIGHTLIGHT        = "nightlight"
CONF_THEMES            = "themes"
CONF_ACTIVE_THEME      = "active_theme"
CONF_SOUND_PACKS       = "sound_packs"
CONF_API_CONNECTED     = "api_connected"
CONF_WIFI_CONNECTED    = "wifi_connected"
CONF_BUTTON_ON_TIME    = "button_on_time"
CONF_TOUCH_LED_DURATION = "touch_led_duration"
CONF_UPSIDE_DOWN       = "upside_down"

# ── Nightlight config keys ────────────────────────────────────────────────────
CONF_NIGHT_SENSOR = "sensor"
CONF_SLEEP_SENSOR = "sleep_sensor"
CONF_AWAY_SENSOR  = "away_sensor"
CONF_ROOM_TYPE    = "room_type"

# ── Button config keys ────────────────────────────────────────────────────────
CONF_POSITION     = "position"
CONF_SWITCH_RELAY = "switch_relay"
CONF_RELAY        = "relay"
CONF_STATE_SENSOR = "state_sensor"
CONF_SENSOR       = "sensor"

# ── Effect config keys ────────────────────────────────────────────────────────
CONF_EFFECT_TYPE        = "type"
CONF_EFFECT_SPEED       = "speed"
CONF_EFFECT_WIDTH       = "width"
CONF_EFFECT_BLANK_SIZE  = "blank_size"
CONF_EFFECT_BIT_SIZE    = "bit_size"
CONF_EFFECT_PROBABILITY = "probability"
CONF_EFFECT_COLOR       = "color"

# ── Theme slot config keys ────────────────────────────────────────────────────
CONF_SLOT_COLOR      = "color"
CONF_SLOT_BRIGHTNESS = "brightness"
CONF_SLOT_EFFECT     = "effect"

# ── Sound pack keys ───────────────────────────────────────────────────────────
CONF_SOUND_PACK       = "sound"
CONF_SOUND_PACK_NAME  = "name"
CONF_SOUND_CLICK      = "click"
CONF_SOUND_LONG_PRESS = "long_press"
CONF_SOUND_MULTI_PRESS = "multi_press"
CONF_SOUND_SLIDE      = "slide"

# ── Internal auto-generated partition IDs (declared in schema) ────────────────
CONF_LEDS_TOP_OUTPUT_ID = "leds_top_output_id"
CONF_LEDS_TOP_STATE_ID  = "leds_top_state_id"
CONF_LEDS_NL_OUTPUT_ID  = "leds_nightlight_output_id"
CONF_LEDS_NL_STATE_ID   = "leds_nightlight_state_id"
CONF_LEDS_BTN_OUTPUT_IDS = [
    "leds_button_0_output_id",
    "leds_button_1_output_id",
    "leds_button_2_output_id",
]
CONF_LEDS_BTN_STATE_IDS = [
    "leds_button_0_state_id",
    "leds_button_1_state_id",
    "leds_button_2_state_id",
]

# ── C++ namespaced types ──────────────────────────────────────────────────────
tx_ultimate_switch_ns = cg.esphome_ns.namespace("tx_ultimate_switch")
TxUltimateSwitch = tx_ultimate_switch_ns.class_("TxUltimateSwitch", cg.Component)
Theme    = tx_ultimate_switch_ns.struct("Theme")
Color3   = tx_ultimate_switch_ns.struct("Color3")
SoundPack = tx_ultimate_switch_ns.struct("SoundPack")
RoomType = tx_ultimate_switch_ns.enum("RoomType", is_class=True)
ButtonPosition  = tx_ultimate_switch_ns.enum("ButtonPosition", is_class=True)
SwitchRelayMode = tx_ultimate_switch_ns.enum("SwitchRelayMode", is_class=True)

partitions_ns = cg.esphome_ns.namespace("partition")
PartitionLightOutput = partitions_ns.class_(
    "PartitionLightOutput", light.AddressableLight
)
AddressableSegment = partitions_ns.class_("AddressableSegment")

light_ns = cg.esphome_ns.namespace("light")
LightState       = light_ns.class_("LightState", cg.EntityBase, cg.Component)
LightRestoreMode = light_ns.enum("LightRestoreMode")

AddressableScanEffect            = light_ns.class_("AddressableScanEffect")
AddressableRainbowEffect         = light_ns.class_("AddressableRainbowLightEffect")
AddressableChristmasEffect       = light_ns.class_("AddressableChristmasEffect")
AddressableStarsEffect           = light_ns.class_("AddressableStarsEffect")
AddressableColorStarsEffectColor = light_ns.struct("AddressableColorStarsEffectColor")

ROOM_TYPE_OPTIONS = {
    "standard": RoomType.STANDARD,
    "bedroom":  RoomType.BEDROOM,
    "dark":     RoomType.DARK,
}

POSITION_OPTIONS = {
    "left":   ButtonPosition.LEFT,
    "center": ButtonPosition.CENTER,
    "right":  ButtonPosition.RIGHT,
}

SWITCH_RELAY_OPTIONS = {
    "true":          SwitchRelayMode.ALWAYS,
    "false":         SwitchRelayMode.NEVER,
    "fallback_ha":   SwitchRelayMode.FALLBACK_HA,
    "fallback_wifi": SwitchRelayMode.FALLBACK_WIFI,
}

ALLOWED_RELAY_IDS = ("relay_1", "relay_2", "relay_3")

# ── Validators ────────────────────────────────────────────────────────────────
def validate_color(value):
    if not isinstance(value, (list, tuple)):
        raise cv.Invalid("Color must be a list of 3 integers [r, g, b]")
    if len(value) != 3:
        raise cv.Invalid(
            f"Color must be exactly 3 integers [r, g, b], got {len(value)}"
        )
    return [cv.int_range(min=0, max=100)(v) for v in value]


COLOR_SCHEMA = validate_color

# ── Effect schema ─────────────────────────────────────────────────────────────
EFFECT_SCHEMA = cv.Schema(
    {
        cv.Required(CONF_EFFECT_TYPE): cv.one_of(
            "scan", "rainbow", "christmas", "stars", lower=True
        ),
        cv.Optional(CONF_EFFECT_SPEED,       default=10):  cv.int_range(min=1, max=255),
        cv.Optional(CONF_EFFECT_WIDTH,       default=20):  cv.int_range(min=1, max=255),
        cv.Optional(CONF_EFFECT_BLANK_SIZE,  default=1):   cv.int_range(min=0, max=255),
        cv.Optional(CONF_EFFECT_BIT_SIZE,    default=1):   cv.int_range(min=1, max=255),
        cv.Optional(CONF_EFFECT_PROBABILITY, default=0.3): cv.percentage,
        cv.Optional(CONF_EFFECT_COLOR, default=[100, 100, 100]): COLOR_SCHEMA,
    }
)


def _slot_schema(default_color, default_brightness, allow_effect=True):
    d = {
        cv.Optional(CONF_SLOT_COLOR,      default=default_color):      COLOR_SCHEMA,
        cv.Optional(CONF_SLOT_BRIGHTNESS, default=default_brightness): cv.percentage,
    }
    if allow_effect:
        d[cv.Optional(CONF_SLOT_EFFECT)] = cv.Any(None, EFFECT_SCHEMA)
    return cv.Schema(d)


# ── Theme schema (nested slots) ───────────────────────────────────────────────
THEME_SCHEMA = cv.Schema(
    {
        cv.Required(CONF_NAME): cv.string,
        cv.Optional("button",      default={}): _slot_schema([0,   0,   100], 0.7,  allow_effect=False),
        cv.Optional("nightlight",  default={}): _slot_schema([80,  70,  0],   0.2),
        cv.Optional("sleep",       default={}): _slot_schema([80,  0,   0],   0.1),
        cv.Optional("touch",       default={}): _slot_schema([0,   100, 100], 1.0),
        cv.Optional("swipe_left",  default={}): _slot_schema([0,   100, 0],   1.0),
        cv.Optional("swipe_right", default={}): _slot_schema([100, 0,   70],  1.0),
        cv.Optional("long_press",  default={}): _slot_schema([100, 0,   0],   1.0),
        cv.Optional("multi_touch", default={}): _slot_schema([0,   0,   0],   1.0),
        cv.Optional(CONF_SOUND_PACK, default="explore_the_stars"): cv.string,
    }
)

def _validate_switch_relay(value):
    if isinstance(value, bool):
        value = "true" if value else "false"
    return cv.enum(SWITCH_RELAY_OPTIONS, lower=True)(value)


def _validate_relay_id(value):
    """Constrain the relay id to relay_1..3 then declare it as a LightState."""
    if not isinstance(value, str):
        raise cv.Invalid(f"relay id must be a string, got {type(value).__name__}")
    if value not in ALLOWED_RELAY_IDS:
        raise cv.Invalid(
            f"relay id must be one of {ALLOWED_RELAY_IDS}, got '{value}'"
        )
    return cv.declare_id(light.LightState)(value)


# Inline relay declaration — uses binary.light.CONFIG_SCHEMA verbatim, with the
# LightState id constrained to relay_1..3. The dict will be re-injected into the
# top-level light: section during FINAL_VALIDATE so the binary.light platform's
# normal codegen + source-copy pipeline picks it up.
RELAY_INLINE_SCHEMA = binary_light.CONFIG_SCHEMA.extend(
    {
        cv.Required(CONF_ID): _validate_relay_id,
    }
)


def _validate_relay(value):
    """Accept either an id reference (string) or an inline binary-light declaration (dict)."""
    if isinstance(value, dict):
        return RELAY_INLINE_SCHEMA(value)
    return cv.use_id(light.LightState)(value)


# Inline press sensor — exposes the button press to HA (replaces Touchfield N).
SENSOR_SCHEMA = binary_sensor.binary_sensor_schema(class_=binary_sensor.BinarySensor)

BUTTON_SCHEMA = cv.Schema(
    {
        cv.Required(CONF_POSITION): cv.enum(POSITION_OPTIONS, lower=True),
        cv.Optional(CONF_SWITCH_RELAY, default="fallback_ha"): _validate_switch_relay,
        cv.Optional(CONF_RELAY): _validate_relay,
        cv.Optional(CONF_STATE_SENSOR): cv.use_id(binary_sensor.BinarySensor),
        cv.Optional(CONF_SENSOR): SENSOR_SCHEMA,
    }
)


def _validate_buttons(value):
    if not isinstance(value, list):
        raise cv.Invalid("buttons must be a list")
    if len(value) < 1 or len(value) > 3:
        raise cv.Invalid(f"buttons must have between 1 and 3 entries, got {len(value)}")

    seen_positions = set()
    seen_relay_ids = set()
    for i, btn in enumerate(value):
        pos_key = str(btn[CONF_POSITION])
        if pos_key in seen_positions:
            raise cv.Invalid(
                f"buttons[{i}]: position is already used by another button"
            )
        seen_positions.add(pos_key)

        if CONF_RELAY in btn and isinstance(btn[CONF_RELAY], dict):
            rid = str(btn[CONF_RELAY][CONF_ID])
            if rid in seen_relay_ids:
                raise cv.Invalid(
                    f"buttons[{i}]: relay id '{rid}' is already used by another button"
                )
            seen_relay_ids.add(rid)
    return value


def _final_validate(config):
    """Inject inline-declared relays into the top-level light: section as
    binary platform entries. This runs after CONFIG_SCHEMA so the relay dicts
    are already validated; we just stamp `platform: binary` on each and add
    them to full_config["light"] so ESPHome's normal pipeline copies sources
    and runs the binary.light to_code for us."""
    inline_entries = []
    for btn in config[CONF_BUTTONS]:
        relay = btn.get(CONF_RELAY)
        if isinstance(relay, dict):
            entry = dict(relay)
            entry[CONF_PLATFORM] = "binary"
            inline_entries.append(entry)

    if not inline_entries:
        return

    full = fv.full_config.get()
    light_section = full.setdefault("light", [])
    light_section.extend(inline_entries)
    # Track in CORE for cache invalidation parity with normal platform loading.
    CORE.loaded_integrations.add("binary")
    CORE.loaded_platforms.add("light/binary")


FINAL_VALIDATE_SCHEMA = _final_validate


NIGHTLIGHT_SCHEMA = cv.Schema(
    {
        cv.Optional(CONF_NIGHT_SENSOR): cv.use_id(binary_sensor.BinarySensor),
        cv.Optional(CONF_SLEEP_SENSOR): cv.use_id(binary_sensor.BinarySensor),
        cv.Optional(CONF_AWAY_SENSOR):  cv.use_id(binary_sensor.BinarySensor),
        cv.Optional(CONF_ROOM_TYPE, default="standard"): cv.enum(
            ROOM_TYPE_OPTIONS, lower=True
        ),
    }
)

SOUND_PACK_SCHEMA = cv.Schema(
    {
        cv.Required(CONF_SOUND_PACK_NAME): cv.string,
        cv.Optional(CONF_SOUND_CLICK):       cv.use_id(audio.AudioFile),
        cv.Optional(CONF_SOUND_LONG_PRESS):  cv.use_id(audio.AudioFile),
        cv.Optional(CONF_SOUND_MULTI_PRESS): cv.use_id(audio.AudioFile),
        cv.Optional(CONF_SOUND_SLIDE):       cv.use_id(audio.AudioFile),
    }
)

CONFIG_SCHEMA = cv.Schema(
    {
        cv.GenerateID(): cv.declare_id(TxUltimateSwitch),
        cv.Required(CONF_BUTTONS): cv.All(
            cv.ensure_list(BUTTON_SCHEMA),
            _validate_buttons,
        ),
        cv.Required(CONF_LEDS): cv.use_id(light.LightState),
        cv.Optional(CONF_UPSIDE_DOWN, default=False): cv.boolean,
        cv.Optional(CONF_VIBRA): cv.use_id(switch.Switch),
        cv.Optional(CONF_MEDIA_PLAYER): cv.use_id(media_player.MediaPlayer),
        cv.Optional(CONF_NIGHTLIGHT): NIGHTLIGHT_SCHEMA,
        cv.Optional(CONF_THEMES, default=[]): cv.ensure_list(THEME_SCHEMA),
        cv.Optional(CONF_ACTIVE_THEME, default="Default"): cv.string,
        cv.Optional(CONF_SOUND_PACKS, default=[]): cv.ensure_list(SOUND_PACK_SCHEMA),
        cv.Optional(CONF_API_CONNECTED): cv.use_id(binary_sensor.BinarySensor),
        cv.Optional(CONF_WIFI_CONNECTED): cv.use_id(binary_sensor.BinarySensor),
        cv.Optional(CONF_BUTTON_ON_TIME,     default="500ms"): cv.positive_time_period_milliseconds,
        cv.Optional(CONF_TOUCH_LED_DURATION, default="6s"):    cv.positive_time_period_milliseconds,
        # Internal auto-generated IDs for dynamic partition LightStates
        cv.GenerateID(CONF_LEDS_TOP_OUTPUT_ID): cv.declare_id(PartitionLightOutput),
        cv.GenerateID(CONF_LEDS_TOP_STATE_ID):  cv.declare_id(LightState),
        cv.GenerateID(CONF_LEDS_NL_OUTPUT_ID):  cv.declare_id(PartitionLightOutput),
        cv.GenerateID(CONF_LEDS_NL_STATE_ID):   cv.declare_id(LightState),
        cv.GenerateID(CONF_LEDS_BTN_OUTPUT_IDS[0]): cv.declare_id(PartitionLightOutput),
        cv.GenerateID(CONF_LEDS_BTN_STATE_IDS[0]):  cv.declare_id(LightState),
        cv.GenerateID(CONF_LEDS_BTN_OUTPUT_IDS[1]): cv.declare_id(PartitionLightOutput),
        cv.GenerateID(CONF_LEDS_BTN_STATE_IDS[1]):  cv.declare_id(LightState),
        cv.GenerateID(CONF_LEDS_BTN_OUTPUT_IDS[2]): cv.declare_id(PartitionLightOutput),
        cv.GenerateID(CONF_LEDS_BTN_STATE_IDS[2]):  cv.declare_id(LightState),
    }
).extend(cv.COMPONENT_SCHEMA)


# ── Helpers ───────────────────────────────────────────────────────────────────

def _make_color3(color_list):
    return cg.StructInitializer(
        Color3,
        ("r", color_list[0]),
        ("g", color_list[1]),
        ("b", color_list[2]),
    )


def _effect_cpp_name(theme_idx, slot_name, effect_type):
    return f"tx_t{theme_idx}_{slot_name}_{effect_type}"


def _effect_display_name(theme_idx, slot_name, effect_type):
    return f"_tx_t{theme_idx}_{slot_name}_{effect_type}"


def _new_typed_id(cpp_var_name, type_class):
    return CoreID(cpp_var_name, is_declaration=True, type=type_class)


async def _create_effect(effect_cfg, display_name, cpp_var_name):
    etype = effect_cfg[CONF_EFFECT_TYPE]

    if etype == "scan":
        return cg.new_Pvariable(
            _new_typed_id(cpp_var_name, AddressableScanEffect), display_name
        )

    if etype == "rainbow":
        v = cg.new_Pvariable(
            _new_typed_id(cpp_var_name, AddressableRainbowEffect), display_name
        )
        cg.add(v.set_speed(effect_cfg[CONF_EFFECT_SPEED]))
        cg.add(v.set_width(effect_cfg[CONF_EFFECT_WIDTH]))
        return v

    if etype == "christmas":
        v = cg.new_Pvariable(
            _new_typed_id(cpp_var_name, AddressableChristmasEffect), display_name
        )
        cg.add(v.set_bit_size(effect_cfg[CONF_EFFECT_BIT_SIZE]))
        cg.add(v.set_blank_size(effect_cfg[CONF_EFFECT_BLANK_SIZE]))
        return v

    if etype == "stars":
        v = cg.new_Pvariable(
            _new_typed_id(cpp_var_name, AddressableStarsEffect), display_name
        )
        cg.add(v.set_stars_probability(effect_cfg[CONF_EFFECT_PROBABILITY]))
        color = effect_cfg[CONF_EFFECT_COLOR]
        cg.add(v.set_color(cg.StructInitializer(
            AddressableColorStarsEffectColor,
            ("r", int(round(color[0] * 2.55))),
            ("g", int(round(color[1] * 2.55))),
            ("b", int(round(color[2] * 2.55))),
            ("w", 0),
        )))
        return v

    return None


async def _create_partition(output_id, state_id, leds_var, segments):
    seg_list = [
        AddressableSegment(leds_var, from_, count, False)
        for from_, count in segments
    ]
    output_var = cg.new_Pvariable(output_id, seg_list)
    await cg.register_component(output_var, {})

    state_var = cg.new_Pvariable(state_id, output_var)
    await cg.register_component(state_var, {})
    cg.add(state_var.set_restore_mode(LightRestoreMode.LIGHT_RESTORE_DEFAULT_OFF))
    return state_var


# ── Code generation ───────────────────────────────────────────────────────────

async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)

    is_reversed = config[CONF_UPSIDE_DOWN]
    cg.add(var.set_upside_down(is_reversed))

    for idx, btn_cfg in enumerate(config[CONF_BUTTONS]):
        relay_var = None
        if CONF_RELAY in btn_cfg:
            relay = btn_cfg[CONF_RELAY]
            # Both forms reduce to a LightState id lookup. Inline dicts were
            # injected into light: at FINAL_VALIDATE; their LightState is
            # created by binary.light.to_code at code-gen time.
            relay_id = relay[CONF_ID] if isinstance(relay, dict) else relay
            relay_var = await cg.get_variable(relay_id)

        cg.add(var.add_button(
            btn_cfg[CONF_POSITION],
            btn_cfg[CONF_SWITCH_RELAY],
            relay_var,
        ))

        if CONF_STATE_SENSOR in btn_cfg:
            cg.add(var.set_button_state_sensor(
                idx, await cg.get_variable(btn_cfg[CONF_STATE_SENSOR])
            ))

        if CONF_SENSOR in btn_cfg:
            press_var = await binary_sensor.new_binary_sensor(btn_cfg[CONF_SENSOR])
            cg.add(var.set_button_press_sensor(idx, press_var))

    leds_var = await cg.get_variable(config[CONF_LEDS])
    cg.add(var.set_leds(leds_var))

    # leds_top: pixels 20-26 (normal) or 0-6 (reversed/180 deg)
    top_from = 0 if is_reversed else 20
    leds_top_var = await _create_partition(
        config[CONF_LEDS_TOP_OUTPUT_ID],
        config[CONF_LEDS_TOP_STATE_ID],
        leds_var, [(top_from, 7)],
    )
    cg.add(var.set_leds_top(leds_top_var))

    # leds_nightlight: all non-button pixels: 0-6, 8, 10, 12-27
    leds_nl_var = await _create_partition(
        config[CONF_LEDS_NL_OUTPUT_ID],
        config[CONF_LEDS_NL_STATE_ID],
        leds_var, [(0, 7), (8, 1), (10, 1), (12, 16)],
    )
    cg.add(var.set_leds_nightlight(leds_nl_var))

    # button LEDs: normal right=7 middle=9 left=11; reversed right=11 middle=9 left=7
    btn_pixels = [11, 9, 7] if is_reversed else [7, 9, 11]
    for i in range(3):
        btn_var = await _create_partition(
            config[CONF_LEDS_BTN_OUTPUT_IDS[i]],
            config[CONF_LEDS_BTN_STATE_IDS[i]],
            leds_var, [(btn_pixels[i], 1)],
        )
        cg.add(var.set_leds_button(i, btn_var))

    if CONF_VIBRA in config:
        cg.add(var.set_vibra(await cg.get_variable(config[CONF_VIBRA])))
    if CONF_MEDIA_PLAYER in config:
        cg.add(var.set_media_player(await cg.get_variable(config[CONF_MEDIA_PLAYER])))
    if CONF_API_CONNECTED in config:
        cg.add(var.set_api_connected(await cg.get_variable(config[CONF_API_CONNECTED])))
    if CONF_WIFI_CONNECTED in config:
        cg.add(var.set_wifi_connected(await cg.get_variable(config[CONF_WIFI_CONNECTED])))

    if CONF_NIGHTLIGHT in config:
        nl_cfg = config[CONF_NIGHTLIGHT]
        if CONF_NIGHT_SENSOR in nl_cfg:
            cg.add(var.set_night_sensor(await cg.get_variable(nl_cfg[CONF_NIGHT_SENSOR])))
        if CONF_SLEEP_SENSOR in nl_cfg:
            cg.add(var.set_sleep_sensor(await cg.get_variable(nl_cfg[CONF_SLEEP_SENSOR])))
        if CONF_AWAY_SENSOR in nl_cfg:
            cg.add(var.set_away_sensor(await cg.get_variable(nl_cfg[CONF_AWAY_SENSOR])))
        cg.add(var.set_room_type(nl_cfg[CONF_ROOM_TYPE]))

    cg.add(var.set_button_on_time_ms(config[CONF_BUTTON_ON_TIME]))
    cg.add(var.set_touch_led_duration_ms(config[CONF_TOUCH_LED_DURATION]))

    # Themes + per-theme effects
    # nightlight/sleep effects go on leds_nightlight; others on leds_top
    _NL_SLOTS  = {"nightlight", "sleep"}
    _TOP_SLOTS = {"touch", "swipe_left", "swipe_right", "long_press", "multi_touch"}

    top_effects = []
    nl_effects  = []

    for t_idx, t_cfg in enumerate(config[CONF_THEMES]):
        slot_effect_names = {}

        for slot_name in _NL_SLOTS | _TOP_SLOTS:
            slot_cfg   = t_cfg.get(slot_name) or {}
            effect_cfg = slot_cfg.get(CONF_SLOT_EFFECT)

            if effect_cfg is not None and effect_cfg.get(CONF_EFFECT_TYPE) is not None:
                etype        = effect_cfg[CONF_EFFECT_TYPE]
                display_name = _effect_display_name(t_idx, slot_name, etype)
                cpp_name     = _effect_cpp_name(t_idx, slot_name, etype)
                slot_effect_names[slot_name] = display_name

                ev = await _create_effect(effect_cfg, display_name, cpp_name)
                if ev is not None:
                    (nl_effects if slot_name in _NL_SLOTS else top_effects).append(ev)
            else:
                slot_effect_names[slot_name] = "None"

        def _s(slot, cfg=t_cfg):
            return cfg.get(slot) or {}

        cg.add(var.add_theme(cg.StructInitializer(
            Theme,
            ("name",                   t_cfg[CONF_NAME]),
            ("button_color",           _make_color3(_s("button")[CONF_SLOT_COLOR])),
            ("button_brightness",      _s("button")[CONF_SLOT_BRIGHTNESS]),
            ("nightlight_color",       _make_color3(_s("nightlight")[CONF_SLOT_COLOR])),
            ("nightlight_brightness",  _s("nightlight")[CONF_SLOT_BRIGHTNESS]),
            ("nightlight_effect",      slot_effect_names["nightlight"]),
            ("sleep_color",            _make_color3(_s("sleep")[CONF_SLOT_COLOR])),
            ("sleep_brightness",       _s("sleep")[CONF_SLOT_BRIGHTNESS]),
            ("sleep_effect",           slot_effect_names["sleep"]),
            ("touch_color",            _make_color3(_s("touch")[CONF_SLOT_COLOR])),
            ("touch_brightness",       _s("touch")[CONF_SLOT_BRIGHTNESS]),
            ("touch_effect",           slot_effect_names["touch"]),
            ("swipe_left_color",       _make_color3(_s("swipe_left")[CONF_SLOT_COLOR])),
            ("swipe_left_brightness",  _s("swipe_left")[CONF_SLOT_BRIGHTNESS]),
            ("swipe_left_effect",      slot_effect_names["swipe_left"]),
            ("swipe_right_color",      _make_color3(_s("swipe_right")[CONF_SLOT_COLOR])),
            ("swipe_right_brightness", _s("swipe_right")[CONF_SLOT_BRIGHTNESS]),
            ("swipe_right_effect",     slot_effect_names["swipe_right"]),
            ("long_press_color",       _make_color3(_s("long_press")[CONF_SLOT_COLOR])),
            ("long_press_brightness",  _s("long_press")[CONF_SLOT_BRIGHTNESS]),
            ("long_press_effect",      slot_effect_names["long_press"]),
            ("multi_touch_color",      _make_color3(_s("multi_touch")[CONF_SLOT_COLOR])),
            ("multi_touch_brightness", _s("multi_touch")[CONF_SLOT_BRIGHTNESS]),
            ("multi_touch_effect",     slot_effect_names["multi_touch"]),
            ("sound_pack",             t_cfg[CONF_SOUND_PACK]),
        )))

    if top_effects:
        cg.add(leds_top_var.add_effects(top_effects))
    if nl_effects:
        cg.add(leds_nl_var.add_effects(nl_effects))

    cg.add(var.set_initial_theme(config[CONF_ACTIVE_THEME]))

    for sp_cfg in config[CONF_SOUND_PACKS]:
        sp_args = [("name", sp_cfg[CONF_SOUND_PACK_NAME])]
        for key, field in [
            (CONF_SOUND_CLICK,       "click"),
            (CONF_SOUND_LONG_PRESS,  "long_press"),
            (CONF_SOUND_MULTI_PRESS, "multi_press"),
            (CONF_SOUND_SLIDE,       "slide"),
        ]:
            if key in sp_cfg:
                sp_args.append((field, await cg.get_variable(sp_cfg[key])))
            else:
                sp_args.append((field, cg.nullptr))
        cg.add(var.add_sound_pack(cg.StructInitializer(SoundPack, *sp_args)))

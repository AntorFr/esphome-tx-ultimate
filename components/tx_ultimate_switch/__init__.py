import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import light, binary_sensor, switch, select, media_player, audio
from esphome.const import CONF_ID, CONF_NAME
from esphome import automation

CODEOWNERS = ["@AntorFr"]
DEPENDENCIES = ["light", "binary_sensor"]
AUTO_LOAD = ["select"]

MULTI_CONF = True  # allow multiple instances (e.g. future blind variant)

CONF_TX_ULTIMATE_SWITCH = "tx_ultimate_switch"

# Top-level keys
CONF_BUTTON_COUNT = "button_count"
CONF_BUTTONS = "buttons"
CONF_LEDS = "leds"
CONF_LEDS_TOP = "leds_top"
CONF_LEDS_NIGHTLIGHT = "leds_nightlight"
CONF_LEDS_BUTTONS = "leds_buttons"
CONF_VIBRA = "vibra"
CONF_MEDIA_PLAYER = "media_player"
CONF_NIGHTLIGHT = "nightlight"
CONF_THEMES = "themes"
CONF_ACTIVE_THEME = "active_theme"
CONF_SOUND_PACKS = "sound_packs"
CONF_API_CONNECTED = "api_connected"
CONF_BUTTON_ON_TIME = "button_on_time"
CONF_TOUCH_LED_DURATION = "touch_led_duration"

# Nightlight keys
CONF_NIGHT_SENSOR = "sensor"
CONF_SLEEP_SENSOR = "sleep_sensor"
CONF_AWAY_SENSOR = "away_sensor"
CONF_ROOM_TYPE = "room_type"

# Button keys
CONF_SWITCH_RELAY = "switch_relay"
CONF_RELAY = "relay"

# Theme keys
CONF_BUTTON_COLOR = "button_color"
CONF_BUTTON_BRIGHTNESS = "button_brightness"
CONF_NIGHTLIGHT_COLOR = "nightlight_color"
CONF_NIGHTLIGHT_BRIGHTNESS = "nightlight_brightness"
CONF_NIGHTLIGHT_EFFECT = "nightlight_effect"
CONF_SLEEP_COLOR = "sleep_color"
CONF_SLEEP_BRIGHTNESS = "sleep_brightness"
CONF_SLEEP_EFFECT = "sleep_effect"
CONF_TOUCH_COLOR = "touch_color"
CONF_TOUCH_BRIGHTNESS = "touch_brightness"
CONF_TOUCH_EFFECT = "touch_effect"
CONF_SWIPE_LEFT_COLOR = "swipe_left_color"
CONF_SWIPE_LEFT_BRIGHTNESS = "swipe_left_brightness"
CONF_SWIPE_LEFT_EFFECT = "swipe_left_effect"
CONF_SWIPE_RIGHT_COLOR = "swipe_right_color"
CONF_SWIPE_RIGHT_BRIGHTNESS = "swipe_right_brightness"
CONF_SWIPE_RIGHT_EFFECT = "swipe_right_effect"
CONF_LONG_PRESS_COLOR = "long_press_color"
CONF_LONG_PRESS_BRIGHTNESS = "long_press_brightness"
CONF_LONG_PRESS_EFFECT = "long_press_effect"
CONF_MULTI_TOUCH_COLOR = "multi_touch_color"
CONF_MULTI_TOUCH_BRIGHTNESS = "multi_touch_brightness"
CONF_MULTI_TOUCH_EFFECT = "multi_touch_effect"
CONF_SOUND_PACK = "sound"

# Sound pack keys
CONF_SOUND_PACK_NAME = "name"
CONF_SOUND_CLICK = "click"
CONF_SOUND_LONG_PRESS = "long_press"
CONF_SOUND_MULTI_PRESS = "multi_press"
CONF_SOUND_SLIDE = "slide"

# ── C++ namespaced types ──────────────────────────────────────────────────────
tx_ultimate_switch_ns = cg.esphome_ns.namespace("tx_ultimate_switch")
TxUltimateSwitch = tx_ultimate_switch_ns.class_("TxUltimateSwitch", cg.Component)
Theme = tx_ultimate_switch_ns.struct("Theme")
Color3 = tx_ultimate_switch_ns.struct("Color3")
SoundPack = tx_ultimate_switch_ns.struct("SoundPack")
RoomType = tx_ultimate_switch_ns.enum("RoomType")

ROOM_TYPE_OPTIONS = {
    "standard": RoomType.STANDARD,
    "bedroom":  RoomType.BEDROOM,
    "dark":     RoomType.DARK,
}

# ── Validators ────────────────────────────────────────────────────────────────
def validate_color(value):
    """Validate an [r, g, b] list with values in [0, 100]."""
    if not isinstance(value, (list, tuple)):
        raise cv.Invalid("Color must be a list of 3 integers [r, g, b]")
    if len(value) != 3:
        raise cv.Invalid(f"Color must be exactly 3 integers [r, g, b], got {len(value)}")
    return [cv.int_range(min=0, max=100)(v) for v in value]


COLOR_SCHEMA = validate_color

THEME_SCHEMA = cv.Schema(
    {
        cv.Required(CONF_NAME): cv.string,
        cv.Optional(CONF_BUTTON_COLOR, default=[0, 0, 100]): COLOR_SCHEMA,
        cv.Optional(CONF_BUTTON_BRIGHTNESS, default=0.7): cv.percentage,
        cv.Optional(CONF_NIGHTLIGHT_COLOR, default=[80, 70, 0]): COLOR_SCHEMA,
        cv.Optional(CONF_NIGHTLIGHT_BRIGHTNESS, default=0.2): cv.percentage,
        cv.Optional(CONF_NIGHTLIGHT_EFFECT, default="None"): cv.string,
        cv.Optional(CONF_SLEEP_COLOR, default=[80, 0, 0]): COLOR_SCHEMA,
        cv.Optional(CONF_SLEEP_BRIGHTNESS, default=0.1): cv.percentage,
        cv.Optional(CONF_SLEEP_EFFECT, default="None"): cv.string,
        cv.Optional(CONF_TOUCH_COLOR, default=[0, 100, 100]): COLOR_SCHEMA,
        cv.Optional(CONF_TOUCH_BRIGHTNESS, default=1.0): cv.percentage,
        cv.Optional(CONF_TOUCH_EFFECT, default="Scan"): cv.string,
        cv.Optional(CONF_SWIPE_LEFT_COLOR, default=[0, 100, 0]): COLOR_SCHEMA,
        cv.Optional(CONF_SWIPE_LEFT_BRIGHTNESS, default=1.0): cv.percentage,
        cv.Optional(CONF_SWIPE_LEFT_EFFECT, default="None"): cv.string,
        cv.Optional(CONF_SWIPE_RIGHT_COLOR, default=[100, 0, 70]): COLOR_SCHEMA,
        cv.Optional(CONF_SWIPE_RIGHT_BRIGHTNESS, default=1.0): cv.percentage,
        cv.Optional(CONF_SWIPE_RIGHT_EFFECT, default="None"): cv.string,
        cv.Optional(CONF_LONG_PRESS_COLOR, default=[100, 0, 0]): COLOR_SCHEMA,
        cv.Optional(CONF_LONG_PRESS_BRIGHTNESS, default=1.0): cv.percentage,
        cv.Optional(CONF_LONG_PRESS_EFFECT, default="None"): cv.string,
        cv.Optional(CONF_MULTI_TOUCH_COLOR, default=[0, 0, 0]): COLOR_SCHEMA,
        cv.Optional(CONF_MULTI_TOUCH_BRIGHTNESS, default=1.0): cv.percentage,
        cv.Optional(CONF_MULTI_TOUCH_EFFECT, default="Rainbow"): cv.string,
        cv.Optional(CONF_SOUND_PACK, default="explore_the_stars"): cv.string,
    }
)

BUTTON_SCHEMA = cv.Schema(
    {
        cv.Optional(CONF_SWITCH_RELAY, default=True): cv.boolean,
        cv.Optional(CONF_RELAY): cv.use_id(light.LightState),
    }
)

# Named button keys in physical order (left → middle → right)
_BUTTON_KEYS_ORDERED = ["button_1", "button_2", "button_3"]


def _normalize_buttons(value):
    """Accept either a positional list or a named dict (button_1/button_2/button_3)."""
    if isinstance(value, list):
        return value
    if isinstance(value, dict):
        unknown = set(value.keys()) - set(_BUTTON_KEYS_ORDERED)
        if unknown:
            raise cv.Invalid(
                f"Unknown button key(s): {unknown}. Valid keys: button_1, button_2, button_3."
            )
        return [value[key] for key in _BUTTON_KEYS_ORDERED if key in value]
    raise cv.Invalid("buttons must be a list or a mapping with button_1/button_2/button_3 keys")

NIGHTLIGHT_SCHEMA = cv.Schema(
    {
        cv.Optional(CONF_NIGHT_SENSOR): cv.use_id(binary_sensor.BinarySensor),
        cv.Optional(CONF_SLEEP_SENSOR): cv.use_id(binary_sensor.BinarySensor),
        cv.Optional(CONF_AWAY_SENSOR): cv.use_id(binary_sensor.BinarySensor),
        # room_type controls nightlight behaviour:
        #   standard (default): nightlight follows night_sensor; sleep → dim guide
        #   bedroom: sleep_sensor active → LED OFF
        #   dark: nightlight always ON (no night_sensor needed); sleep/away still respected
        cv.Optional(CONF_ROOM_TYPE, default="standard"): cv.enum(ROOM_TYPE_OPTIONS, lower=True),
    }
)

SOUND_PACK_SCHEMA = cv.Schema(
    {
        cv.Required(CONF_SOUND_PACK_NAME): cv.string,
        cv.Optional(CONF_SOUND_CLICK): cv.use_id(audio.AudioFile),
        cv.Optional(CONF_SOUND_LONG_PRESS): cv.use_id(audio.AudioFile),
        cv.Optional(CONF_SOUND_MULTI_PRESS): cv.use_id(audio.AudioFile),
        cv.Optional(CONF_SOUND_SLIDE): cv.use_id(audio.AudioFile),
    }
)

CONFIG_SCHEMA = cv.Schema(
    {
        cv.GenerateID(): cv.declare_id(TxUltimateSwitch),
        cv.Optional(CONF_BUTTON_COUNT, default=3): cv.int_range(min=1, max=3),
        cv.Optional(CONF_BUTTONS, default=[]): cv.All(
            _normalize_buttons,
            cv.ensure_list(BUTTON_SCHEMA),
        ),
        cv.Required(CONF_LEDS): cv.use_id(light.LightState),
        cv.Required(CONF_LEDS_TOP): cv.use_id(light.LightState),
        cv.Required(CONF_LEDS_NIGHTLIGHT): cv.use_id(light.LightState),
        cv.Required(CONF_LEDS_BUTTONS): cv.All(
            cv.ensure_list(cv.use_id(light.LightState)),
            cv.Length(min=1, max=3),
        ),
        cv.Optional(CONF_VIBRA): cv.use_id(switch.Switch),
        cv.Optional(CONF_MEDIA_PLAYER): cv.use_id(media_player.MediaPlayer),
        cv.Optional(CONF_NIGHTLIGHT): NIGHTLIGHT_SCHEMA,
        cv.Optional(CONF_THEMES, default=[]): cv.ensure_list(THEME_SCHEMA),
        cv.Optional(CONF_ACTIVE_THEME, default="Default"): cv.string,
        cv.Optional(CONF_SOUND_PACKS, default=[]): cv.ensure_list(SOUND_PACK_SCHEMA),
        cv.Optional(CONF_API_CONNECTED): cv.use_id(binary_sensor.BinarySensor),
        cv.Optional(CONF_BUTTON_ON_TIME, default="500ms"): cv.positive_time_period_milliseconds,
        cv.Optional(CONF_TOUCH_LED_DURATION, default="6s"): cv.positive_time_period_milliseconds,
    }
).extend(cv.COMPONENT_SCHEMA)


# ── Code generation ───────────────────────────────────────────────────────────

def _make_color3(color_list):
    """Return a C++ Color3 struct initializer."""
    return cg.StructInitializer(
        Color3,
        ("r", color_list[0]),
        ("g", color_list[1]),
        ("b", color_list[2]),
    )


async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)

    # Button count
    cg.add(var.set_button_count(config[CONF_BUTTON_COUNT]))

    # Buttons
    for btn_cfg in config[CONF_BUTTONS]:
        relay = None
        if CONF_RELAY in btn_cfg:
            relay = await cg.get_variable(btn_cfg[CONF_RELAY])
        cg.add(var.add_button(btn_cfg[CONF_SWITCH_RELAY], relay))

    # LED lights
    leds = await cg.get_variable(config[CONF_LEDS])
    cg.add(var.set_leds(leds))
    leds_top = await cg.get_variable(config[CONF_LEDS_TOP])
    cg.add(var.set_leds_top(leds_top))
    leds_nl = await cg.get_variable(config[CONF_LEDS_NIGHTLIGHT])
    cg.add(var.set_leds_nightlight(leds_nl))
    for idx, led_id in enumerate(config[CONF_LEDS_BUTTONS]):
        led = await cg.get_variable(led_id)
        cg.add(var.set_leds_button(idx, led))

    # Optional peripherals
    if CONF_VIBRA in config:
        vibra = await cg.get_variable(config[CONF_VIBRA])
        cg.add(var.set_vibra(vibra))
    if CONF_MEDIA_PLAYER in config:
        mp = await cg.get_variable(config[CONF_MEDIA_PLAYER])
        cg.add(var.set_media_player(mp))
    if CONF_API_CONNECTED in config:
        api_s = await cg.get_variable(config[CONF_API_CONNECTED])
        cg.add(var.set_api_connected(api_s))

    # Nightlight sensors
    if CONF_NIGHTLIGHT in config:
        nl_cfg = config[CONF_NIGHTLIGHT]
        if CONF_NIGHT_SENSOR in nl_cfg:
            ns = await cg.get_variable(nl_cfg[CONF_NIGHT_SENSOR])
            cg.add(var.set_night_sensor(ns))
        if CONF_SLEEP_SENSOR in nl_cfg:
            ss = await cg.get_variable(nl_cfg[CONF_SLEEP_SENSOR])
            cg.add(var.set_sleep_sensor(ss))
        if CONF_AWAY_SENSOR in nl_cfg:
            aws = await cg.get_variable(nl_cfg[CONF_AWAY_SENSOR])
            cg.add(var.set_away_sensor(aws))
        cg.add(var.set_room_type(nl_cfg[CONF_ROOM_TYPE]))

    # Timings
    cg.add(var.set_button_on_time_ms(config[CONF_BUTTON_ON_TIME]))
    cg.add(var.set_touch_led_duration_ms(config[CONF_TOUCH_LED_DURATION]))

    # Themes — build list from all merged theme definitions
    theme_names = []
    for t_cfg in config[CONF_THEMES]:
        theme_names.append(t_cfg[CONF_NAME])
        theme_struct = cg.StructInitializer(
            Theme,
            ("name", t_cfg[CONF_NAME]),
            ("button_color", _make_color3(t_cfg[CONF_BUTTON_COLOR])),
            ("button_brightness", t_cfg[CONF_BUTTON_BRIGHTNESS]),
            ("nightlight_color", _make_color3(t_cfg[CONF_NIGHTLIGHT_COLOR])),
            ("nightlight_brightness", t_cfg[CONF_NIGHTLIGHT_BRIGHTNESS]),
            ("nightlight_effect", t_cfg[CONF_NIGHTLIGHT_EFFECT]),
            ("sleep_color", _make_color3(t_cfg[CONF_SLEEP_COLOR])),
            ("sleep_brightness", t_cfg[CONF_SLEEP_BRIGHTNESS]),
            ("sleep_effect", t_cfg[CONF_SLEEP_EFFECT]),
            ("touch_color", _make_color3(t_cfg[CONF_TOUCH_COLOR])),
            ("touch_brightness", t_cfg[CONF_TOUCH_BRIGHTNESS]),
            ("touch_effect", t_cfg[CONF_TOUCH_EFFECT]),
            ("swipe_left_color", _make_color3(t_cfg[CONF_SWIPE_LEFT_COLOR])),
            ("swipe_left_brightness", t_cfg[CONF_SWIPE_LEFT_BRIGHTNESS]),
            ("swipe_left_effect", t_cfg[CONF_SWIPE_LEFT_EFFECT]),
            ("swipe_right_color", _make_color3(t_cfg[CONF_SWIPE_RIGHT_COLOR])),
            ("swipe_right_brightness", t_cfg[CONF_SWIPE_RIGHT_BRIGHTNESS]),
            ("swipe_right_effect", t_cfg[CONF_SWIPE_RIGHT_EFFECT]),
            ("long_press_color", _make_color3(t_cfg[CONF_LONG_PRESS_COLOR])),
            ("long_press_brightness", t_cfg[CONF_LONG_PRESS_BRIGHTNESS]),
            ("long_press_effect", t_cfg[CONF_LONG_PRESS_EFFECT]),
            ("multi_touch_color", _make_color3(t_cfg[CONF_MULTI_TOUCH_COLOR])),
            ("multi_touch_brightness", t_cfg[CONF_MULTI_TOUCH_BRIGHTNESS]),
            ("multi_touch_effect", t_cfg[CONF_MULTI_TOUCH_EFFECT]),
            ("sound_pack", t_cfg[CONF_SOUND_PACK]),
        )
        cg.add(var.add_theme(theme_struct))

    cg.add(var.set_initial_theme(config[CONF_ACTIVE_THEME]))

    # Sound packs
    for sp_cfg in config[CONF_SOUND_PACKS]:
        # Sound files are referenced by their ESPHome variable ID string.
        # They must be declared as `files:` entries in the media_player config
        # with matching IDs. We get the C++ pointer via get_variable.
        sp_init_args = [("name", sp_cfg[CONF_SOUND_PACK_NAME])]
        for key, field in [
            (CONF_SOUND_CLICK, "click"),
            (CONF_SOUND_LONG_PRESS, "long_press"),
            (CONF_SOUND_MULTI_PRESS, "multi_press"),
            (CONF_SOUND_SLIDE, "slide"),
        ]:
            if key in sp_cfg:
                sound_var = await cg.get_variable(sp_cfg[key])
                sp_init_args.append((field, sound_var))
            else:
                sp_init_args.append((field, cg.nullptr))

        sp_struct = cg.StructInitializer(SoundPack, *sp_init_args)
        cg.add(var.add_sound_pack(sp_struct))

    # Build theme select options from registered themes
    # The select entity is expected to be declared in the device YAML;
    # we wire it here after all themes are known.
    # (Select is declared as a template select in packages/tx_ultimate_hw.yaml
    # and referenced via api id; we set its options dynamically via C++ at runtime.)

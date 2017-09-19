#pragma once
#include <pebble.h>

#define REPEAT_INTERVAL_MS 50
#define MAX_BITMAPS 9

#ifdef PBL_SDK_3
//static StatusBarLayer *s_status_bar;
#endif

#define VOLUME_PKEY 1
#define VOLUME_DEFAULT 0

#define STATUS_CONSULTAR_ESTADO_MUTE_Y_VOLUMEN 100
#define STATUS_CONSULTAR_ESTADO_MUTE_Y_VOLUMEN_CON_DELAY 101

#define STATUS_RECIBIDO_TV_APAGADA 204
#define STATUS_RECIBIDO_TIMEOUT 202

#define SETTINGS_KEY 1

#define MAX_LENGTH 30

// A structure containing our settings
typedef struct ClaySettings {
  char ipaddr[MAX_LENGTH];
  char port[MAX_LENGTH];
} __attribute__((__packed__)) ClaySettings;

static void prv_default_settings();
static void prv_load_settings();
static void prv_save_settings();

static bool startsWith(const char *pre, const char *str);
static void ledon();
static void ledoff();
static void accel_tap_handler(AccelAxisType axis, int32_t direction);
static void battery_callback(BatteryChargeState state);
static void bluetooth_callback(bool connected);
static void battery_update_proc(Layer *layer, GContext *ctx);
static void handle_minute_tick(struct tm *tick_time, TimeUnits units_changed);
static void bars_update_callback(Layer *me, GContext* ctx);
static void progress_update_callback(Layer *me, GContext* ctx);
static void update_text();
static void update_action_bar_layer();
static void update_tv_layers();
static void tv_screen_is_on();
static void tv_screen_is_off();
static void tv_is_on();
static void tv_is_off();
static void select_long_click_handler(ClickRecognizerRef recognizer, void *context);
static void increment_click_handler(ClickRecognizerRef recognizer, void *context);
static void select_click_handler(ClickRecognizerRef recognizer, void *context);
static void decrement_click_handler(ClickRecognizerRef recognizer, void *context);
static void click_config_provider(void *context);

static void send_message_to_phone(int status);
static void send_configuration_message_to_phone();
static void in_received_handler(DictionaryIterator *received, void *context);
static void in_dropped_handler(AppMessageResult reason, void *context);
static void out_failed_handler(DictionaryIterator *failed, AppMessageResult reason, void *context);

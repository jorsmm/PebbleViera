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

static Window *s_main_window;
static TextLayer *s_label_layer;
static TextLayer *text_time_layer;
static TextLayer *text_day_layer;
static BitmapLayer *s_icon_layer;
static TextLayer *s_tv_screen_layer;
static ActionBarLayer *s_action_bar_layer;

static GBitmap *s_icon_bitmap;

static GBitmap* table[MAX_BITMAPS];
static char* titles[MAX_BITMAPS/3];

int offset=2;
static int s_volume = VOLUME_DEFAULT;
static int s_mute = 0;

static int s_volume_status_pending=0;

Layer *bars_layer;
Layer *main_layer;

static BitmapLayer *s_led_layer;
static BitmapLayer *s_led_layer2;
static GBitmap *s_led_bitmap_red;
static GBitmap *s_led_bitmap_orange;
static GBitmap *s_led_bitmap_green;

static GContext* s_ctx;

static int s_battery_level;
static bool s_battery_is_charging;
static Layer *s_battery_layer;
static BitmapLayer *s_battery_icon_layer;
static GBitmap *s_battery_icon_bitmap;
static TextLayer *text_battery_layer;

static BitmapLayer *s_bt_icon_layer;
static GBitmap *s_bt_icon_bitmap;

static bool s_tv_screen_is_on=false;
static bool s_tv_is_on=false;

static int s_last_status_sent=-1;

// status a enviar para encender/apagar TV
static int TVONOFF=8;

static int lateral=PBL_IF_RECT_ELSE(0, 25);

#ifdef PBL_PLATFORM_EMERY
#else
#endif

bool startsWith(const char *pre, const char *str);
void ledon();
void ledoff();
static void accel_tap_handler(AccelAxisType axis, int32_t direction);
static void battery_callback(BatteryChargeState state);
static void bluetooth_callback(bool connected);
static void battery_update_proc(Layer *layer, GContext *ctx);
void handle_minute_tick(struct tm *tick_time, TimeUnits units_changed);
void bars_update_callback(Layer *me, GContext* ctx);
void progress_update_callback(Layer *me, GContext* ctx);
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

void send_message_to_phone(int status);
static void in_received_handler(DictionaryIterator *received, void *context);
static void in_dropped_handler(AppMessageResult reason, void *context);
static void out_failed_handler(DictionaryIterator *failed, AppMessageResult reason, void *context);

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool startsWith(const char *pre, const char *str) {
    size_t lenpre = strlen(pre),
           lenstr = strlen(str);
    return lenstr < lenpre ? false : strncmp(pre, str, lenpre) == 0;
}

/////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////
void ledon() {
    layer_set_hidden(bitmap_layer_get_layer(s_led_layer), false);
}
void ledoff() {
    layer_set_hidden(bitmap_layer_get_layer(s_led_layer), true);
}

/////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////
static void accel_tap_handler(AccelAxisType axis, int32_t direction) {
  // A tap event occured
  APP_LOG(APP_LOG_LEVEL_INFO, "accel_tap_handler: axis=%d. dir=%d", axis, (int)direction);
  send_message_to_phone(STATUS_CONSULTAR_ESTADO_MUTE_Y_VOLUMEN);
}

/////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////
static void battery_callback(BatteryChargeState state) {
  // Record the new battery level
  s_battery_level = state.charge_percent;
  s_battery_is_charging = state.is_charging;
}
static void bluetooth_callback(bool connected) {
  // Show icon if disconnected
  layer_set_hidden(bitmap_layer_get_layer(s_bt_icon_layer), connected);
  if(!connected) {
    // Issue a vibrating alert
    vibes_double_pulse();
  }
}
/////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////
static void battery_update_proc(Layer *layer, GContext *ctx) {
  GRect bounds = layer_get_bounds(layer);
  // Find the width of the bar
  int width = (int)(float)(((float)s_battery_level / 100.0F) * bounds.size.w);
  // Draw the background
  graphics_context_set_fill_color(ctx, GColorClear);
  graphics_fill_rect(ctx, bounds, 0, GCornerNone);
  // Draw the bar
  if (s_battery_is_charging) {
    graphics_context_set_fill_color(ctx, GColorYellow);
  }
  else {
    if (s_battery_level > 20) {
      graphics_context_set_fill_color(ctx, GColorGreen);
    }
    else {
      graphics_context_set_fill_color(ctx, GColorRed);
    }
  }
  graphics_fill_rect(ctx, GRect(0, 0, width, bounds.size.h), 0, GCornerNone);
  static char ss_battery_level[4];
  snprintf(ss_battery_level,sizeof(ss_battery_level),"%u", s_battery_level);
  text_layer_set_text(text_battery_layer, ss_battery_level);
}

/////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////
void handle_minute_tick(struct tm *tick_time, TimeUnits units_changed) {
  static char time_text[] = "00:00";
  static char day_text[] = "00";
  char *time_format;
  if (clock_is_24h_style()) {
    time_format = "%R";
  } else {
    time_format = "%I:%M";
  }
  strftime(time_text, sizeof(time_text), time_format, tick_time);
  if (!clock_is_24h_style() && (time_text[0] == '0')) {
    memmove(time_text, &time_text[1], sizeof(time_text) - 1);
  }
  strftime(day_text, sizeof(day_text), "%d", tick_time);
  text_layer_set_text(text_time_layer, time_text);
  text_layer_set_text(text_day_layer, day_text);
}

/////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////

int volbar_x=5;
int volbar_y=135;
int volbar_border=1;
#if PBL_DISPLAY_HEIGHT == 228
int volbar_multi=14;
#else
int volbar_multi=10;
#endif

void bars_update_callback(Layer *me, GContext* ctx) {
  int volbar_h=25*volbar_multi/10;
  int volbar_w=102*volbar_multi/10;
  Layer *window_layer = window_get_root_layer(layer_get_window(me));
  GRect bounds = layer_get_unobstructed_bounds(window_layer);
  (void)me;
  s_ctx=ctx;
  volbar_x=(bounds.size.w-ACTION_BAR_WIDTH-volbar_w-(2*volbar_border))/2;
  volbar_y=bounds.size.h-volbar_h-10;

  if (offset==0) {
    layer_set_hidden (me, false);
    graphics_context_set_fill_color(ctx, GColorOxfordBlue);
    graphics_fill_rect(ctx, GRect(volbar_x, volbar_y, volbar_w, volbar_h), 4, GCornersAll);
    graphics_context_set_fill_color(ctx, GColorWhite);
    graphics_fill_rect(ctx, GRect(volbar_x+volbar_border, volbar_y+volbar_border, volbar_w-(volbar_border*2), volbar_h-(volbar_border*2)), 4, GCornersAll);
  }
  else {
    layer_set_hidden (me, true);
  }
}
void progress_update_callback(Layer *me, GContext* ctx) {
  int volbar_h=25*volbar_multi/10;
  int volbar_w=102*volbar_multi/10;

  s_ctx=ctx;
  if (offset==0) {
    layer_set_hidden (me, false);
    if (s_mute==1) {
//      graphics_context_set_stroke_color(ctx, GColorLightGray);
      graphics_context_set_fill_color(ctx, GColorLightGray);
    }
    else {
//      graphics_context_set_stroke_color(ctx, GColorBlue);
      graphics_context_set_fill_color(ctx, GColorBlue);
    }
    graphics_fill_rect(ctx, GRect(volbar_x+volbar_border, volbar_y+(volbar_border), s_volume*volbar_multi/10, volbar_h-(2*volbar_border)), 4, GCornersAll);
  }
  else {
    layer_set_hidden (me, true);
  }
}

/////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////
static void update_text() {
  layer_mark_dirty(main_layer);
  static char s_body_text[18];
  if (offset==0 && s_mute==1) {
    text_layer_set_text_color (s_label_layer, GColorRed);
    snprintf(s_body_text, sizeof(s_body_text), "MUTE");
  }
  else {
    if (s_volume<80) {
      text_layer_set_text_color (s_label_layer, GColorBlack);
    }
    else {
      text_layer_set_text_color (s_label_layer, GColorWhite);
    }

    snprintf(s_body_text, sizeof(s_body_text), titles[offset], s_volume);
  }
  text_layer_set_text(s_label_layer, s_body_text);
  if (s_ctx!=NULL) {
    bars_update_callback(bars_layer, s_ctx);
    progress_update_callback(main_layer, s_ctx);
  }
}
static void update_action_bar_layer() {
  layer_mark_dirty(bars_layer);
  action_bar_layer_set_icon(s_action_bar_layer, BUTTON_ID_UP, table[offset*3]);
  action_bar_layer_set_icon(s_action_bar_layer, BUTTON_ID_SELECT, table[(offset*3)+1]);
  action_bar_layer_set_icon(s_action_bar_layer, BUTTON_ID_DOWN, table[(offset*3)+2]);
  update_text();
}

/////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////
static void update_tv_layers() {
  printf("s_tv_screen_is_on=%s s_tv_is_on=%s", s_tv_screen_is_on ? "true" : "false", s_tv_is_on ? "true" : "false");

  if (!s_tv_is_on) {
    bitmap_layer_set_bitmap(s_led_layer2, s_led_bitmap_red);
  }
  else {
    if (s_tv_screen_is_on) {
      bitmap_layer_set_bitmap(s_led_layer2, s_led_bitmap_green);
    }
    else {
      bitmap_layer_set_bitmap(s_led_layer2, s_led_bitmap_orange);
    }
  }

  layer_set_hidden(text_layer_get_layer(s_tv_screen_layer), s_tv_screen_is_on);
  layer_set_hidden(text_layer_get_layer(s_label_layer), !s_tv_screen_is_on && s_tv_is_on);
  layer_set_hidden(main_layer, !s_tv_screen_is_on);
  layer_set_hidden(bars_layer, !s_tv_screen_is_on);
}
static void tv_screen_is_on() {
  APP_LOG(APP_LOG_LEVEL_DEBUG, "tv_screen_is_on: %d", s_tv_screen_is_on);
  if (!s_tv_screen_is_on) {
    APP_LOG(APP_LOG_LEVEL_INFO, "tv_screen_is_on: LA TV ESTA ENCENDIDA");
    s_tv_screen_is_on = true;
    s_tv_is_on = true;
    offset=0;
    update_action_bar_layer();
    update_tv_layers();
    update_text();
  }
}
static void tv_screen_is_off() {
  APP_LOG(APP_LOG_LEVEL_DEBUG, "tv_screen_is_off: %d", s_tv_screen_is_on);
  if (s_tv_screen_is_on) {
    APP_LOG(APP_LOG_LEVEL_INFO, "tv_screen_is_off: LA TV ESTA APAGADA");
    s_tv_screen_is_on = false;
    s_tv_is_on = true;
    offset=2;
    update_action_bar_layer();
    update_tv_layers();
    update_text();
  }
}

static void tv_is_off() {
  APP_LOG(APP_LOG_LEVEL_DEBUG, "tv_is_off: %d", s_tv_is_on);
  if (s_tv_is_on) {
    APP_LOG(APP_LOG_LEVEL_INFO, "tv_is_off: LA TV NO TIENE RED");
    s_tv_screen_is_on = false;
    s_tv_is_on = false;
    update_action_bar_layer();
    update_tv_layers();
    update_text();
  }
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
static void select_long_click_handler(ClickRecognizerRef recognizer, void *context) {
  offset=(offset+1)%(MAX_BITMAPS/3);
  update_action_bar_layer();
}
static void increment_click_handler(ClickRecognizerRef recognizer, void *context) {
  APP_LOG(APP_LOG_LEVEL_INFO, "increment_click_handler: %d",offset);

  // si se está en la parte de volumen
  if (offset==0) {
    APP_LOG(APP_LOG_LEVEL_DEBUG, "s_volume_status_pending: %d",s_volume_status_pending);
    if (s_volume_status_pending) {return;}
    if (s_volume<100) {
      s_volume++;
      s_mute=0;
    }
    else {
      return;
    }
  }
  update_text();
  send_message_to_phone(offset*3+0x1);
}
static void select_click_handler(ClickRecognizerRef recognizer, void *context) {
  if (offset==0) {
    s_mute=(s_mute+1)%2;
  }
  send_message_to_phone(offset*3+0x2);
  update_text();
}
static void decrement_click_handler(ClickRecognizerRef recognizer, void *context) {
  APP_LOG(APP_LOG_LEVEL_INFO, "decrement_click_handler: %d",offset);
  if (offset==0) {
    APP_LOG(APP_LOG_LEVEL_DEBUG, "s_volume_status_pending: %d",s_volume_status_pending);
    if (s_volume_status_pending) {return;}
    if (s_volume>0) {
      s_mute=0;
      s_volume--;
    }
    else {
      return;
    }
  }
  update_text();
  send_message_to_phone(offset*3+0x3);
}
static void click_config_provider(void *context) {
  window_single_repeating_click_subscribe(BUTTON_ID_UP, REPEAT_INTERVAL_MS, increment_click_handler);
  window_single_repeating_click_subscribe(BUTTON_ID_DOWN, REPEAT_INTERVAL_MS, decrement_click_handler);
  window_single_click_subscribe(BUTTON_ID_SELECT, select_click_handler);
  window_long_click_subscribe(BUTTON_ID_SELECT, 400, select_long_click_handler, NULL);
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// Write message to buffer & send
void send_message_to_phone(int status){
  APP_LOG(APP_LOG_LEVEL_INFO, "send_message_to_phone: %d", status);
  if (status != STATUS_CONSULTAR_ESTADO_MUTE_Y_VOLUMEN) {
    s_last_status_sent=status;
    // se pulsa subir/bajar volumen
    if (status == 1 || status ==3) {
      APP_LOG(APP_LOG_LEVEL_ERROR, "send_message_to_phone: %d poner s_volume_status_pending=1", status);
      s_volume_status_pending=1;
    }
  }
  else {
    if (s_volume_status_pending) {
      APP_LOG(APP_LOG_LEVEL_WARNING, "send_message_to_phone. NO ENVIAR: %d", status);
      return;
    }
    else {
      APP_LOG(APP_LOG_LEVEL_ERROR, "send_message_to_phone: %d poner s_volume_status_pending=1", status);
      s_volume_status_pending=1;
    }
  }

  ledon();

  DictionaryIterator *iter;
	app_message_outbox_begin(&iter);
	dict_write_uint8(iter, MESSAGE_KEY_status, status);
	dict_write_end(iter);
  app_message_outbox_send();
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// Called when a message is received from PebbleKitJS
static void in_received_handler(DictionaryIterator *received, void *context) {
	Tuple *tuple;

	APP_LOG(APP_LOG_LEVEL_DEBUG, "Received DictionaryIterator from Phone");

  // mensajes con la parte JavaScript
	tuple = dict_find(received, MESSAGE_KEY_status);
	if(tuple) {
    ledoff();
    s_volume_status_pending=0;
		APP_LOG(APP_LOG_LEVEL_DEBUG, "   Received Status from Phone: %d", (int)tuple->value->uint32);
    // recibido status indicando que la tv está apagada
    if ((int)tuple->value->uint32 == STATUS_RECIBIDO_TV_APAGADA) {
      	tv_screen_is_off();
    }
    else if ((int)tuple->value->uint32 == STATUS_RECIBIDO_TIMEOUT) {
      	tv_is_off();
    }

    if (s_last_status_sent==TVONOFF && (int)tuple->value->uint32 == 0) {
      send_message_to_phone(STATUS_CONSULTAR_ESTADO_MUTE_Y_VOLUMEN_CON_DELAY);
    }
	}
	tuple = dict_find(received, MESSAGE_KEY_message);
	if(tuple) {
    ledoff();
		APP_LOG(APP_LOG_LEVEL_DEBUG, "   Received Message from Phone: %s", tuple->value->cstring);
    if (startsWith("v=",tuple->value->cstring)) {
      size_t lenstring = strlen(tuple->value->cstring);
      int tamano=lenstring-2;
      char svolume[tamano+1];
      memcpy( svolume, &tuple->value->cstring[2], tamano );
      svolume[tamano] = '\0';
      s_volume = atoi( svolume );
      APP_LOG(APP_LOG_LEVEL_INFO, "   Received VOLUME: %u", s_volume);
      s_volume_status_pending=0;
      // si se recibe un volumen es que la TV está encendida
      tv_screen_is_on();
      // por si acaso actualizar volumen
      update_text();
    }
    else if (startsWith("m=",tuple->value->cstring)) {
      size_t lenstring = strlen(tuple->value->cstring);
      int tamano=lenstring-2;
      char smute[tamano+1];
      memcpy( smute, &tuple->value->cstring[2], tamano );
      smute[tamano] = '\0';
      s_mute = atoi( smute );
      APP_LOG(APP_LOG_LEVEL_INFO, "   Received MUTE: %u", s_mute);
      update_text();
    }
    else if (startsWith("init=",tuple->value->cstring)) {
      APP_LOG(APP_LOG_LEVEL_INFO, "   Received JS Ready");
      send_message_to_phone(STATUS_CONSULTAR_ESTADO_MUTE_Y_VOLUMEN);
    }
	}
  // valores de configuracion
	tuple = dict_find(received, MESSAGE_KEY_ipaddr);
	if(tuple) {
		APP_LOG(APP_LOG_LEVEL_INFO, "   Received IP Address configuration value from Phone: %s", tuple->value->cstring);
//    persist_write_string(PKEY_IPADDR,tuple->value->cstring);
	}
	tuple = dict_find(received, MESSAGE_KEY_port);
	if(tuple) {
		APP_LOG(APP_LOG_LEVEL_INFO, "   Received Port configuration value from Phone: %s", tuple->value->cstring);
	}
}
// Called when an incoming message from PebbleKitJS is dropped
static void in_dropped_handler(AppMessageResult reason, void *context) {
}
// Called when PebbleKitJS does not acknowledge receipt of a message
static void out_failed_handler(DictionaryIterator *failed, AppMessageResult reason, void *context) {
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
static void window_load(Window *window) {
  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_unobstructed_bounds(window_layer);

#ifdef PBL_SDK_3
  // Set up the status bar last to ensure it is on top of other Layers
//  s_status_bar = status_bar_layer_create();
//  layer_add_child(window_layer, status_bar_layer_get_layer(s_status_bar));
//  status_bar_layer_set_colors(s_status_bar, GColorGreen, GColorBlack);
#endif

  // el dia
  #if PBL_DISPLAY_HEIGHT == 228
  text_day_layer = text_layer_create(GRect(-2+lateral, -5, 42, 36));
  #else
  text_day_layer = text_layer_create(GRect(-2+lateral, -5, 26, 30));
  #endif
  text_layer_set_background_color(text_day_layer, GColorBlue);
  text_layer_set_text_color (text_day_layer, GColorWhite);
  text_layer_set_text_alignment(text_day_layer, GTextAlignmentCenter);
#if PBL_DISPLAY_HEIGHT == 228
  text_layer_set_font(text_day_layer, fonts_get_system_font(FONT_KEY_LECO_32_BOLD_NUMBERS));
#else
  text_layer_set_font(text_day_layer, fonts_get_system_font(FONT_KEY_GOTHIC_28_BOLD));
#endif
  layer_add_child(window_layer, text_layer_get_layer(text_day_layer));

  // la hora
  const GEdgeInsets time_insets = {.top = -5, .right = ACTION_BAR_WIDTH +4, .bottom = 145, .left = ACTION_BAR_WIDTH / 3 - 4 + lateral };
  text_time_layer = text_layer_create(grect_inset(bounds, time_insets));
  text_layer_set_background_color(text_time_layer, GColorClear);
  text_layer_set_text_color (text_time_layer, GColorBlue);
  text_layer_set_text_alignment(text_time_layer, GTextAlignmentCenter);
#if PBL_DISPLAY_HEIGHT == 228
  text_layer_set_font(text_time_layer, fonts_get_system_font(FONT_KEY_LECO_32_BOLD_NUMBERS));
#else
  text_layer_set_font(text_time_layer, fonts_get_system_font(FONT_KEY_GOTHIC_28_BOLD));
#endif
  layer_add_child(window_layer, text_layer_get_layer(text_time_layer));

  int right_x=bounds.size.w - ACTION_BAR_WIDTH - 28;
  // Create battery meter Layer
  s_battery_icon_bitmap = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_BATTERY);
  s_battery_icon_layer = bitmap_layer_create(GRect(right_x, 2, 24, 24));
  bitmap_layer_set_bitmap(s_battery_icon_layer, s_battery_icon_bitmap);
  layer_add_child(window_get_root_layer(window), bitmap_layer_get_layer(s_battery_icon_layer));

  s_battery_layer = layer_create(GRect(right_x+2, 10, 19, 8));
  layer_set_update_proc(s_battery_layer, battery_update_proc);
  layer_add_child(window_get_root_layer(window), s_battery_layer);

  text_battery_layer = text_layer_create(GRect(right_x+2, 8, 19, 9));
  text_layer_set_background_color(text_battery_layer, GColorClear);
  text_layer_set_text_color (text_battery_layer, GColorBlack);
  text_layer_set_text_alignment(text_battery_layer, GTextAlignmentCenter);
  text_layer_set_font(text_battery_layer, fonts_get_system_font(FONT_KEY_GOTHIC_09));
  layer_add_child(window_layer, text_layer_get_layer(text_battery_layer));

  // Ensure battery level is displayed from the start
  battery_callback(battery_state_service_peek());

  // Create the Bluetooth icon GBitmap
  s_bt_icon_bitmap = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_BT_ICON);
  s_bt_icon_layer = bitmap_layer_create(GRect(right_x, 2, 24, 24));
  bitmap_layer_set_background_color(s_bt_icon_layer, GColorRed);
  bitmap_layer_set_bitmap(s_bt_icon_layer, s_bt_icon_bitmap);
  layer_add_child(window_get_root_layer(window), bitmap_layer_get_layer(s_bt_icon_layer));
  // Show the correct state of the BT connection from the start
  bluetooth_callback(connection_service_peek_pebble_app_connection());

  #if PBL_DISPLAY_HEIGHT == 228
  int offfset_ico_tv=10;
  #else
  int offfset_ico_tv=0;
  #endif

  // icono central TV
  s_icon_bitmap = gbitmap_create_with_resource(RESOURCE_ID_TVC);
  GRect boundsIcoTV=gbitmap_get_bounds(s_icon_bitmap);
  s_icon_layer = bitmap_layer_create(GRect((bounds.size.w-ACTION_BAR_WIDTH-boundsIcoTV.size.w)/2, 40+offfset_ico_tv, boundsIcoTV.size.w, boundsIcoTV.size.h));
  bitmap_layer_set_bitmap(s_icon_layer, s_icon_bitmap);
  bitmap_layer_set_compositing_mode(s_icon_layer, GCompOpSet);
  layer_add_child(window_layer, bitmap_layer_get_layer(s_icon_layer));
  GRect boundsTV = layer_get_frame((Layer *)s_icon_layer);
  APP_LOG(APP_LOG_LEVEL_ERROR, "watch   bounds x=%d. y=%d. w=%d. h=%d", bounds.origin.x, bounds.origin.y, bounds.size.w, bounds.size.h);
  APP_LOG(APP_LOG_LEVEL_ERROR, "ActionBarWidth x=%d", ACTION_BAR_WIDTH);

  // layer central pantalla negra TV
  #if PBL_DISPLAY_HEIGHT == 228
  const GEdgeInsets tv_screen_insets = {.top = 45, .right = 25, .bottom = 5, .left = 5};
  #else
  const GEdgeInsets tv_screen_insets = {.top = 30, .right = 20, .bottom = 5, .left = 5};
  #endif
  s_tv_screen_layer = text_layer_create(grect_inset(boundsTV, tv_screen_insets));
  text_layer_set_background_color(s_tv_screen_layer, GColorBlack);
  layer_set_hidden(text_layer_get_layer(s_tv_screen_layer), s_tv_screen_is_on);
  layer_add_child(window_layer, text_layer_get_layer(s_tv_screen_layer));

  // bitmap de los tres leds
  s_led_bitmap_red = gbitmap_create_with_resource(RESOURCE_ID_REDDOT);
  s_led_bitmap_orange = gbitmap_create_with_resource(RESOURCE_ID_ORANGEDOT);
  s_led_bitmap_green = gbitmap_create_with_resource(RESOURCE_ID_GREENDOT);

  // boton led rojo arriba
  #if PBL_DISPLAY_HEIGHT == 228
  const GEdgeInsets led_insets = {.top = 59, .right = 8, .bottom = 45, .left = 96};
  #else
  const GEdgeInsets led_insets = {.top = 39, .right = 7, .bottom = 31, .left = 63};
  #endif
  s_led_layer = bitmap_layer_create(grect_inset(boundsTV, led_insets));
  layer_set_hidden(bitmap_layer_get_layer(s_led_layer), false);
  bitmap_layer_set_bitmap(s_led_layer, s_led_bitmap_red);
  bitmap_layer_set_compositing_mode(s_led_layer, GCompOpSet);
  layer_add_child(window_layer, bitmap_layer_get_layer(s_led_layer));

  // boton led abajo
  #if PBL_DISPLAY_HEIGHT == 228
  const GEdgeInsets led_insets2 = {.top = 78, .right = 8, .bottom = 26, .left = 96};
  #else
  const GEdgeInsets led_insets2 = {.top = 51, .right = 7, .bottom = 19, .left = 63};
  #endif
  s_led_layer2 = bitmap_layer_create(grect_inset(boundsTV, led_insets2));
  layer_set_hidden(bitmap_layer_get_layer(s_led_layer2), false);
  bitmap_layer_set_bitmap(s_led_layer2, s_led_bitmap_red);
  bitmap_layer_set_compositing_mode(s_led_layer2, GCompOpSet);
  layer_add_child(window_layer, bitmap_layer_get_layer(s_led_layer2));

  // barra de volumen
  bars_layer = layer_create(layer_get_frame(window_layer));
  layer_set_update_proc(bars_layer, bars_update_callback);
  layer_set_hidden(bars_layer, !s_tv_screen_is_on);
  layer_add_child(window_layer, bars_layer);
  main_layer = layer_create(layer_get_frame(window_layer));
  layer_set_update_proc(main_layer, progress_update_callback);
  layer_set_hidden(main_layer, !s_tv_screen_is_on);
  layer_add_child(window_layer, main_layer);

  // texto
   s_label_layer = text_layer_create(GRect(10, bounds.size.h-30-(volbar_multi-10)-10, (bounds.size.w-ACTION_BAR_WIDTH-2*10), 30));
  text_layer_set_background_color(s_label_layer, GColorClear);
  text_layer_set_text_alignment(s_label_layer, GTextAlignmentCenter);
  #if PBL_DISPLAY_HEIGHT == 228
  text_layer_set_font(s_label_layer, fonts_get_system_font(FONT_KEY_GOTHIC_28_BOLD));
  #else
  text_layer_set_font(s_label_layer, fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD));
  #endif
  layer_set_hidden(text_layer_get_layer(s_label_layer), s_tv_screen_is_on);
  layer_add_child(window_layer, text_layer_get_layer(s_label_layer));

  titles[0]="Vol = %u";
  titles[1]="TV Channels";
  titles[2]="General";
  table[0] = gbitmap_create_with_resource(RESOURCE_ID_VOLUP);
  table[1] = gbitmap_create_with_resource(RESOURCE_ID_MUTE);
  table[2] = gbitmap_create_with_resource(RESOURCE_ID_VOLDOWN);
  table[3] = gbitmap_create_with_resource(RESOURCE_ID_CHUP);
  table[4] = gbitmap_create_with_resource(RESOURCE_ID_ICON);
  table[5] = gbitmap_create_with_resource(RESOURCE_ID_CHDOWN);
  table[6] = gbitmap_create_with_resource(RESOURCE_ID_CHINPUT);
  table[7] = gbitmap_create_with_resource(RESOURCE_ID_POWER);
  table[8] = gbitmap_create_with_resource(RESOURCE_ID_INFO);

  s_action_bar_layer = action_bar_layer_create();
  update_action_bar_layer();
  action_bar_layer_set_background_color(s_action_bar_layer, GColorBlack);
  action_bar_layer_add_to_window(s_action_bar_layer, window);
  // Set the click config provider:
  action_bar_layer_set_click_config_provider(s_action_bar_layer,click_config_provider);
}

static void window_unload(Window *window) {
  text_layer_destroy(s_label_layer);
  text_layer_destroy(text_time_layer);
  text_layer_destroy(text_day_layer);
  action_bar_layer_destroy(s_action_bar_layer);
  bitmap_layer_destroy(s_icon_layer);
  gbitmap_destroy(s_led_bitmap_red);
  gbitmap_destroy(s_led_bitmap_orange);
  gbitmap_destroy(s_led_bitmap_green);
  bitmap_layer_destroy(s_led_layer);
  bitmap_layer_destroy(s_led_layer2);
  text_layer_destroy(s_tv_screen_layer);
  layer_destroy(bars_layer);
  layer_destroy(main_layer);
  layer_destroy(s_battery_layer);
  gbitmap_destroy(s_bt_icon_bitmap);
  bitmap_layer_destroy(s_bt_icon_layer);
  gbitmap_destroy(s_battery_icon_bitmap);
  bitmap_layer_destroy(s_battery_icon_layer);
  text_layer_destroy(text_battery_layer);
  int i=0;
  for (i=0; i< MAX_BITMAPS; i++) {
    gbitmap_destroy(table[i]);
  }

  window_destroy(window);
  s_main_window = NULL;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void init(void) {
  APP_LOG(APP_LOG_LEVEL_INFO, ">>>Init");
  s_main_window = window_create();
    window_set_background_color(s_main_window, PBL_IF_COLOR_ELSE(GColorWhite , GColorWhite));
    window_set_window_handlers(s_main_window, (WindowHandlers) {
        .load = window_load,
        .unload = window_unload,
    });
	window_stack_push(s_main_window, true);
	// Register AppMessage handlers
	app_message_register_inbox_received(in_received_handler);
	app_message_register_inbox_dropped(in_dropped_handler);
	app_message_register_outbox_failed(out_failed_handler);
	app_message_open(app_message_inbox_size_maximum(), app_message_outbox_size_maximum());
  // Register for minute updates
  tick_timer_service_subscribe(MINUTE_UNIT, handle_minute_tick);
  // Register for battery level updates
  battery_state_service_subscribe(battery_callback);
  // Register for Bluetooth connection updates
  connection_service_subscribe((ConnectionHandlers) {
    .pebble_app_connection_handler = bluetooth_callback
  });
  // Subscribe to tap events
  accel_tap_service_subscribe(accel_tap_handler);

//  s_volume = persist_exists(VOLUME_PKEY) ? persist_read_int(VOLUME_PKEY) : VOLUME_DEFAULT;
}
void deinit(void) {
  APP_LOG(APP_LOG_LEVEL_INFO, "<<<DeInit");
//  persist_write_int(VOLUME_PKEY, s_volume);
	app_message_deregister_callbacks();
  battery_state_service_unsubscribe();
  tick_timer_service_unsubscribe();
  connection_service_unsubscribe();
  accel_tap_service_unsubscribe();
  window_destroy(s_main_window);
}
int main( void ) {
	init();
	app_event_loop();
	deinit();
}

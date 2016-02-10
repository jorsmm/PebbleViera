#include <pebble.h>
	
#define REPEAT_INTERVAL_MS 50
#define MAX_BITMAPS 9

#ifdef PBL_SDK_3
//static StatusBarLayer *s_status_bar;
#endif

#define VOLUME_PKEY 1
#define VOLUME_DEFAULT 0

static Window *s_main_window;
static TextLayer *s_label_layer;
static TextLayer *text_time_layer;
static TextLayer *text_day_layer;
static BitmapLayer *s_icon_layer;
static ActionBarLayer *s_action_bar_layer;

static GBitmap *s_icon_bitmap;

static GBitmap* table[MAX_BITMAPS];
static char* titles[MAX_BITMAPS/3];

int offset=2;
static int s_volume = VOLUME_DEFAULT;
static int s_mute = 0;

static int s_status_pending=0;

Layer *bars_layer;
Layer *main_layer;

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

// Key values for AppMessage Dictionary
enum {
	STATUS_KEY = 0,	
	MESSAGE_KEY = 1
};

static int lateral=PBL_IF_RECT_ELSE(0, 25);

/////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////
bool startsWith(const char *pre, const char *str) {
    size_t lenpre = strlen(pre),
           lenstr = strlen(str);
    return lenstr < lenpre ? false : strncmp(pre, str, lenpre) == 0;
}

/////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////
// Write message to buffer & send
void send_message(int status){
  s_status_pending=1;
	DictionaryIterator *iter;
	app_message_outbox_begin(&iter);
	dict_write_uint8(iter, STATUS_KEY, status);
	dict_write_end(iter);
  app_message_outbox_send();
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
  //Time
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
  if (!s_tv_screen_is_on) {
    send_message(100);
  }
}

/////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////
void bars_update_callback(Layer *me, GContext* ctx) {
  (void)me;
  s_ctx=ctx;
  if (offset==0) {
    layer_set_hidden (me, false);
    graphics_context_set_stroke_color(ctx, GColorBlue);
    graphics_draw_line(ctx, GPoint(5, 135),     GPoint(112-7, 135));
    graphics_draw_line(ctx, GPoint(5+lateral, 135+18),   GPoint(112-7+lateral, 135+18));
    graphics_draw_line(ctx, GPoint(4, 136),     GPoint(4, 136+16));
    graphics_draw_line(ctx, GPoint(112-6, 136), GPoint(112-6, 136+16));
  }
  else {
    layer_set_hidden (me, true);
  }
}
void progress_update_callback(Layer *me, GContext* ctx) {
  s_ctx=ctx;  
  if (offset==0) {
    layer_set_hidden (me, false);
    if (s_mute==1) {
      graphics_context_set_stroke_color(ctx, GColorLightGray);
      graphics_context_set_fill_color(ctx, GColorLightGray);
    }
    else {
      graphics_context_set_stroke_color(ctx, GColorCyan);
      graphics_context_set_fill_color(ctx, GColorCyan);
    }
    graphics_fill_rect(ctx, GRect(5+lateral, 136, s_volume, 17), 0, GCornersAll);
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
    text_layer_set_text_color (s_label_layer, GColorBlack);
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
  layer_set_hidden(bitmap_layer_get_layer(s_icon_layer), !s_tv_screen_is_on);
  layer_set_hidden(text_layer_get_layer(s_label_layer), !s_tv_screen_is_on);
  layer_set_hidden(main_layer, !s_tv_screen_is_on);
  layer_set_hidden(bars_layer, !s_tv_screen_is_on);
}
static void tv_screen_is_on() {
  APP_LOG(APP_LOG_LEVEL_DEBUG, "tv_screen_is_on: %d", s_tv_screen_is_on);
  if (!s_tv_screen_is_on) {
    s_tv_screen_is_on = true;
    offset=0;
    update_action_bar_layer();
    update_tv_layers();
  }
}
static void tv_screen_is_off() {
  APP_LOG(APP_LOG_LEVEL_DEBUG, "tv_screen_is_off: %d", s_tv_screen_is_on);
  if (s_tv_screen_is_on) {
    s_tv_screen_is_on = false;
    offset=2;
    update_action_bar_layer();
    update_tv_layers();
    update_text();
  }
}

/////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////
static void select_long_click_handler(ClickRecognizerRef recognizer, void *context) {
  offset=(offset+1)%(MAX_BITMAPS/3);
  update_action_bar_layer();
}
static void increment_click_handler(ClickRecognizerRef recognizer, void *context) {
  if (s_status_pending) {return;}
  if (offset==0) {
    if (s_volume<100) {
      s_volume++;
      s_mute=0;
    }
    else {
      return;
    }
  }
  update_text();
  send_message(offset*3+0x1);
}
static void select_click_handler(ClickRecognizerRef recognizer, void *context) {
  if (offset==0) {
    s_mute=(s_mute+1)%2;
  }
  send_message(offset*3+0x2);
  update_text();
}
static void decrement_click_handler(ClickRecognizerRef recognizer, void *context) {
  if (s_status_pending) {return;}
  if (offset==0) {
    if (s_volume>0) {
      s_mute=0;
      s_volume--;
    }
    else {
      return;
    }
  }
  update_text();
  send_message(offset*3+0x3);
}
static void click_config_provider(void *context) {
  window_single_repeating_click_subscribe(BUTTON_ID_UP, REPEAT_INTERVAL_MS, increment_click_handler);
  window_single_repeating_click_subscribe(BUTTON_ID_DOWN, REPEAT_INTERVAL_MS, decrement_click_handler);
  window_single_click_subscribe(BUTTON_ID_SELECT, select_click_handler); 
  window_long_click_subscribe(BUTTON_ID_SELECT, 400, select_long_click_handler, NULL);
}

/////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////
static void window_load(Window *window) {
  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(window_layer);

#ifdef PBL_SDK_3
  // Set up the status bar last to ensure it is on top of other Layers
//  s_status_bar = status_bar_layer_create();
//  layer_add_child(window_layer, status_bar_layer_get_layer(s_status_bar));
//  status_bar_layer_set_colors(s_status_bar, GColorGreen, GColorBlack);
#endif
  
  // el dia
  const GEdgeInsets day_insets = {.top = -5, .right = 117, .bottom = 140, .left = -2 + lateral };
  text_day_layer = text_layer_create(grect_inset(bounds, day_insets));
  text_layer_set_background_color(text_day_layer, GColorBlue);
  text_layer_set_text_color (text_day_layer, GColorWhite);
  text_layer_set_text_alignment(text_day_layer, GTextAlignmentCenter);
  text_layer_set_font(text_day_layer, fonts_get_system_font(FONT_KEY_GOTHIC_28_BOLD));
  layer_add_child(window_layer, text_layer_get_layer(text_day_layer));

  // la hora
  const GEdgeInsets time_insets = {.top = -5, .right = ACTION_BAR_WIDTH +4, .bottom = 145, .left = ACTION_BAR_WIDTH / 3 - 4 + lateral };
  text_time_layer = text_layer_create(grect_inset(bounds, time_insets));
  text_layer_set_background_color(text_time_layer, GColorClear);
  text_layer_set_text_color (text_time_layer, GColorBlue);
  text_layer_set_text_alignment(text_time_layer, GTextAlignmentCenter);
  text_layer_set_font(text_time_layer, fonts_get_system_font(FONT_KEY_GOTHIC_28_BOLD));
  layer_add_child(window_layer, text_layer_get_layer(text_time_layer));

    // Create battery meter Layer
  s_battery_icon_bitmap = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_BATTERY);
  s_battery_icon_layer = bitmap_layer_create(GRect(88, 2, 24, 24));
  bitmap_layer_set_bitmap(s_battery_icon_layer, s_battery_icon_bitmap);
  layer_add_child(window_get_root_layer(window), bitmap_layer_get_layer(s_battery_icon_layer));

  s_battery_layer = layer_create(GRect(90, 10, 19, 8));
  layer_set_update_proc(s_battery_layer, battery_update_proc);
  layer_add_child(window_get_root_layer(window), s_battery_layer);

  text_battery_layer = text_layer_create(GRect(90, 8, 19, 9));
  text_layer_set_background_color(text_battery_layer, GColorClear);
  text_layer_set_text_color (text_battery_layer, GColorBlack);
  text_layer_set_text_alignment(text_battery_layer, GTextAlignmentCenter);
  text_layer_set_font(text_battery_layer, fonts_get_system_font(FONT_KEY_GOTHIC_09));
  layer_add_child(window_layer, text_layer_get_layer(text_battery_layer));

  // Ensure battery level is displayed from the start
  battery_callback(battery_state_service_peek());  
  
  // Create the Bluetooth icon GBitmap
  s_bt_icon_bitmap = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_BT_ICON);
  s_bt_icon_layer = bitmap_layer_create(GRect(90, 2, 24, 24));
  bitmap_layer_set_background_color(s_bt_icon_layer, GColorRed);
  bitmap_layer_set_bitmap(s_bt_icon_layer, s_bt_icon_bitmap);
  layer_add_child(window_get_root_layer(window), bitmap_layer_get_layer(s_bt_icon_layer));
  // Show the correct state of the BT connection from the start
  bluetooth_callback(connection_service_peek_pebble_app_connection());
  
  // icono central
  s_icon_bitmap = gbitmap_create_with_resource(RESOURCE_ID_TVC);
  const GEdgeInsets icon_insets = {.top = 32, .right = ACTION_BAR_WIDTH, .bottom = 46, .left = ACTION_BAR_WIDTH / 3};
  s_icon_layer = bitmap_layer_create(grect_inset(bounds, icon_insets));
  bitmap_layer_set_bitmap(s_icon_layer, s_icon_bitmap);
  bitmap_layer_set_compositing_mode(s_icon_layer, GCompOpSet);
  layer_set_hidden(bitmap_layer_get_layer(s_icon_layer), !s_tv_screen_is_on);
  layer_add_child(window_layer, bitmap_layer_get_layer(s_icon_layer));

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
  const GEdgeInsets label_insets = {.top = 127, .right = ACTION_BAR_WIDTH, .left = ACTION_BAR_WIDTH / 3 + lateral};
  s_label_layer = text_layer_create(grect_inset(bounds, label_insets));
  text_layer_set_background_color(s_label_layer, GColorClear);
  text_layer_set_text_alignment(s_label_layer, GTextAlignmentCenter);
  text_layer_set_font(s_label_layer, fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD));
  layer_set_hidden(text_layer_get_layer(s_label_layer), !s_tv_screen_is_on);
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

/////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////

// Called when a message is received from PebbleKitJS
static void in_received_handler(DictionaryIterator *received, void *context) {
	Tuple *tuple;
	tuple = dict_find(received, STATUS_KEY);
	if(tuple) {
    s_status_pending=0;
		APP_LOG(APP_LOG_LEVEL_DEBUG, "Received Status: %d", (int)tuple->value->uint32);
    if ((int)tuple->value->uint32 == 0) {
      	tv_screen_is_on();
    }
    else if ((int)tuple->value->uint32 == 104) {
      	tv_screen_is_off();
    }
	}
	tuple = dict_find(received, MESSAGE_KEY);
	if(tuple) {
		APP_LOG(APP_LOG_LEVEL_DEBUG, "Received Message: %s", tuple->value->cstring);
    if (startsWith("v=",tuple->value->cstring)) {
      size_t lenstring = strlen(tuple->value->cstring);
      int tamano=lenstring-2;
      char svolume[tamano+1];
      memcpy( svolume, &tuple->value->cstring[2], tamano );
      svolume[tamano] = '\0';
      s_volume = atoi( svolume );
      APP_LOG(APP_LOG_LEVEL_DEBUG, "Received VOLUME: %u", s_volume);
      update_text();
    }
    else if (startsWith("m=",tuple->value->cstring)) {
      size_t lenstring = strlen(tuple->value->cstring);
      int tamano=lenstring-2;
      char smute[tamano+1];
      memcpy( smute, &tuple->value->cstring[2], tamano );
      smute[tamano] = '\0';
      s_mute = atoi( smute );
      APP_LOG(APP_LOG_LEVEL_DEBUG, "Received MUTE: %u", s_mute);
      update_text();
    }

	}
}
// Called when an incoming message from PebbleKitJS is dropped
static void in_dropped_handler(AppMessageResult reason, void *context) {	
}
// Called when PebbleKitJS does not acknowledge receipt of a message
static void out_failed_handler(DictionaryIterator *failed, AppMessageResult reason, void *context) {
}

/////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////

void init(void) {
  APP_LOG(APP_LOG_LEVEL_DEBUG, ">>>Init");
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
//  s_volume = persist_exists(VOLUME_PKEY) ? persist_read_int(VOLUME_PKEY) : VOLUME_DEFAULT;
}
void deinit(void) {
  APP_LOG(APP_LOG_LEVEL_DEBUG, "<<<DeInit");
//  persist_write_int(VOLUME_PKEY, s_volume);
	app_message_deregister_callbacks();
  battery_state_service_unsubscribe();
  tick_timer_service_unsubscribe();
  connection_service_unsubscribe();
	window_destroy(s_main_window);
}
int main( void ) {
	init();
	app_event_loop();
	deinit();
}
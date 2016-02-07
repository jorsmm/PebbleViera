#include <pebble.h>
	
#define REPEAT_INTERVAL_MS 50
#define MAX_BITMAPS 9

#ifdef PBL_SDK_3
static StatusBarLayer *s_status_bar;
#endif

static Window *s_main_window;
static TextLayer *s_label_layer;
static BitmapLayer *s_icon_layer;
static ActionBarLayer *s_action_bar_layer;

static GBitmap *s_icon_bitmap;

static GBitmap* table[MAX_BITMAPS];
static char* titles[MAX_BITMAPS/3];

int offset=0;

// Key values for AppMessage Dictionary
enum {
	STATUS_KEY = 0,	
	MESSAGE_KEY = 1
};

// Write message to buffer & send
void send_message(int status){
	DictionaryIterator *iter;
	
	app_message_outbox_begin(&iter);
	dict_write_uint8(iter, STATUS_KEY, status);
	
	dict_write_end(iter);
  	app_message_outbox_send();
}

static void update_action_bar_layer() {
  action_bar_layer_set_icon(s_action_bar_layer, BUTTON_ID_UP, table[offset*3]);
  action_bar_layer_set_icon(s_action_bar_layer, BUTTON_ID_SELECT, table[(offset*3)+1]);
  action_bar_layer_set_icon(s_action_bar_layer, BUTTON_ID_DOWN, table[(offset*3)+2]); 
  text_layer_set_text(s_label_layer, titles[offset]);

} 

static void select_long_click_handler(ClickRecognizerRef recognizer, void *context) {
  offset=(offset+1)%(MAX_BITMAPS/3);
  update_action_bar_layer();
}
static void increment_click_handler(ClickRecognizerRef recognizer, void *context) {
  send_message(offset*3+0x1);
}
static void select_click_handler(ClickRecognizerRef recognizer, void *context) {
  send_message(offset*3+0x2);
}
static void decrement_click_handler(ClickRecognizerRef recognizer, void *context) {
    send_message(offset*3+0x3);
}

static void click_config_provider(void *context) {
  window_single_repeating_click_subscribe(BUTTON_ID_UP, REPEAT_INTERVAL_MS, increment_click_handler);
  window_single_repeating_click_subscribe(BUTTON_ID_DOWN, REPEAT_INTERVAL_MS, decrement_click_handler);
  window_single_click_subscribe(BUTTON_ID_SELECT, select_click_handler); 
  window_long_click_subscribe(BUTTON_ID_SELECT, 400, select_long_click_handler, NULL);
}


static void window_load(Window *window) {
  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(window_layer);

#ifdef PBL_SDK_3
  // Set up the status bar last to ensure it is on top of other Layers
  s_status_bar = status_bar_layer_create();
  layer_add_child(window_layer, status_bar_layer_get_layer(s_status_bar));
  status_bar_layer_set_colors(s_status_bar, GColorGreen, GColorBlack);
#endif
  
  s_icon_bitmap = gbitmap_create_with_resource(RESOURCE_ID_TVC);

  const GEdgeInsets icon_insets = {.top = 27, .right = 28, .bottom = 46, .left = 9};
  s_icon_layer = bitmap_layer_create(grect_inset(bounds, icon_insets));
  bitmap_layer_set_bitmap(s_icon_layer, s_icon_bitmap);
  bitmap_layer_set_compositing_mode(s_icon_layer, GCompOpSet);
  layer_add_child(window_layer, bitmap_layer_get_layer(s_icon_layer));

  const GEdgeInsets label_insets = {.top = 122, .right = ACTION_BAR_WIDTH, .left = ACTION_BAR_WIDTH / 3 };
  s_label_layer = text_layer_create(grect_inset(bounds, label_insets));
  text_layer_set_background_color(s_label_layer, GColorClear);
  text_layer_set_text_alignment(s_label_layer, GTextAlignmentCenter);
  text_layer_set_font(s_label_layer, fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD));
  layer_add_child(window_layer, text_layer_get_layer(s_label_layer));

  titles[0]="Volume";
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
  action_bar_layer_destroy(s_action_bar_layer);
  bitmap_layer_destroy(s_icon_layer);
  int i=0;
  for (i=0; i< MAX_BITMAPS; i++) {
    gbitmap_destroy(table[i]);
  }

  window_destroy(window);
  s_main_window = NULL;
}

// Called when a message is received from PebbleKitJS
static void in_received_handler(DictionaryIterator *received, void *context) {
	Tuple *tuple;
	tuple = dict_find(received, STATUS_KEY);
	if(tuple) {
		APP_LOG(APP_LOG_LEVEL_DEBUG, "Received Status: %d", (int)tuple->value->uint32); 
	}
	tuple = dict_find(received, MESSAGE_KEY);
	if(tuple) {
		APP_LOG(APP_LOG_LEVEL_DEBUG, "Received Message: %s", tuple->value->cstring);
	}
}
// Called when an incoming message from PebbleKitJS is dropped
static void in_dropped_handler(AppMessageResult reason, void *context) {	
}
// Called when PebbleKitJS does not acknowledge receipt of a message
static void out_failed_handler(DictionaryIterator *failed, AppMessageResult reason, void *context) {
}

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
}
void deinit(void) {
  APP_LOG(APP_LOG_LEVEL_DEBUG, "<<<DeInit");
	app_message_deregister_callbacks();
	window_destroy(s_main_window);
}
int main( void ) {
	init();
	app_event_loop();
	deinit();
}
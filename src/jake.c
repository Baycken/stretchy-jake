#include <pebble.h>

static Window *window;
static BitmapLayer *background_layer;
static GBitmap *background_image;
static TextLayer *text;
static char time_text[10];
void handle_minuit_tick(struct tm *tick_time, TimeUnits units_changed){
    time_text[0] = ((tick_time->tm_year)%100)/10 + 48;
    time_text[1] = ((tick_time->tm_year)%10) + 48;
    time_text[2] = ((tick_time->tm_mon)/10) + 48;
    time_text[3] = ((tick_time->tm_mon)%10) + 49;
    time_text[4] = ((tick_time->tm_mday)/10) + 48;
    time_text[5] = ((tick_time->tm_mday)%10) + 48;
    time_text[6] = ((tick_time->tm_hour)/10) + 48;
    time_text[7] = ((tick_time->tm_hour)%10) + 48;
    time_text[8] = ((tick_time->tm_min)/10) + 48;
    time_text[9] = ((tick_time->tm_min)%10) + 48;
    text_layer_set_text(text,time_text);
}


static void window_load(Window *window) {
    Layer *window_layer = window_get_root_layer(window);
    background_layer = bitmap_layer_create(GRect(0,0,144,168));
    background_image = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_BACKGROUND);
    bitmap_layer_set_bitmap(background_layer,background_image);
    bitmap_layer_set_alignment(background_layer, GAlignCenter);
    text = text_layer_create(GRect(0,100,144,30));
    text_layer_set_text_alignment(text,GTextAlignmentCenter);
    text_layer_set_background_color(text,GColorClear);
    layer_add_child(window_layer, bitmap_layer_get_layer(background_layer));
    layer_add_child(window_layer, text_layer_get_layer(text));
    tick_timer_service_subscribe(MINUTE_UNIT,handle_minuit_tick);
    
}

static void window_unload(Window *window) {
    tick_timer_service_unsubscribe();
    text_layer_destroy(text);
    gbitmap_destroy(background_image);
    bitmap_layer_destroy(background_layer);
}

static void init(void) {
    window = window_create();
    window_set_window_handlers(window, (WindowHandlers) {
        .load = window_load,
        .unload = window_unload,
    });
    const bool animated = true;
    window_stack_push(window, animated);
    
}

static void deinit(void) {
    
    window_destroy(window);
}

int main(void) {
    init();
    app_event_loop();
    deinit();
}

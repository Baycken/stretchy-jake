#include <pebble.h>

#define ANIMATION_DURATION 400
#define AMERICAN_FORMAT

static Window *window;
static BitmapLayer *background_layer;
static GBitmap *background_image;

static BitmapLayer *battery_low_layer;
static GBitmap *battery_low_img;

static Layer *time_layer;
static BitmapLayer *time_num_layer[5];
static GBitmap *time_num_img[5];

static Layer *date_layer;
static BitmapLayer *date_num_layer[7];
static GBitmap *date_num_img[7];

static bool initialized = false;

static int time_cache[4]={-1,-1,-1,-1}; //hour 10s, hour 1s,minit 10s,minut 1s
static int date_cache[7]={-1,-1,-1,-1,-1,-1,-1};//wday, month 10s, month 1s, date 10s, date 1s, year 10s, year 1s
static int time_now[4];
static int date_now[7];

static GRect time_from_rect;
static GRect time_to_rect;
static GRect date_from_rect;
static GRect date_to_rect;
static PropertyAnimation *time_animation_beg;
static PropertyAnimation *time_animation_end;
static PropertyAnimation *date_animation_beg;
static PropertyAnimation *date_animation_end;

static const char time_num_position[5][4] = {
    {  0,0,26,34}, //hour 10s
    { 29,0,26,34}, //hour 1s
    { 73,0,26,34}, //minuit 10s
    {102,0,26,34},  //minuit 1s
    { 58,0,12,34} // :
};
#ifdef AMERICAN_FORMAT
static const char date_num_position[7][4] ={
    {  0,0,25,15}, //day
    { 32,0,12,15}, //month 10s
    { 45,0,12,15}, //month 1s
    { 63,0,12,15}, //date 10s
    { 76,0,12,15}, //date 1s
    { 93,0,12,15}, //year 10s
    {106,0,12,15}, //year 1s
};
#else
static const char date_num_position[7][4] ={
    {  0,0,25,15}, //day
    { 63,0,12,15}, //date 10s
    { 76,0,12,15}, //date 1s
    { 32,0,12,15}, //month 10s
    { 45,0,12,15}, //month 1s
    { 93,0,12,15}, //year 10s
    {106,0,12,15}, //year 1s
};
#endif

const int time_num_id[10]={
    RESOURCE_ID_IMAGE_NUM_L_0,
    RESOURCE_ID_IMAGE_NUM_L_1,
    RESOURCE_ID_IMAGE_NUM_L_2,
    RESOURCE_ID_IMAGE_NUM_L_3,
    RESOURCE_ID_IMAGE_NUM_L_4,
    RESOURCE_ID_IMAGE_NUM_L_5,
    RESOURCE_ID_IMAGE_NUM_L_6,
    RESOURCE_ID_IMAGE_NUM_L_7,
    RESOURCE_ID_IMAGE_NUM_L_8,
    RESOURCE_ID_IMAGE_NUM_L_9
};

const int date_num_id[10] = {
    RESOURCE_ID_IMAGE_NUM_S_0,
    RESOURCE_ID_IMAGE_NUM_S_1,
    RESOURCE_ID_IMAGE_NUM_S_2,
    RESOURCE_ID_IMAGE_NUM_S_3,
    RESOURCE_ID_IMAGE_NUM_S_4,
    RESOURCE_ID_IMAGE_NUM_S_5,
    RESOURCE_ID_IMAGE_NUM_S_6,
    RESOURCE_ID_IMAGE_NUM_S_7,
    RESOURCE_ID_IMAGE_NUM_S_8,
    RESOURCE_ID_IMAGE_NUM_S_9
};

const int week_num_id[7] = {
    RESOURCE_ID_IMAGE_WEEK_0,
    RESOURCE_ID_IMAGE_WEEK_1,
    RESOURCE_ID_IMAGE_WEEK_2,
    RESOURCE_ID_IMAGE_WEEK_3,
    RESOURCE_ID_IMAGE_WEEK_4,
    RESOURCE_ID_IMAGE_WEEK_5,
    RESOURCE_ID_IMAGE_WEEK_6
};

void time_animation_stopped(Animation *animation, void *data) {
    (void)animation;
    (void)data;
    
    property_animation_destroy(time_animation_end);
    time_animation_end= property_animation_create_layer_frame(time_layer,&time_from_rect,&time_to_rect);
    animation_set_curve(    &time_animation_end->animation, AnimationCurveLinear);
    animation_set_duration( &time_animation_end->animation, ANIMATION_DURATION);
    
    //set time image
    for(int i =0; i<4;i++){
        if(time_now[i]!=time_cache[i]){
            time_cache[i]=time_now[i];
            gbitmap_destroy(time_num_img[i]);
            time_num_img[i]= gbitmap_create_with_resource(time_num_id[time_now[i]]);
            bitmap_layer_set_bitmap(time_num_layer[i],time_num_img[i]);
            
        }
    }
    //set column
    if(time_num_img[4] == NULL){
        time_num_img[4]= gbitmap_create_with_resource(RESOURCE_ID_IMAGE_COLUMN);
        bitmap_layer_set_bitmap(time_num_layer[4],time_num_img[4]);
    }
    
    layer_set_hidden(bitmap_layer_get_layer(battery_low_layer),battery_state_service_peek().charge_percent > 25); //charge indication
    layer_mark_dirty(time_layer);
    animation_schedule((Animation*) time_animation_end);
}

void date_animation_stopped(Animation *animation, void *data) {
    (void)animation;
    (void)data;
    
    property_animation_destroy(date_animation_end);
    date_animation_end= property_animation_create_layer_frame(date_layer,&date_from_rect,&date_to_rect);
    animation_set_curve(    &date_animation_end->animation, AnimationCurveLinear);
    animation_set_duration( &date_animation_end->animation, ANIMATION_DURATION);

    //set week day
    if (date_now[0]!= date_cache[0]) {
        date_cache[0] = date_now[0];
        gbitmap_destroy(date_num_img[0]);
        date_num_img[0]= gbitmap_create_with_resource(week_num_id[date_now[0]]);
        bitmap_layer_set_bitmap(date_num_layer[0],date_num_img[0]);
    }
    
    //set date image
    for(int i =1; i<7;i++){
        if(date_now[i]!=date_cache[i]){
            date_cache[i]=date_now[i];
            gbitmap_destroy(date_num_img[i]);
            date_num_img[i]= gbitmap_create_with_resource(date_num_id[date_now[i]]);
            bitmap_layer_set_bitmap(date_num_layer[i],date_num_img[i]);   
        }   
    }
    
    layer_mark_dirty(date_layer);
    animation_schedule((Animation*) date_animation_end);
}

//Parsing time and display time
void display_time(struct tm *tick_time){
  //hour
  int hour = tick_time->tm_hour;
  //12h and 24 hour support
  if(!clock_is_24h_style()){
    hour = hour % 12;
      if(hour == 0){
        hour = 12;
      }
    }
  time_now[0] = hour/10;
  time_now[1] = hour%10;
  
  //minuit
  int minuit = tick_time->tm_min;
  time_now[2] = minuit/10;
  time_now[3] = minuit%10;
  
  //start animations
  property_animation_destroy(time_animation_beg);
  time_animation_beg= property_animation_create_layer_frame(time_layer,&time_to_rect,&time_from_rect);
  animation_set_curve(&time_animation_beg->animation, AnimationCurveLinear);
  animation_set_duration( &time_animation_beg->animation, ANIMATION_DURATION);
  animation_set_handlers((Animation*) time_animation_beg, (AnimationHandlers){
    .started = NULL,
    .stopped = (AnimationStoppedHandler) time_animation_stopped
  }, 0);
  animation_schedule((Animation*) time_animation_beg);
}

//Parsing data and display date
void display_date(struct tm *tick_time){
  //Day of the week
  date_now[0] = tick_time->tm_wday;
  
  //Month
  int month = tick_time->tm_mon + 1;
  date_now[1] = month / 10;
  date_now[2] = month % 10;
  //Date 
  
  int date = tick_time->tm_mday;
  date_now[3] = date / 10;
  date_now[4] = date % 10;
  
  //Year
  int year = (tick_time->tm_year) % 100;
  date_now[5] = year / 10;
  date_now[6] = year % 10;
  
  //start animation
  if(date_now[4] != date_cache[4]){
    property_animation_destroy(date_animation_beg);
    date_animation_beg= property_animation_create_layer_frame(date_layer,&date_to_rect,&date_from_rect);
    animation_set_curve(&date_animation_beg->animation, AnimationCurveLinear);
    animation_set_duration( &date_animation_beg->animation, ANIMATION_DURATION);
    animation_set_handlers((Animation*) date_animation_beg, (AnimationHandlers){
      .started = NULL,
      .stopped = (AnimationStoppedHandler) date_animation_stopped
    }, 0);
    animation_schedule((Animation*) date_animation_beg); 
  }  
}
  

void handle_minuit_tick(struct tm *tick_time, TimeUnits units_changed){
  
  display_time(tick_time);
    
  if (((units_changed & DAY_UNIT) == DAY_UNIT) || (!initialized)) {  
    initialized = true;
    display_date(tick_time);
  }
  
}




static void window_load(Window *window) {
    Layer *window_layer = window_get_root_layer(window);
    
    time_from_rect = GRect(8,108,136,0);
    time_to_rect = GRect(8,76,136,34);
    date_from_rect = GRect(13,140,131,0);
    date_to_rect = GRect(13,127,131,15);
//init background layer
    background_layer = bitmap_layer_create(GRect(0,1,144,166));
    background_image = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_BACKGROUND);
    bitmap_layer_set_bitmap(background_layer,background_image);
    bitmap_layer_set_alignment(background_layer, GAlignCenter);
    layer_add_child(window_layer, bitmap_layer_get_layer(background_layer));
//init time layer
    time_layer = layer_create(time_from_rect);
    layer_set_clips(time_layer,1);
    layer_add_child(window_layer,time_layer);
    for(int i = 0; i<5 ;i++){
        time_num_layer[i] = bitmap_layer_create(GRect(time_num_position[i][0],time_num_position[i][1],time_num_position[i][2],time_num_position[i][3]));
        layer_add_child(time_layer,bitmap_layer_get_layer(time_num_layer[i]));
    }
//init date layer
  date_layer = layer_create(date_to_rect);
  layer_set_clips(date_layer,1);
  layer_add_child(window_layer,date_layer);
  for(int i = 0; i<7 ;i++){
    date_num_layer[i] = bitmap_layer_create(GRect(date_num_position[i][0],date_num_position[i][1],date_num_position[i][2],date_num_position[i][3]));
    layer_add_child(date_layer,bitmap_layer_get_layer(date_num_layer[i]));
  }
  
  //init battery low layer
  battery_low_layer = bitmap_layer_create(GRect(66,3,12,31));
  battery_low_img = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_BAT_LOW);
  bitmap_layer_set_bitmap(battery_low_layer,battery_low_img);
  layer_set_hidden(bitmap_layer_get_layer(battery_low_layer),1);
  layer_add_child(window_layer,bitmap_layer_get_layer(battery_low_layer));
  
  //init time tick callback
  tick_timer_service_subscribe(MINUTE_UNIT,handle_minuit_tick);
    
}

static void window_unload(Window *window) {
    initialized = false;
    tick_timer_service_unsubscribe();
    property_animation_destroy(time_animation_beg);
    property_animation_destroy(time_animation_end);
    property_animation_destroy(date_animation_beg);
    property_animation_destroy(date_animation_end);

    for(int i = 0; i<5 ;i++){
        gbitmap_destroy(time_num_img[i]);
        bitmap_layer_destroy(time_num_layer[i]);
    }
    
    for(int i = 0; i<7 ;i++){
        gbitmap_destroy(date_num_img[i]);
        bitmap_layer_destroy(date_num_layer[i]);
    }
    
    gbitmap_destroy(battery_low_img);
    bitmap_layer_destroy(battery_low_layer);
    
    layer_destroy(time_layer);
    layer_destroy(date_layer);
    
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

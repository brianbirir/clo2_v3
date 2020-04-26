#include "Particle.h"
#include "neopixel.h"

#include <include/clo2_peripherals.h>
#include <include/clo2_process.h>
#include <include/serial_debug.h>

// #define DEBUG_CLO2_PERIPHERALS

/* local variables */
extern unsigned long ALWAYS_ON;
extern unsigned long HALF_A_SECOND;
extern unsigned long ONE_SECOND;
extern unsigned long TWO_SECONDS;
// fans
#define fan_one_pin D3
#define fan_two_pin D4
bool fan_state = false;
// button
#define button_pin D6
unsigned long button_timer;
unsigned long button_poll_interval = ONE_SECOND; 
uint8_t button_counter;
uint8_t actionable_button_counter;
// buzzer
#define buzzer_pin D8
volatile bool buzzing_state;
unsigned long buzzer_timer;
unsigned long buzzer_period = 100; //One second
static int buzzer_counter;
// on board led
#define on_board_led_pin D7
unsigned long heart_beat_timer = 0;
unsigned long heart_beat_duration = ONE_SECOND;
// pixel led
#define pixel_pin D2
#define PIXEL_COUNT 1
#define PIXEL_TYPE WS2812B
Adafruit_NeoPixel strip(PIXEL_COUNT, pixel_pin, PIXEL_TYPE);
bool pixel_flag;
unsigned long pixel_period = HALF_A_SECOND;
unsigned long pixel_timer;
#define GREEN 0x00FF00 // color green

extern volatile bool current_system_state;
extern trigger_source current_trigger_source;

// local functions
void clo2_peripherals_setup_buzzer(void);
void clo2_peripherals_buzzer_handle(void);
void clo2_peripherals_setup_on_board_led(void);
void clo2_peripherals_heartbeat(void);
void clo2_peripherals_setup_fans(void);
bool clo2_peripherals_get_fan_state(void);
void clo2_peripherals_set_fans(void);
void clo2_peripherals_reset_fans(void);
void clo2_peripherals_toggle_fans(bool _state);
void clo2_peripherals_setup_button(void);
void clo2_peripherals_button_poll(void);
void clo2_peripherals_action_button_press(void);
void clo2_button_processor(void);
void clo2_peripheral_setup_neopixel(void);
void clo2_peripheral_flash_neopixel(void);
void clo2_peripheral_set_pixel_color(uint32_t c); 
void clo2_peripherals_setup(void);

void clo2_peripherals_machine(void)
{
  clo2_button_processor();
  clo2_peripherals_heartbeat();
  clo2_peripherals_buzzer_handle();
  clo2_peripheral_flash_neopixel();
}

void clo2_peripheral_prep_state(void)
{
  buzzing_state = true;
  buzzer_counter = 4;
  clo2_peripherals_reset_fans();
  pixel_period = TWO_SECONDS;
}

void clo2_peripheral_chlorination_state(void)
{
  buzzing_state = true;
  buzzer_counter = 4;
  clo2_peripherals_set_fans();
  pixel_period = ALWAYS_ON;
}

void clo2_peripheral_aftermath_state(void)
{
  buzzing_state = false;
  clo2_peripherals_reset_fans();
  pixel_period = HALF_A_SECOND;
}

void clo2_peripheral_idle_state(void)
{
  buzzing_state = false;
  clo2_peripherals_reset_fans();
  pixel_period = HALF_A_SECOND;
}


/* buzzer */
void clo2_peripherals_setup_buzzer(void)
{
  pinMode(buzzer_pin, OUTPUT);
  digitalWrite(buzzer_pin, true);
  buzzing_state = false;
  buzzer_counter = false;
  buzzer_timer = 0;
}

void clo2_peripherals_buzzer_handle(void)
{
    if(buzzing_state) {
        if((millis() - buzzer_timer) > buzzer_period) {
            digitalWrite(buzzer_pin, !digitalRead(buzzer_pin));
            buzzer_counter--;
            buzzer_timer = millis();
        }
        if(buzzer_counter <= 0) {
            digitalWrite(buzzer_pin, true);
            buzzing_state = false;
        }
    }
    else
    {
      digitalWrite(buzzer_pin, true); // keep buzzer off
    }
}
/* end of buzzer */

/* on board led */
void clo2_peripherals_setup_on_board_led(void)
{
  pinMode(on_board_led_pin, OUTPUT);
  digitalWrite(on_board_led_pin, false);
}

void clo2_peripherals_heartbeat(void)
{
  if ( (millis() - heart_beat_timer) > heart_beat_duration )
  {
    heart_beat_timer = millis();
    digitalWrite(on_board_led_pin, !digitalRead(on_board_led_pin));
  }
}
/* end of on board led */


/* Fans */
void clo2_peripherals_setup_fans(void)
{
  pinMode(fan_one_pin, OUTPUT);
  pinMode(fan_two_pin, OUTPUT);
  digitalWrite(fan_one_pin, fan_state);
  digitalWrite(fan_two_pin, fan_state);
}

bool clo2_peripherals_get_fan_state(void)
{
  return digitalRead(fan_one_pin);
}

void clo2_peripherals_set_fans(void)
{
  digitalWrite(fan_one_pin, true);
  digitalWrite(fan_two_pin, true);
}

void clo2_peripherals_reset_fans(void)
{
  digitalWrite(fan_one_pin, false);
  digitalWrite(fan_two_pin, false);
}

void clo2_peripherals_toggle_fans(bool _state)
{
  if(_state)
  {
    clo2_peripherals_set_fans();
  }
  else
  {
    clo2_peripherals_reset_fans();
  }
}
/* end of fans */

/* Button functions */
void clo2_peripherals_setup_button(void)
{
  pinMode(button_pin, INPUT_PULLUP);
  button_counter = 0;
  actionable_button_counter = 0;
  button_timer = 0;
}

void clo2_peripherals_button_poll(void) 
{
  if((millis() - button_timer) > button_poll_interval)
  { // check button state every second
    if(!digitalRead(button_pin)) 
    { //if the button is pressed, increase button counter
      button_counter++;
      actionable_button_counter = button_counter;
#ifdef DEBUG_CLO2_PERIPHERALS
      serial_debug_print("Actionable counter: ");
      serial_debug_println(String(actionable_button_counter));
#endif     
    }
    else 
    {
      button_counter = 0; // reset to zero
    }
    button_timer = millis(); //reset_timer;
  }  
}

void clo2_peripherals_action_button_press(void) 
{ 
  if(button_counter == 0) 
  { //check if there has been a button press not yet actioned
    if(actionable_button_counter < 1) 
    {
      actionable_button_counter = 0; //not held long enough, reset
    }
    else if(actionable_button_counter >= 1) 
    {
      current_system_state = !current_system_state; 
      current_trigger_source = TRIG_BUTTON;
      buzzer_counter = 4;
      buzzing_state = true;
      actionable_button_counter = 0; // reset actionable counter to zero
#ifdef DEBUG_CLO2_PERIPHERALS
      serial_debug_print("System state: ");
      serial_debug_println(String(current_system_state));
#endif   
    }
  }
}

void clo2_button_processor(void)
{
  clo2_peripherals_button_poll();
  clo2_peripherals_action_button_press();
}
/* end of button */

/* neo pixel functions */

void clo2_peripheral_setup_neopixel(void)
{
  strip.begin();
  strip.show(); // Initialize all pixels to 'off'
  pixel_flag = false;
  pixel_timer = 0;
}

void clo2_peripheral_set_pixel_color(uint32_t c) 
{
  for(int i=0; i<strip.numPixels(); i++) 
  {
    strip.setPixelColor(i, c);
  }
  strip.show();
}

void clo2_peripheral_flash_neopixel(void) 
{
    if((millis() - pixel_timer) > pixel_period) 
    {
        pixel_flag = !pixel_flag;
        if(pixel_period == ALWAYS_ON) {
            // force the pixel to stay on
            pixel_flag = true;
        }
        // toggle pixel
        if(pixel_flag) {
            clo2_peripheral_set_pixel_color(GREEN);
        }
        else{
            clo2_peripheral_set_pixel_color(0);
        }
        pixel_timer = millis();
    }
}

/* end of neo pixel */

/* peripherals */
void clo2_peripherals_setup(void)
{
  clo2_peripherals_setup_button();
  clo2_peripherals_setup_on_board_led();
  clo2_peripherals_setup_fans();
  clo2_peripherals_setup_buzzer();
  clo2_peripheral_setup_neopixel();
}

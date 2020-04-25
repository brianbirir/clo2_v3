#include "Particle.h"
#include <include/clo2_process.h>
#include <include/ruleblox_mqtt.h>
#include <include/clo2_peripherals.h>

#include <include/serial_debug.h>

// #define DEBUG_CLO2_PROCESS

String system_op_states_str[4] =
{
  "IDLE",
  "PREPARATION",
  "CHLORINATION",
  "AFTERMATH"
};
system_op_states current_system_op_state;

String trigger_source_str[3] =
{
    "TRIG_BUTTON",
    "TRIG_MQTT",
    "UNKNOWN_TRIG"
};
volatile trigger_source current_trigger_source;

volatile bool current_system_state;
volatile bool prev_system_state;

extern unsigned long ONE_SECOND; // in mS
extern unsigned long ONE_MIN;

volatile unsigned long system_timer;
unsigned long default_time;
volatile unsigned long prep_time;
volatile unsigned long chlorination_time;
unsigned long aftermath_time;

unsigned long status_counter = 0;

#define RUNNING_TIME (millis() - system_timer)
#define PREP_TIME (prep_time)
#define CHLORINATION_TIME (prep_time + chlorination_time)
#define AFTERMATH_TIME (prep_time + chlorination_time + aftermath_time)

String status_counter_name = "STATUS";
String system_state_name = "SYSTEM_STATE";
String system_op_state_name = "SYSTEM_OP_STATE";
String trigger_source_name = "TRIGGER_SOURCE";

extern String update_prep_time;
extern String update_chlorination_time;
extern String true_;
extern String false_;

/* local functions */
bool clo2_process_system_state_change_publisher(void);
bool clo2_process_publish_data(void);
void clo2_process_state_monitor(void);
void clo2_process_state_change_monitor(void);

void clo2_process_state_change_monitor(void)
{
  if(current_system_state != prev_system_state)
  {
    if ( current_system_state == true ) 
    {
      // send notification
      clo2_process_system_state_change_publisher();
      // start system timer
      system_timer = millis();
    }

    if ( current_system_state == false ) 
    {
      // send notification
      clo2_process_system_state_change_publisher();
      // start system timer
      system_timer = millis();
    }
    prev_system_state = current_system_state;
  }
}


void clo2_process_state_monitor(void)
{
  if ( current_system_state )
  {
    if ( RUNNING_TIME <=  PREP_TIME)
    {
      if( current_system_op_state == IDLE)
      {
        // update state only once if system was idle
        current_system_op_state = PREPARATION;
        clo2_peripheral_prep_state();
#ifdef DEBUG_CLO2_PROCESS
        serial_debug_print("OP STATE: ");
        serial_debug_println(system_op_states_str[current_system_op_state]);
#endif
        // publish data
        clo2_process_publish_data();
      }
    }
    else if ( (RUNNING_TIME > PREP_TIME) && (RUNNING_TIME <= CHLORINATION_TIME) )
    {
      if( current_system_op_state == PREPARATION)
      {
        // update state only once if system was in prep state
        current_system_op_state = CHLORINATION;
        clo2_peripheral_chlorination_state();
#ifdef DEBUG_CLO2_PROCESS
        serial_debug_print("OP STATE: ");
        serial_debug_println(system_op_states_str[current_system_op_state]);
#endif
        // publish data
        clo2_process_publish_data();
      }
    }
    else if ( (RUNNING_TIME > CHLORINATION_TIME) && (RUNNING_TIME <= AFTERMATH_TIME) )
    {
      if( current_system_op_state == CHLORINATION)
      {
        // update state only once if system was in chlorination state
        current_system_op_state = AFTERMATH;
        clo2_peripheral_aftermath_state();
#ifdef DEBUG_CLO2_PROCESS
        serial_debug_print("OP STATE: ");
        serial_debug_println(system_op_states_str[current_system_op_state]);
#endif
        // publish data
        clo2_process_publish_data();
      }
    }
    else
    {
      if( current_system_op_state == AFTERMATH)
      {
        // stay in or go to idle state if not in a valid time space
        current_system_op_state = IDLE;
        current_system_state = false;
        clo2_peripheral_idle_state();
#ifdef DEBUG_CLO2_PROCESS
        serial_debug_print("OP STATE: ");
        serial_debug_println(system_op_states_str[current_system_op_state]);
#endif
        // publish data
        clo2_process_publish_data();
      }
      
    }

#ifdef DEBUG_CLO2_PROCESS
    static unsigned long csc = 0;
    static unsigned long psc = 0;

    csc = (millis() - system_timer) / 1000 ;
    if( csc != psc )
    {
      serial_debug_print("countdown: ");
      serial_debug_println(String(csc));
      psc = csc;
    }
        
#endif 
  }
  else
  {
    current_system_op_state = IDLE;
    clo2_peripheral_idle_state();    
  }
}

bool clo2_process_system_state_change_publisher(void)
{
  // {"STATUS":1 ,"SYSTEM_STATUS":true , "STATUS":1 }
  String _str;
  String _buffer;
  String _status;
  // start payload
  _buffer = build_payload_start(status_counter_name, status_counter);
  _str.concat(_buffer);
  // Add trigger source
  _buffer = build_payload(trigger_source_name, trigger_source_str[current_trigger_source]);
  _str.concat(_buffer);
  // Add status
  _status = (current_system_state ? true_ : false_ );
  _buffer = build_payload(system_state_name, _status);
  _str.concat(_buffer);
  // end payload
  _buffer = build_payload_end(status_counter_name, status_counter);
  _str.concat(_buffer);
  // increase counter
  status_counter++;
  // publish the data
  return mqtt_publish_then_subscribe(_str);
}

bool clo2_process_publish_data(void)
{
  // {"STATUS":1 ,"SYSTEM_STATUS":true ,"RESET":true ,"PREP_TIME":10 ,"CHLORINATION_TIME":3 , "STATUS":1 }
  String _str;
  String _buffer;
  String _status;
  // start payload
  _buffer = build_payload_start(status_counter_name, status_counter);
  _str.concat(_buffer);
  // Add status
  _status = (current_system_state ? true_ : false_ );
  _buffer = build_payload(system_state_name, _status);
  _str.concat(_buffer);
  // Add op state
  _buffer = build_payload(system_op_state_name, system_op_states_str[current_system_op_state]);
  _str.concat(_buffer);
  // add prep time
  _status = String(prep_time);
  _buffer = build_payload( update_prep_time, _status );
  _str.concat( _buffer );
  // add chlorination time
  _status = String(chlorination_time);
  _buffer = build_payload( update_prep_time, _status );
  _str.concat( _buffer );
  // end payload
  _buffer = build_payload_end(status_counter_name, status_counter);
  _str.concat(_buffer);
  // increase counter
  status_counter++;
  // publish the data
  return mqtt_publish_then_subscribe(_str);
}

bool clo2_process_reset_system(void)
{
  // reset all op system variables to default
  default_time = (ONE_SECOND * 10);
  current_system_state = false;
  prev_system_state = false;
  prep_time = default_time;
  chlorination_time = default_time;
  aftermath_time = default_time;
  current_system_op_state = IDLE;

  system_timer = 0;
  return clo2_process_publish_data();
}

void clo2_process_init(void)
{
  current_trigger_source = UNKNOWN_TRIG;
  clo2_process_reset_system();
}

void test_process_functions(void)
{
  if(Serial.available() > 0)
  {
    String _comm;
    _comm = Serial.readString();

    if(_comm.indexOf("start") >= 0)
    {
      current_system_state = true;
    }
    else if(_comm.indexOf("stop") >= 0)
    {
      current_system_state = false;
    }
  }

  clo2_process_state_change_monitor();
  clo2_process_state_monitor();

}

void clo2_process(void)
{
  // check state of process
  clo2_process_state_change_monitor();
  
  // check system state
  clo2_process_state_monitor(); 
}

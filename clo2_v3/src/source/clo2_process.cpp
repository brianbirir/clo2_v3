#include "Particle.h"
#include <include/clo2_process.h>
#include <include/ruleblox_mqtt.h>

#include <include/serial_debug.h>

#define DEBUG_CLO2_PROCESS

String system_op_states_str[4] =
{
  "IDLE",
  "PREPARATION",
  "CHLORINATION",
  "AFTERMATH"
};

enum system_op_states
{
  IDLE,
  PREPARATION,
  CHLORINATION,
  AFTERMATH
};

int current_system_op_state = IDLE;

bool current_system_state = false;
bool prev_system_state = false;

extern unsigned long ONE_SECOND; // in mS
extern unsigned long ONE_MIN;


unsigned long system_timer = 0;
unsigned long default_time = ONE_MIN;
unsigned long prep_time = default_time;
unsigned long chlorination_time = default_time;
unsigned long aftermath_time = ONE_SECOND * 15; // take 15 seconds to cool down

unsigned long status_counter = 0;

#define RUNNING_TIME (millis() - system_timer)
#define PREP_TIME (prep_time)
#define CHLORINATION_TIME (prep_time + chlorination_time)
#define AFTERMATH_TIME (prep_time + chlorination_time + aftermath_time)

String status_counter_name = "STATUS";
String system_state_name = "SYSTEM_STATE";
String system_op_state_name = "SYSTEM_OP_STATE";

extern String update_prep_time;
extern String update_chlorination_time;
extern String true_;
extern String false_;

/* local functions */
bool clo2_process_system_state_change_publisher(void);
bool clo2_process_publish_data(void);

void clo2_process_state_change_monitor(void)
{
  if ( (current_system_state == true) && (prev_system_state == false) ) {
    prev_system_state = current_system_state;
    // send notification
    clo2_process_system_state_change_publisher();
    // start system timer
    system_timer = millis();
  }

  if ( (current_system_state == false) && (prev_system_state == true) ) {
    prev_system_state = current_system_state;
    // send notification
    clo2_process_system_state_change_publisher();
    // start system timer
    system_timer = millis();
  }

#ifdef DEBUG_CLO2_PROCESS
  if(current_system_state != prev_system_state)
  {
    serial_debug_print("SYSTEM CURR STATE: ");
    serial_debug_print(String(current_system_state));
    serial_debug_print("SYSTEM PREV STATE: ");
    serial_debug_println(String(prev_system_state));
  }
#endif

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
        current_system_op_state = AFTERMATH_TIME;
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
      // stay in or go to idle state if not in a valid time space
      current_system_op_state = IDLE;
      current_system_state = false;
      // publish data
      clo2_process_publish_data();
    }
  }
  else
  {
    current_system_op_state = IDLE;
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
  current_system_state = false;
  prev_system_state = false;
  prep_time = default_time;
  chlorination_time = default_time;
  current_system_op_state = IDLE;

  return clo2_process_publish_data();
}

void clo2_process_init(void)
{
  clo2_process_reset_system();
}

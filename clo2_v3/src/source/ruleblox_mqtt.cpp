#include "Particle.h"
#include "MQTT.h"
#include <include/ruleblox_mqtt.h>
#include <include/clo2_process.h>

#include <include/serial_debug.h>

// #define DEBUG_RULEBLOX_MQTT

/* MQTT variables */
char domain[] = "www.ruleblox.com/mqtt";
byte ip_address[] = {138,197,6,61};
uint16_t port = 3881;
String topic_root = "project/clo2/device/";
String command = "/command";
String device_data = "/device_data";

volatile bool process_command_flag = false;
String command_to_process;

uint32_t command_counter = 0;
uint32_t _incoming_cc = 0;

String toggle_system_command = "TOGGLE_SYSTEM";
String reset_system_command = "RESET";
String update_prep_time = "PREP_TIME";
String update_chlorination_time = "CHLORINATION_TIME";
String processed_cc = "PCID";
String true_ = "true";
String false_ = "false";


// external variables
extern String device_id;
extern unsigned long ONE_SECOND;
extern unsigned long ONE_MIN;
extern volatile bool current_system_state;
extern volatile unsigned long prep_time;
extern volatile unsigned long chlorination_time;

extern trigger_source current_trigger_source;


/* mqtt functions */
void mqtt_callback(char* topic, byte* payload, unsigned int length);
MQTT client(ip_address, port, mqtt_callback);

bool mqtt_connect(void);
void mqtt_connect_with_retries(void);
bool mqtt_connected(void);
bool mqtt_publish(String _payload);
bool mqtt_subscribe(void);
String startup_message(void);
bool check_command_counter(String _command);
String parse_command_content(String _command, int _command_index);
bool find_string(String _find, String _parent);
String extract_command(String _raw_command);
void process_mqtt_command(void);
void mqtt_loop(void);
String get_mqtt_topic(void);

// may have to be called only once
bool mqtt_connect(void)
{
  return client.connect(device_id);
}

bool mqtt_connected(void)
{
  return client.isConnected();
}

bool mqtt_publish_then_subscribe(String _payload)
{
  // TODO: do a better job here
  bool _ret = false;
  _ret = mqtt_publish(_payload);
  mqtt_subscribe();
  return _ret;
}

bool mqtt_publish(String _payload)
{
  // publish to device data topic
  String topic = get_mqtt_topic();

  topic.concat(device_data);

#ifdef DEBUG_RULEBLOX_MQTT
  // debug
  serial_debug_print("Payload: ");
  serial_debug_print(_payload);
  serial_debug_println(" ------>");
#endif

  return client.publish(topic,_payload);
}

bool mqtt_subscribe(void)
{
  // subscribe to command topic
  String topic = get_mqtt_topic();
  topic.concat(command);
  return client.subscribe(topic);
}


String startup_message(void)
{
  String _startup_msg = device_id;
  _startup_msg.concat(" : ready");
  return _startup_msg;
}

void mqtt_callback(char* topic, byte* payload, unsigned int length)
{
  char p[length + 1];
  memcpy(p, payload, length);
  p[length] = '\0';

  command_to_process = String(p);

  // confirm that this is a new command
  if(check_command_counter(command_to_process))
  {
    process_command_flag = true;
  }
}

bool check_command_counter(String _command)
{
  String _cc = parse_command_content(_command, 0); // from the start

  _incoming_cc = _cc.toInt();

  if(_incoming_cc > command_counter) {
    return true;
  }

  return false;
}

bool mqtt_setup(void)
{
  mqtt_connect_with_retries();
  if( mqtt_connected() )
  {
    String _payload = startup_message();
    mqtt_publish(_payload);
    // Switch to listening mode after publishing
    mqtt_subscribe();
    return true;
  }
  return false;
}

String parse_command_content(String _command, int _command_index)
{
  int _collon_index = _command.indexOf(":", _command_index) + 1;
  int _comma_index = _command.indexOf(",", _command_index);
  String _command_content = _command.substring(_collon_index, _comma_index);
  return _command_content;
}

bool find_string(String _find, String _parent)
{
  if (_parent.indexOf(_find) >= 0)
  {
    return true;
  }
  return false;
}

String extract_command(String _raw_command)
{
  String _buffer;
  String _payload_resp;
  _buffer = build_payload_start(processed_cc, _incoming_cc);
  _payload_resp.concat( _buffer );

#ifdef DEBUG_RULEBLOX_MQTT
  // debug
  serial_debug_print("Command: ");
  serial_debug_print(_raw_command);
  serial_debug_println(" <------");
#endif

  // possible command structure
  // {"CC":1 ,"TOGGLE_SYSTEM":true ,"RESET":true ,"PREP_TIME":10 ,"CHLORINATION_TIME":3 , "CC":1 }
  int _command_index = -1;

  // toggle system over MQTT
  _command_index = _raw_command.indexOf(toggle_system_command);
  if ( _command_index >= 0 )
  {
    String _toggle_system = parse_command_content(_raw_command, _command_index);
    // start process here
    if( find_string(true_, _toggle_system) )
    {
      //turn on system
      current_system_state = true;
    }
    else if( find_string(false_, _toggle_system) )
    {
      // turn off system
      current_system_state = false;
    }
    // do nothing if an unknown command is issued
    current_trigger_source = TRIG_MQTT;
    _buffer = build_payload( toggle_system_command, _toggle_system );
    _payload_resp.concat( _buffer );
  }

  // reset system over MQTT
  _command_index = _raw_command.indexOf(reset_system_command);
  if ( _command_index >= 0 )
  {
    String _reset_system = parse_command_content(_raw_command, _command_index);
    // start process here
    if( find_string(true_, _reset_system) )
    {
      // reset system after confirming the message was received
      _buffer = build_payload( reset_system_command, _reset_system );
      _payload_resp.concat( _buffer );
      clo2_process_reset_system();
    }
    // do nothing if an unknown command is issued
  }

  // Update the preparation time over MQTT
  _command_index = _raw_command.indexOf(update_prep_time);
  if ( _command_index >= 0 )
  {
    String _new_prep_time = parse_command_content(_raw_command, _command_index);
    int __new_prep_time = _new_prep_time.toInt();

    if( __new_prep_time ) // only update if the new oreo time is valid
    {
      // update prep time here
      prep_time = __new_prep_time  * ONE_SECOND;
      String _time = String(__new_prep_time);
      _buffer = build_payload( update_prep_time, _time );
      _payload_resp.concat( _buffer );
    }
  }

  // Update the chlorination time over MQTT
  _command_index = _raw_command.indexOf(update_chlorination_time);
  if ( _command_index >= 0 )
  {
    String _new_chlorination_time = parse_command_content(_raw_command, _command_index);
    int __new_chlorination_time = _new_chlorination_time.toInt();

    if( __new_chlorination_time ) // only update if the new oreo time is valid
    {
      // update chlorination time here
      chlorination_time = __new_chlorination_time * ONE_SECOND;
      String _time = String(__new_chlorination_time);
      _buffer = build_payload( update_chlorination_time, _time );
      _payload_resp.concat( _buffer );
    }
  }

  // device will send back in the following format
  // {"CC":1 ,"TOGGLE_SYSTEM":true ,"RESET":true ,"PREP_TIME":10 ,"CHLORINATION_TIME":3 , "CC":1 }
  _payload_resp.concat( build_payload_end(processed_cc, _incoming_cc) );
  return _payload_resp;
}

String build_payload_start(String _key, unsigned long _count)
{
  String _str;
  _str.concat("{\"");
  _str.concat(_key);
  _str.concat("\"");
  _str.concat(":");
  _str.concat(_count);

  return _str;
}

String build_payload_end(String _key, unsigned long _count)
{
  String _str;
  _str.concat(",\"");
  _str.concat(_key);
  _str.concat("\"");
  _str.concat(":");
  _str.concat(_count);
  _str.concat(" }");

  return _str;
}

String build_payload(String _command_name , String _command_content)
{
  String _str;
  _str.concat(",\"");
  _str.concat(_command_name);
  _str.concat("\":");
  _str.concat(_command_content);

  return _str;
}

void process_mqtt_command(void)
{
  if(process_command_flag)
  {
    process_command_flag = false;
    // process the command here
    String _payload;

    _payload = extract_command(command_to_process);

    mqtt_publish_then_subscribe(_payload);

    command_counter = _incoming_cc;
  }
}

void mqtt_loop(void)
{
  if (mqtt_connected()) {
      client.loop(); 
  }
  else
  {
#ifdef DEBUG_RULEBLOX_MQTT
    // debug
    serial_debug_print("hit this point");
#endif     
    // try to 
    mqtt_connect_with_retries();
    mqtt_subscribe();
  }
}

void mqtt_connect_with_retries(void)
{
  for ( int i = 0; i < 20; i++) // take 20 seconds to reconnect?
  {
#ifdef DEBUG_RULEBLOX_MQTT
    // debug
    serial_debug_print("Connection attempt: ");
    serial_debug_print(String(i));
    serial_debug_println(" .......");
#endif
    mqtt_connect();
    if (mqtt_connected()) break;
    // delay(ONE_SECOND);
  }
}

void mqtt_process(void)
{
  mqtt_loop();
  process_mqtt_command();
}

String get_mqtt_topic(void)
{
  String _topic;
  _topic.concat(topic_root);
  _topic.concat(device_id);
  return _topic;
}

/* end of mqtt functions */

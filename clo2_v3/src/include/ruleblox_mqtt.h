#ifndef RULEBLOX_MQTT
#define RULEBLOX_MQTT

/* external functions api */
bool mqtt_publish_then_subscribe(String _payload);
bool mqtt_setup(void);
void mqtt_process(void);
String build_payload_start(String _key, unsigned long _count);
String build_payload_end(String _key, unsigned long _count);
String build_payload(String _command_name , String _command_content);

#endif

#include "Particle.h"
#include <include/ruleblox_mqtt.h>
#include <include/clo2_device.h>
#include <include/clo2_process.h>
#include <include/clo2_peripherals.h>

#include <include/serial_debug.h>

//enable threads
// SYSTEM_THREAD(ENABLED);

// global variables
unsigned long ONE_SECOND = 1000; // in mS
unsigned long ONE_MIN = ONE_SECOND * 60;

void setup() {
  // Initialize werial when necessary
  serial_debug_setup();

  // get the device ID
  get_device_id();

  // setup mqtt
  mqtt_setup();

  //init process
  clo2_process_init();

  // setup peripherals
  clo2_peripheral_setup();

}

void loop() {
  // mqtt process run in here
  mqtt_process();

  // system state machine
  clo2_process();

  // tests
  // test_process_functions();


}

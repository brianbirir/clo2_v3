#include "Particle.h"
#include <include/clo2_device.h>
#include <include/serial_debug.h>

#define DEBUG_CLO2_DEVICE

// variables
String device_id = "null";

/* device OS function API */
bool get_device_id(void)
{
  device_id = System.deviceID();

#ifdef DEBUG_CLO2_DEVICE
  serial_debug_print("Device ID: ");
  serial_debug_println(device_id);
#endif

  if(device_id == "null"){
    return false;
  }
  return true;
}

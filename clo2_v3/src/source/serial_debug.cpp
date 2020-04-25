#include "Particle.h"
#include <include/serial_debug.h>

#define DEBUG_CLO2

void serial_debug_setup(void)
{
#ifdef DEBUG_CLO2
  Serial.begin(115200);
#endif
}

void serial_debug_print(String _str)
{
#ifdef DEBUG_CLO2  
  Serial.print(_str);
#endif
}

void serial_debug_println(String _str)
{
  serial_debug_print(_str);
  serial_debug_print("\n");
}

#ifndef CLO2_PROCESS
#define CLO2_PROCESS

// global variables
enum system_op_states
{
  IDLE,
  PREPARATION,
  CHLORINATION,
  AFTERMATH
};

enum trigger_source
{
    TRIG_BUTTON,
    TRIG_MQTT,
    UNKNOWN_TRIG
};

/* external functions api */
void clo2_process_init(void);
bool clo2_process_reset_system(void);
void clo2_process(void);

// test functions
void test_process_functions(void);

#endif

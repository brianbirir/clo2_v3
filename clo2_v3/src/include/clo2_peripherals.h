#ifndef CLO2_PERIPHERALS
#define CLO2_PERIPHERALS

/* Peripheral external function API */
void clo2_peripherals_setup(void);
void clo2_peripherals_machine(void);

void clo2_peripheral_prep_state(void);
void clo2_peripheral_chlorination_state(void);
void clo2_peripheral_aftermath_state(void);
void clo2_peripheral_idle_state(void);

#endif

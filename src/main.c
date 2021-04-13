#include "system_tm4c1294.h"
#include "driverleds.h"
#include "driverbuttons.h"
#include "cmsis_os2.h"
#include "stdbool.h"
#include "inc/hw_memmap.h"
#include "driverlib/sysctl.h"
#include "driverlib/gpio.h"
#include "driverlib/interrupt.h"

void main(void)
{
    SystemInit();
    LEDInit(LED4 | LED3 | LED2 | LED1);
} // main

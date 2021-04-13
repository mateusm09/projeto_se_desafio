#include "system_tm4c1294.h"
#include "driverleds.h"
#include "driverbuttons.h"
#include "cmsis_os2.h"
#include "stdbool.h"
#include "inc/hw_memmap.h"
#include "driverlib/sysctl.h"
#include "driverlib/gpio.h"
#include "driverlib/interrupt.h"
#include "RTX_Config.h"

#define DEBOUNCE_TIME 100

#define FLAG_BUTTON_1 0x0001
#define FLAG_BUTTON_2 0x0002

#define MESSAGE_QUEUE_SIZE 16

int lastTick = 0;
osThreadId_t managerId;
osMessageQueueId_t messageQueueId;

typedef struct
{
    uint8_t led;
    int intensity;
} message;

typedef struct
{
    int ledNumber;
} threadArgument;

void uswIntHandler()
{
    uint8_t button = GPIOIntStatus(GPIO_PORTJ_BASE, true);
    ButtonIntClear(USW1 | USW2);

    int currentTick = osKernelGetTickCount();
    if (currentTick - lastTick > DEBOUNCE_TIME)
    {
        lastTick = currentTick;

        if (button & USW1)
        {
            osThreadFlagsSet(managerId, FLAG_BUTTON_1);
        }

        if (button & USW2)
        {
            osThreadFlagsSet(managerId, FLAG_BUTTON_2);
        }
    }
    return;
}

void initButtons()
{
    ButtonInit(USW1 | USW2);
    ButtonIntDisable(USW1 | USW2);
    GPIOIntRegister(GPIO_PORTJ_BASE, uswIntHandler);
    ButtonIntEnable(USW1 | USW2);
}

// dutyCycle em porcentagem de tempo ligado
void softwarePwm(uint8_t led, float dutyCycle)
{
    LEDOn(led);
    osDelay(10 * (dutyCycle / 100.f)); // precisa do .f para usar os regs float
                                       // se não ele arredonda
    LEDOff(led);
    osDelay(10 * (1.f - dutyCycle / 100.f));
}

void threadLed(void *args)
{
    threadArgument *tArgs = (threadArgument *)args;

    int intensity = 50;
    int selectedLed = 0;

    osStatus_t status;
    message msg;

    while (1)
    {
        status = osMessageQueueGet(messageQueueId, &msg, NULL, 0);

        if (status == osOK)
        {
            selectedLed = msg.led;
            selectedLed;
        }

        if (selectedLed == 1 && tArgs->ledNumber == LED1)
        {
            osDelay(1000);
        }
        if (selectedLed == 2 && tArgs->ledNumber == LED2)
        {
            osDelay(1000);
        }
        if (selectedLed == 3 && tArgs->ledNumber == LED3)
        {
            osDelay(1000);
        }
        if (selectedLed == 4 && tArgs->ledNumber == LED4)
        {
            osDelay(1000);
        }

        softwarePwm(tArgs->ledNumber, (float)intensity);
    }
}

void threadManager(void *args)
{
    int selectedLed = 0;
    int intensity[4] = {50, 50, 50, 50};

    while (1)
    {
        uint8_t flag = osThreadFlagsWait(FLAG_BUTTON_1 | FLAG_BUTTON_2, osFlagsWaitAny, osWaitForever);

        if (flag & FLAG_BUTTON_1)
        {
            selectedLed++;
            if (selectedLed > 4)
                selectedLed = 0;

            if (selectedLed >= 1 && selectedLed <= 4)
            {
                message msg = {
                    .intensity = intensity[selectedLed],
                    .led = selectedLed,
                };
                osMessageQueuePut(messageQueueId, &msg, NULL, osWaitForever);
            }
        }
        else if (flag & FLAG_BUTTON_2)
        {
            if (selectedLed >= 1 && selectedLed <= 4)
            {
                intensity[selectedLed] += 10;
                if (intensity[selectedLed] > 100)
                    intensity[selectedLed] = 0;

                message msg = {
                    .intensity = intensity[selectedLed],
                    .led = selectedLed,
                };
                osMessageQueuePut(messageQueueId, &msg, NULL, osWaitForever);
            }
        }
    }
}

void main(void)
{
    SystemInit();
    LEDInit(LED4 | LED3 | LED2 | LED1);
    initButtons();
    osKernelInitialize();

    threadArgument tArg1 = {.ledNumber = LED1};
    threadArgument tArg2 = {.ledNumber = LED2};
    threadArgument tArg3 = {.ledNumber = LED3};
    threadArgument tArg4 = {.ledNumber = LED4};

    osThreadNew(threadLed, &tArg1, NULL);
    osThreadNew(threadLed, &tArg2, NULL);
    osThreadNew(threadLed, &tArg3, NULL);
    osThreadNew(threadLed, &tArg4, NULL);

    managerId = osThreadNew(threadManager, NULL, NULL);

    messageQueueId = osMessageQueueNew(MESSAGE_QUEUE_SIZE, sizeof(message), NULL);

    if (osKernelGetState() == osKernelReady)
        osKernelStart();

    while (1)
        ;
} // main

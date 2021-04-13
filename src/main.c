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
#include "stdbool.h"

#define DEBOUNCE_TIME 200

#define FLAG_BUTTON_1 0x0001
#define FLAG_BUTTON_2 0x0002

#define MESSAGE_QUEUE_SIZE 4

int lastTick = 0;
osThreadId_t managerId;

osMessageQueueId_t messageQueueId1;
osMessageQueueId_t messageQueueId2;
osMessageQueueId_t messageQueueId3;
osMessageQueueId_t messageQueueId4;

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

    osStatus_t status;
    message msg;

    bool isSelected = false;

    int lastWaitTime = 0;
    while (1)
    {
        if (tArgs->ledNumber == LED1)
        {
            status = osMessageQueueGet(messageQueueId1, &msg, NULL, 0);
        }
        else if (tArgs->ledNumber == LED2)
        {
            status = osMessageQueueGet(messageQueueId2, &msg, NULL, 0);
        }
        else if (tArgs->ledNumber == LED3)
        {
            status = osMessageQueueGet(messageQueueId3, &msg, NULL, 0);
        }
        else if (tArgs->ledNumber == LED4)
        {
            status = osMessageQueueGet(messageQueueId4, &msg, NULL, 0);
        }

        if (status == osOK)
        {
            if (msg.led == tArgs->ledNumber)
                isSelected = true;
            else
                isSelected = false;
        }

        if (isSelected)
        {
            int currentTick = osKernelGetTickCount();

            if (currentTick - lastWaitTime < 500)
            {
                softwarePwm(tArgs->ledNumber, (float)intensity);
            }
            else if (currentTick - lastWaitTime > 1000)
            {
                lastWaitTime = currentTick;
            }
        }
        else
        {
            softwarePwm(tArgs->ledNumber, (float)intensity);
        }
    }
}

void threadManager(void *args)
{
    uint8_t selectedLed = 0x1;
    uint8_t ledIntensities[4] = {50, 50, 50, 50};

    while (1)
    {
        uint8_t flag = osThreadFlagsWait(FLAG_BUTTON_1 | FLAG_BUTTON_2, osFlagsWaitAny, osWaitForever);

        if (flag & FLAG_BUTTON_1)
        {
            selectedLed <<= 1;
            if (selectedLed > 16)
                selectedLed = 1;

            message msg = {
                .intensity = 50,
                .led = selectedLed,
            };

            osMessageQueuePut(messageQueueId1, &msg, NULL, osWaitForever);
            osMessageQueuePut(messageQueueId2, &msg, NULL, osWaitForever);
            osMessageQueuePut(messageQueueId3, &msg, NULL, osWaitForever);
            osMessageQueuePut(messageQueueId4, &msg, NULL, osWaitForever);
        }
        else if (flag & FLAG_BUTTON_2)
        {
            // ainda nada
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

    messageQueueId1 = osMessageQueueNew(MESSAGE_QUEUE_SIZE, sizeof(message), NULL);
    messageQueueId2 = osMessageQueueNew(MESSAGE_QUEUE_SIZE, sizeof(message), NULL);
    messageQueueId3 = osMessageQueueNew(MESSAGE_QUEUE_SIZE, sizeof(message), NULL);
    messageQueueId4 = osMessageQueueNew(MESSAGE_QUEUE_SIZE, sizeof(message), NULL);

    if (osKernelGetState() == osKernelReady)
        osKernelStart();

    while (1)
        ;
} // main

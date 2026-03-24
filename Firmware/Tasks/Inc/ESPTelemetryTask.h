#include "StatusLEDs.h"
#include "pinDefs.h"
#include "ESP32.h"
#include "printf.h"
#include "ADC_Sense.h"
#include <stdlib.h>
#include <string.h>


void Task_ESP_Tx(void *argument);
void Task_ESP_Rx(void *argument);


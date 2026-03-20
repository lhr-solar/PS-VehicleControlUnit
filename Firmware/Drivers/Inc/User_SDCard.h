#pragma once

#include "sdcard.h"
#include "InitTask.h"
#include "pinDefs.h"

sd_status_t SDCard_Init(void);

sd_status_t SDCard_Write(const char *filename, const char *data, TickType_t delay_ticks);

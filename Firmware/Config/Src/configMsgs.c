#include "common.h"
#include "config.h"
#include "pinConfig.h"

// fix this later

#define PRE(s)  "\r    "s"  "   // \r removes the filepath and 'note: '#pragma message:...' parts
#define STR(x)  #x
#define XSTR(x) STR(x)


#pragma message(PRE("⚡") "NUM_BATTERY_MODULES set to                " XSTR(NUM_BATTERY_MODULES))

#pragma once

// Set to 1 to build for FSM bench-testing without real precharge/contactor
// hardware wired up:
//   - contactor_get_sense() returns a fixed CLOSED instead of reading GPIO
//   - motor controller telemetry is read over CarCAN (FDCAN3) instead of
//     the dedicated MotorCAN bus (FDCAN1)
//   - the precharge task is not started
// Must be 0 for any build that drives real contactor/precharge hardware -
// none of the above are safe on a car.
#define FSM_DEBUG_BUILD 0

// #include "Precharge.h"
// #include "ADC_Sense.h"
// #include "Contactors.h"

// static uint32_t Precharge_Threshold = PRECHARGE_GOOD_THRESHOLD;

// static Precharge_State_t State = PRECHARGE_STATE_INITIAL;
// static TickType_t Start_Tick = 0;

// Precharge_Status_t PrechargeStart() // Start precharge sequence and return status
// {       
//     ADC_Sense_Result ADC_Result = {0};

//     uint32_t updated = 0;
//     if (Read_ADC(PRECHARGE_ADC_TIMEOUT_MS, &ADC_Result, &updated) != ADC_SENSE_OK) 
//     {
//         // TODO: open contactors / safe state
//     }

//     int32_t Battery_Voltage = ADC_Result.Battery_Voltage;
//     int32_t Motor_Voltage = ADC_Result.Motor_Voltage;

//     if (Motor_Voltage > Battery_Voltage)
//     {
//         // Fault handler
//     }

//     if (Battery_Voltage > OVERVOLTAGE_THRESHOLD_MV)
//     {
//         /* BATTERY ABOUT TO GO BOOM */
//         // Fault handler
//     }

//     if (Battery_Voltage < UNDERVOLTAGE_THRESHOLD_MV)
//     {
//         /* Battery voltage is too low or battery is disconnected, treat as fault */
//         // Fault handler
//     }

//     // Startup state: Closes main contactor and moves to precharging state
//     if (State == PRECHARGE_STATE_INITIAL) 
//     {
//         // TODO: close main contactor
//         ErrorStatus close_main = contactor_set(MOTOR_CONTACTOR, CLOSED, CALLBACK_BLOCKING_TIME, false);
//         if (close_main != SUCCESS)
//         {
//             // Didn't close
//         }
//         State = PRECHARGE_STATE_RUNNING;

//         // Start a timer for precharging
//         Start_Tick = xTaskGetTickCount();
//     }
    
//     if (State == PRECHARGE_STATE_RUNNING)
//     {
//         // Wait for some time
//         if ((int64_t)Motor_Voltage * RATIO_SCALE >= (int64_t)Battery_Voltage * Precharge_Threshold) 
//         {
//             // TODO: close precharge contactor
//             ErrorStatus close_precharge = contactor_set(MOTOR_PRE_CONTACTOR, CLOSED, CALLBACK_BLOCKING_TIME, false);
//             if (close_precharge != SUCCESS)
//             {
//                 // Didn't close precharge
//             }
//             State = PRECHARGE_STATE_TRANSITION;
//         }

//         const TickType_t Current_Tick = xTaskGetTickCount();
//         if ((Current_Tick - Start_Tick) > pdMS_TO_TICKS(PRECHARGE_TIMEOUT_MS)) // Faults if precharging takes too long
//         {
//             // Fault handler
//         }
//     }

//     if (State == PRECHARGE_STATE_TRANSITION)
//     {
//         Precharge_Threshold = PRECHARGE_TRANSITION_THRESHOLD;
//         if ((int64_t)Motor_Voltage * RATIO_SCALE >= (int64_t)Battery_Voltage * Precharge_Threshold) 
//         {
//             State = PRECHARGE_STATE_COMPLETE;
//         }
//         else
//         {
//             // Fault handler
//         }
//     }
    
//     return PRECHARGE_OK;
// }

// // TODO: Precharge task during car operation

// // TODO: Precharge shutdown sequence
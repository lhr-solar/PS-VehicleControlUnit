// vcu_watchdog.c
/**
 * @file vcu_watchdog.c
 * @brief Watchdog timer implementation for CAN message monitoring
 */

#include "watchdog.h"
#include "vcu_errors.h"
#include "FreeRTOS.h"
#include "task.h"

// ============================================================================
// WATCHDOG ENTRY DEFINITIONS
// ============================================================================

/**
 * Watchdog configuration for each CAN signal
 * Ordered by FSMSignal_e enum
 */
static const struct {
    uint32_t can_id;
    const char *name;
    uint16_t timeout_ms;
    bool is_critical;
} watchdog_config[FSM_SIGNAL_COUNT] = {
    [FSM_PEDALS] = {
        CAN_ID_PEDALS, 
        "Pedals", 
        WD_TIMEOUT_PEDALS,
        true
    },
    [FSM_GEARS] = {
        CAN_ID_GEARS,
        "Gears",
        WD_TIMEOUT_GEARS,
        true
    },
    [FSM_REGEN_BUTTON] = {
        CAN_ID_REGEN_BUTTON,
        "Regen Button",
        WD_TIMEOUT_REGEN_BUTTON,
        false
    },
    [FSM_REGEN_ENABLED] = {
        CAN_ID_REGEN_ENABLED,
        "Regen Enabled",
        WD_TIMEOUT_REGEN_ENABLED,
        false
    },
    [FSM_CRUISE_CONTROL] = {
        CAN_ID_CRUISE_CONTROL,
        "Cruise Control",
        WD_TIMEOUT_CRUISE_CONTROL,
        false
    },
    [FSM_BPS_OK_TO_REGEN] = {
        CAN_ID_BPS_OK_TO_REGEN,
        "BPS OK to Regen",
        WD_TIMEOUT_BPS_OK_TO_REGEN,
        true
    },
    [FSM_BPS_TRIP] = {
        CAN_ID_BPS_TRIP,
        "BPS Trip",
        WD_TIMEOUT_BPS_TRIP,
        true
    },
    [FSM_IGNITION_STATE] = {
        CAN_ID_IGNITION_STATE,
        "Ignition State",
        WD_TIMEOUT_IGNITION,
        true
    },
};

// Global watchdog manager instance
static WatchdogManager_t *g_watchdog_manager = NULL;

// ============================================================================
// INITIALIZATION
// ============================================================================

bool vcu_watchdog_init(WatchdogManager_t *mgr) {
    if (!mgr) return false;
    
    g_watchdog_manager = mgr;
    mgr->count = FSM_SIGNAL_COUNT;
    mgr->check_cycles = 0;
    
    TickType_t current_tick = xTaskGetTickCount();
    
    for (int i = 0; i < FSM_SIGNAL_COUNT; i++) {
        WatchdogEntry_t *entry = &mgr->entries[i];
        
        entry->can_id = watchdog_config[i].can_id;
        entry->name = watchdog_config[i].name;
        entry->timeout_ms = watchdog_config[i].timeout_ms;
        entry->last_received_tick = current_tick;  // Initialize to now
        entry->is_faulted = false;
        entry->miss_count = 0;
    }
    
    return true;
}

// ============================================================================
// WATCHDOG OPERATIONS
// ============================================================================

/**
 * @brief Mark a CAN message as received
 * Called from CAN RX task when message arrives
 * 
 * Safety: Timing-safe (no blocking). Can be called from ISR.
 */
void vcu_watchdog_mark_received(WatchdogManager_t *mgr, FSMSignal_e signal) {
    if (!mgr || signal >= FSM_SIGNAL_COUNT) return;
    
    WatchdogEntry_t *entry = &mgr->entries[signal];
    
    // Update reception timestamp
    entry->last_received_tick = xTaskGetTickCount();
    
    // Clear fault if it was previously set (message recovered)
    if (entry->is_faulted) {
        entry->is_faulted = false;
        entry->miss_count = 0;
        
        // Optional: Log recovery event
        // printf("[WD] %s recovered\n", entry->name);
    }
}

bool vcu_watchdog_is_faulted(WatchdogManager_t *mgr, FSMSignal_e signal) {
    if (!mgr || signal >= FSM_SIGNAL_COUNT) return false;
    return mgr->entries[signal].is_faulted;
}

uint32_t vcu_watchdog_get_faults_bitmask(WatchdogManager_t *mgr) {
    if (!mgr) return 0;
    
    uint32_t faults = 0;
    for (int i = 0; i < FSM_SIGNAL_COUNT; i++) {
        if (mgr->entries[i].is_faulted) {
            faults |= (1 << i);
        }
    }
    return faults;
}

void vcu_watchdog_clear_fault(WatchdogManager_t *mgr, FSMSignal_e signal) {
    if (!mgr || signal >= FSM_SIGNAL_COUNT) return;
    mgr->entries[signal].is_faulted = false;
    mgr->entries[signal].miss_count = 0;
}

void vcu_watchdog_clear_all_faults(WatchdogManager_t *mgr) {
    if (!mgr) return;
    for (int i = 0; i < FSM_SIGNAL_COUNT; i++) {
        mgr->entries[i].is_faulted = false;
        mgr->entries[i].miss_count = 0;
    }
}

uint32_t vcu_watchdog_get_miss_count(WatchdogManager_t *mgr, FSMSignal_e signal) {
    if (!mgr || signal >= FSM_SIGNAL_COUNT) return 0;
    return mgr->entries[signal].miss_count;
}

// ============================================================================
// WATCHDOG MONITOR TASK
// ============================================================================

/**
 * @brief Main watchdog monitoring task
 * 
 * Runs periodically (every 10ms) and checks each watched message:
 * - If time since last reception > timeout → fault triggered
 * - Sends fault notification to error task
 * 
 * Design rationale:
 * - Single task instead of N timers = lower overhead
 * - Timestamp-based instead of window-based = more flexible
 * - Direct error task notification = immediate fault response
 * 
 * Priority: 1 (high, but below error task @ priority 0)
 */
void vcu_watchdog_monitor_task(void *pvParameters) {
    WatchdogManager_t *mgr = (WatchdogManager_t *)pvParameters;
    ErrorManager_t *em = get_error_manager();
    
    if (!mgr || !em) {
        // Can't run without manager
        vTaskDelete(NULL);
        return;
    }
    
    TickType_t current_tick;
    uint32_t elapsed_ms;
    
    while (1) {
        // Check every 10ms for responsive fault detection
        vTaskDelay(pdMS_TO_TICKS(10));
        
        current_tick = xTaskGetTickCount();
        mgr->check_cycles++;
        
        // Check each watched message
        for (int i = 0; i < mgr->count; i++) {
            WatchdogEntry_t *entry = &mgr->entries[i];
            
            // Calculate time since last reception
            uint32_t ticks_elapsed = current_tick - entry->last_received_tick;
            elapsed_ms = ticks_elapsed * portTICK_PERIOD_MS;
            
            // Check if timeout exceeded
            if (elapsed_ms > entry->timeout_ms) {
                
                // Message timeout detected
                if (!entry->is_faulted) {
                    entry->is_faulted = true;
                    entry->miss_count++;
                    
                    // Log the fault
                    printf("[WD] TIMEOUT: %s (elapsed %lu ms > %u ms)\n",
                           entry->name, elapsed_ms, entry->timeout_ms);
                    
                    // Map signal index to fault bit
                    FaultBits_e fault_bit = (FaultBits_e)(1 << i);
                    
                    // Trigger error task immediately
                    vcu_fault_trigger(em, fault_bit);
                }
            }
        }
    }
}

/**
 * 
 */
WatchdogManager_t* get_watchdog_manager(void) {
    return g_watchdog_manager;
}

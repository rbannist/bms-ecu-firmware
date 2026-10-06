/**
 * @file bms_hw_regs.h
 * @brief Memory-mapped hardware register definitions for the BMS Contactor ECU.
 *
 * In target ARM Cortex-M4 builds, BMS_HW_REGS_BASE points to peripheral address
 * 0x40021000U. In Software-in-the-Loop (SIL) host builds, a caller-allocated
 * BMS_HwRegs_t instance is passed directly to the monitor functions.
 */

#ifndef BMS_HW_REGS_H
#define BMS_HW_REGS_H

#include <stdint.h>

/** @brief Simulated peripheral base address on Cortex-M4 MCU. */
#define BMS_HW_REGS_BASE_ADDR       (0x40021000UL)

/** @brief Contactor control register bitmasks (Offset 0x04). */
#define BMS_REG_CONTACTOR_OPEN      (0x00000000UL)
#define BMS_REG_CONTACTOR_CLOSED_BIT (0x00000001UL)
#define BMS_REG_CONTACTOR_TRIP_BIT  (0x00000002UL)

/** @brief Fault status register bitmasks (Offset 0x08). */
#define BMS_FAULT_NONE              (0x00000000UL)
#define BMS_FAULT_OVERCURRENT_BIT   (0x00000001UL)
#define BMS_FAULT_SENSOR_ERR_BIT    (0x00000002UL)

/** @brief Sentinel value indicating an ADC/CAN current sensor open-circuit fault. */
#define BMS_SENSOR_INVALID_RAW      (0xFFFFFFFFUL)

/**
 * @brief Memory-mapped register block for the BMS High-Voltage Contactor peripheral.
 */
typedef struct {
    volatile uint32_t pack_current_ma; /**< 0x00: Rectified absolute pack current in mA (|I_pack|) */
    volatile uint32_t contactor_ctrl;  /**< 0x04: Contactor coil driver control bits */
    volatile uint32_t fault_status;    /**< 0x08: Diagnostic fault status flags */
} BMS_HwRegs_t;

#endif /* BMS_HW_REGS_H */

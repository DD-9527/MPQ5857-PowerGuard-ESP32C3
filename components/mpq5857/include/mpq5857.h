#ifndef MPQ5857_H
#define MPQ5857_H

#include <stdint.h>
#include "esp_err.h"

#define MPQ5857_DEV_ADDR        0x30
#define MPQ5857_I2C_FREQ_HZ     100000
#define MPQ5857_TIMEOUT_MS      100

// ========== 寄存器地址定义 ==========
typedef enum {
    REG_USER_00    = 0x00,  // 电流/欠压保护配置
    REG_USER_01    = 0x01,  // 电压检测/过压保护配置
    REG_USER_02    = 0x02,  // 故障时间/输出控制
    REG_USER_03    = 0x03,  // 软启动/I2C地址
    REG_USER_04    = 0x04,  // 工作模式控制
    REG_DEVICE_ID  = 0x05,  // 设备ID
    REG_DEV_STAT_1 = 0x06,  // 故障状态1
    REG_DEV_STAT_2 = 0x07   // 故障状态2（温度）
} MPQ5857_Register_t;

// ========== 枚举类型定义 ==========

// 电流限值等级
typedef enum {
    CURRENT_LIMIT_50MV  = 0x00,
    CURRENT_LIMIT_100MV = 0x01,
    CURRENT_LIMIT_150MV = 0x02,
    CURRENT_LIMIT_200MV = 0x03,
    CURRENT_LIMIT_250MV = 0x04
} CurrentLimitLevel_t;

// 电压检测比例
typedef enum {
    VOLTAGE_RATIO_1_30 = 0x01,
    VOLTAGE_RATIO_1_24 = 0x02,  // 默认
    VOLTAGE_RATIO_1_15 = 0x03
} VoltageSenseRatio_t;

// 保护模式
typedef enum {
    PROTECTION_MODE_IMMEDIATE_OFF   = 0x00,
    PROTECTION_MODE_DEGLITCH_OFF    = 0x01,
    PROTECTION_MODE_REGISTER_ONLY   = 0x02,
    PROTECTION_MODE_NO_PROTECTION   = 0x03,
    PROTECTION_MODE_CLAMP_OFF       = 0x01,
    PROTECTION_MODE_CLAMP_INFINITE  = 0x02
} ProtectionMode_t;

// 软启动斜率
typedef enum {
    SOFT_START_0_25V_PER_MS  = 0x00, // 0.25V/ms (默认)
    SOFT_START_0_5V_PER_MS   = 0x01,
    SOFT_START_1V_PER_MS     = 0x02,
    SOFT_START_4V_PER_MS     = 0x03,
    SOFT_START_16V_PER_MS    = 0x04,
    SOFT_START_32V_PER_MS    = 0x05,
    SOFT_START_64V_PER_MS    = 0x06,
    SOFT_START_DISABLED      = 0x07
} SoftStartSlewRate_t;

// 故障时间
typedef enum {
    FAULT_TIME_IMMEDIATE = 0x00,  // 0ms
    FAULT_TIME_0_2MS     = 0x01,  // 0.2ms
    FAULT_TIME_0_4MS     = 0x02,  // 0.4ms
    FAULT_TIME_1MS       = 0x03,  // 1ms
    FAULT_TIME_2MS       = 0x04,  // 2ms (默认)
    FAULT_TIME_8MS       = 0x05,  // 8ms
    FAULT_TIME_32MS      = 0x06,  // 32ms
    FAULT_TIME_128MS     = 0x07   // 128ms
} FaultDebounceTime_t;

// I2C地址
typedef enum {
    I2C_ADDR_0x30 = 0x00,         // (默认)
    I2C_ADDR_0x31 = 0x01,
    I2C_ADDR_0x32 = 0x02,
    I2C_ADDR_0x33 = 0x03,
    I2C_ADDR_0x34 = 0x04,
    I2C_ADDR_0x35 = 0x05,
    I2C_ADDR_0x36 = 0x06,
    I2C_ADDR_0x37 = 0x07
} I2C_Address_t;












// ========== Register Bit-field Definitions ==========

/**
 * @brief USER_00 Register (Over-current/Under-voltage protection config)
 */
typedef union {
    struct {
        uint8_t uvpm   : 2;  /**< [1:0] Under-voltage protection mode */
        uint8_t uvplvl : 3;  /**< [4:2] Under-voltage protection threshold */
        uint8_t ocpm   : 1;  /**< [5] Over-current protection mode */
        uint8_t ocplvl : 2;  /**< [7:6] Current limit level */
    } bits;
    uint8_t value;
} USER_00_Register_t;

/**
 * @brief USER_01 Register (Voltage detection/Over-voltage protection config)
 */
typedef union {
    struct {
        uint8_t vss    : 2;  /**< [1:0] Voltage sense ratio */
        uint8_t ovpm   : 1;  /**< [2] Over-voltage protection mode */
        uint8_t ovplvl : 5;  /**< [7:3] Over-voltage protection threshold */
    } bits;
    uint8_t value;
} USER_01_Register_t;

/**
 * @brief USER_02 Register (Fault timing/Output control)
 */
typedef union {
    struct {
        uint8_t isns   : 1;  /**< [0] Current sense output enable */
        uint8_t odc    : 1;  /**< [1] Output discharge enable */
        uint8_t fpm    : 1;  /**< [2] Fault pin mode */
        uint8_t fom    : 2;  /**< [4:3] Fault off-time multiplier */
        uint8_t flttmr : 3;  /**< [7:5] Fault debounce time */
    } bits;
    uint8_t value;
} USER_02_Register_t;

/**
 * @brief USER_03 Register (Soft-start/I2C address)
 */
typedef union {
    struct {
        uint8_t addr     : 3;  /**< [2:0] I2C address LSB */
        uint8_t sssr     : 3;  /**< [5:3] Soft-start slew rate */
        uint8_t reserved : 2;  /**< [7:6] Reserved */
    } bits;
    uint8_t value;
} USER_03_Register_t;

/**
 * @brief USER_04 Register (Operating mode control)
 */
typedef union {
    struct {
        uint8_t ilcm     : 1;  /**< [0] Low current mode */
        uint8_t en2      : 1;  /**< [1] Load switch force enable */
        uint8_t vmon_en1 : 1;  /**< [2] Voltage monitor enable at EN1 */
        uint8_t iham     : 1;  /**< [3] Current sense mode */
        uint8_t oc_latch : 1;  /**< [4] Over-current protection latch */
        uint8_t ovclp_inf: 1;  /**< [5] Over-voltage infinite clamp */
        uint8_t ov_latch : 1;  /**< [6] Over-voltage protection latch */
        uint8_t reserved : 1;  /**< [7] Reserved */
    } bits;
    uint8_t value;
} USER_04_Register_t;

/**
 * @brief Device ID Register
 */
typedef union {
    struct {
        uint8_t CID : 4;  /**< Configuration ID */
        uint8_t RID : 4;  /**< Revision ID */
    } bits;
    uint8_t value;
} DEV_ID_Register_t;

/**
 * @brief Device Status Register 1
 */
typedef union {
    struct {
        uint8_t sss  : 1;  /**< [0] Soft-start status */
        uint8_t afs  : 1;  /**< [1] All fault status OR */
        uint8_t fos  : 1;  /**< [2] Fault off status */
        uint8_t uvps : 1;  /**< [3] Under-voltage status */
        uint8_t ovps : 1;  /**< [4] Over-voltage status */
        uint8_t ilims: 1;  /**< [5] Over-current status */
        uint8_t rbs  : 1;  /**< [6] Reverse battery status */
        uint8_t g1_f : 1;  /**< [7] G1 fast pull-up timeout */
    } bits;
    uint8_t value;
} DEV_STAT_1_Register_t;

/**
 * @brief Device Status Register 2
 */
typedef union {
    struct {
        uint8_t tw       : 1;  /**< [0] Temperature warning */
        uint8_t tsd      : 1;  /**< [1] Thermal shutdown */
        uint8_t reserved : 6;  /**< [7:2] Reserved */
    } bits;
    uint8_t value;
} DEV_STAT_2_Register_t;

/**
 * @brief MPQ5857 Register Map Structure
 *
 * Packed structure to ensure tight memory alignment for I2C transfers.
 * Note: reg06 and reg07 are both DEV_STAT_2_Register_t as per datasheet.
 */
#pragma pack(push, 1)
typedef struct {
    USER_00_Register_t reg00;  /**< Address 0x00 */
    USER_01_Register_t reg01;  /**< Address 0x01 */
    USER_02_Register_t reg02;  /**< Address 0x02 */
    USER_03_Register_t reg03;  /**< Address 0x03 */
    USER_04_Register_t reg04;  /**< Address 0x04 */
    DEV_ID_Register_t  reg05;  /**< Address 0x05 (read-only) */
    DEV_STAT_1_Register_t reg06; /**< Address 0x06 */
    DEV_STAT_2_Register_t reg07; /**< Address 0x07 */
} MPQ5857_Registers_t;
#pragma pack(pop)

typedef union {
    MPQ5857_Registers_t value;
    uint8_t bytes[8];
} MPQ5857_Registers_u_t;










/**
 * @brief 初始化MPQ5857 I2C设备
 */
void mpq5857_init(void);

esp_err_t mpq5857_set_OCPLVL(uint8_t value);
uint8_t mpq5857_get_OCPLVL();
esp_err_t mpq5857_set_OCPM(uint8_t value);
esp_err_t mpq5857_set_OVPLVL(uint8_t value);
esp_err_t mpq5857_set_OVPM(uint8_t value);
esp_err_t mpq5857_set_UVPLVL(uint8_t value);
esp_err_t mpq5857_set_UVPM(uint8_t value);
esp_err_t mpq5857_set_VSS(uint8_t value);
esp_err_t mpq5857_set_FLTTMR(uint8_t value);
uint8_t mpq5857_get_FLTTMR();
esp_err_t mpq5857_set_FOM(uint8_t value);
esp_err_t mpq5857_set_FPM(uint8_t value);
esp_err_t mpq5857_set_OCD(uint8_t value);
esp_err_t mpq5857_set_ISNS(uint8_t value);
esp_err_t mpq5857_set_SSSR(uint8_t value);
uint8_t mpq5857_get_SSSR();
esp_err_t mpq5857_set_ADDR(uint8_t value);
/**
 * @brief 写寄存器
 */
esp_err_t mpq5857_write_reg(uint8_t reg, uint8_t data);

/**
 * @brief 读多个寄存器
 */
esp_err_t mpq5857_read_regs(uint8_t reg, uint8_t *data, size_t len);
MPQ5857_Registers_u_t mpq5857_flush(void);
/**
 * @brief 开启/关闭 负载输出
 */
esp_err_t mpq5857_set_output(uint32_t state);

/**
 * @brief 演示函数：读取寄存器 0x00~0x07
 */
MPQ5857_Registers_u_t mpq5857_demo(void);

#endif // MPQ5857_H
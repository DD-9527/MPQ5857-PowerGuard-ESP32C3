#include <stdint.h>
#include <string.h>

#include "mpq5857.h"
#include "driver/i2c_master.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "esp_err.h"

static const char *TAG = "MPQ5857";

static i2c_master_dev_handle_t dev_handle = NULL;
static i2c_master_bus_handle_t bus_handle = NULL;
static MPQ5857_Registers_u_t mpq5857_regs = {0};

/**
 * @brief Convert current limit level to string representation
 */
static const char* current_limit_level_to_string(uint8_t value)
{
    switch (value & 0x03) {
        case CURRENT_LIMIT_50MV:   return "50mV";
        case CURRENT_LIMIT_100MV:  return "100mV";
        case CURRENT_LIMIT_150MV:  return "150mV";
        case CURRENT_LIMIT_200MV:  return "200mV";
        default: return "UNKNOWN";
    }
}
esp_err_t mpq5857_set_OCPLVL(uint8_t value){
    mpq5857_regs.value.reg00.bits.ocplvl = value & 0x03;
    return mpq5857_write_reg(0x00, mpq5857_regs.value.reg00.value);
}
uint8_t mpq5857_get_OCPLVL(){
    return mpq5857_regs.value.reg00.bits.ocplvl;
}
esp_err_t mpq5857_set_OCPM(uint8_t value){
    mpq5857_regs.value.reg00.bits.ocpm = value & 0x01;
    return mpq5857_write_reg(0x00, mpq5857_regs.value.reg00.value);
}
esp_err_t mpq5857_set_UVPLVL(uint8_t value){
    mpq5857_regs.value.reg00.bits.uvplvl = value & 0x07;
    return mpq5857_write_reg(0x00, mpq5857_regs.value.reg00.value);
}
esp_err_t mpq5857_set_UVPM(uint8_t value){
    mpq5857_regs.value.reg00.bits.uvpm = value & 0x03;
    return mpq5857_write_reg(0x00, mpq5857_regs.value.reg00.value);
}
/**
 * @brief Convert voltage ratio to string representation
 */
static const char* voltage_ratio_to_string(uint8_t value)
{
    switch (value & 0x03) {
        case VOLTAGE_RATIO_1_30: return "1:30";
        case VOLTAGE_RATIO_1_24: return "1:24";
        case VOLTAGE_RATIO_1_15: return "1:15";
        default: return "UNKNOWN";
    }
}

/**
 * @brief Convert protection mode to string representation
 */
static const char* protection_mode_to_string(uint8_t value)
{
    switch (value & 0x03) {
        case PROTECTION_MODE_IMMEDIATE_OFF:  return "IMMEDIATE_OFF";
        case PROTECTION_MODE_DEGLITCH_OFF:   return "DEGLITCH_OFF";
        case PROTECTION_MODE_REGISTER_ONLY:  return "REGISTER_ONLY";
        case PROTECTION_MODE_NO_PROTECTION:  return "NO_PROTECTION";
        default: return "UNKNOWN";
    }
}
esp_err_t mpq5857_set_OVPLVL(uint8_t value){
    mpq5857_regs.value.reg01.bits.ovplvl = value & 0x1F;
    return mpq5857_write_reg(0x01, mpq5857_regs.value.reg01.value);
}
esp_err_t mpq5857_set_OVPM(uint8_t value){
    mpq5857_regs.value.reg01.bits.ovpm = value & 0x03;
    return mpq5857_write_reg(0x01, mpq5857_regs.value.reg01.value);
}
esp_err_t mpq5857_set_VSS(uint8_t value){
    mpq5857_regs.value.reg01.bits.vss = value & 0x01;
    return mpq5857_write_reg(0x01, mpq5857_regs.value.reg01.value);
}

esp_err_t mpq5857_set_FLTTMR(uint8_t value){
    mpq5857_regs.value.reg02.bits.flttmr = value & 0x07;
    return mpq5857_write_reg(0x02, mpq5857_regs.value.reg02.value);
}

uint8_t mpq5857_get_FLTTMR(){
    return mpq5857_regs.value.reg02.bits.flttmr;
}

esp_err_t mpq5857_set_FOM(uint8_t value){
    mpq5857_regs.value.reg02.bits.fom = value & 0x03;
    return mpq5857_write_reg(0x02, mpq5857_regs.value.reg02.value);
}
esp_err_t mpq5857_set_FPM(uint8_t value){
    mpq5857_regs.value.reg02.bits.fpm = value & 0x01;
    return mpq5857_write_reg(0x02, mpq5857_regs.value.reg02.value);
}
esp_err_t mpq5857_set_OCD(uint8_t value){
    mpq5857_regs.value.reg02.bits.odc = value & 0x01;
    return mpq5857_write_reg(0x02, mpq5857_regs.value.reg02.value);
}
esp_err_t mpq5857_set_ISNS(uint8_t value){
    mpq5857_regs.value.reg02.bits.isns = value & 0x01;
    return mpq5857_write_reg(0x02, mpq5857_regs.value.reg02.value);
}

esp_err_t mpq5857_set_SSSR(uint8_t value){
    mpq5857_regs.value.reg03.bits.sssr = value & 0x07;
    return mpq5857_write_reg(0x03, mpq5857_regs.value.reg03.value);
}

uint8_t mpq5857_get_SSSR(){
    return mpq5857_regs.value.reg03.bits.sssr;
}

esp_err_t mpq5857_set_ADDR(uint8_t value){
    mpq5857_regs.value.reg03.bits.addr = value & 0x07;
    return mpq5857_write_reg(0x03, mpq5857_regs.value.reg03.value);
}
/**
 * @brief Convert soft start slew rate to string representation
 */
static const char* soft_start_slew_rate_to_string(uint8_t value)
{
    switch (value & 0x07) {
        case SOFT_START_0_25V_PER_MS:  return "0.25V/ms";
        case SOFT_START_0_5V_PER_MS:   return "0.5V/ms";
        case SOFT_START_1V_PER_MS:     return "1V/ms";
        case SOFT_START_4V_PER_MS:     return "4V/ms";
        case SOFT_START_16V_PER_MS:    return "16V/ms";
        case SOFT_START_32V_PER_MS:    return "32V/ms";
        case SOFT_START_64V_PER_MS:    return "64V/ms";
        case SOFT_START_DISABLED:      return "DISABLED";
        default: return "UNKNOWN";
    }
}

/**
 * @brief Convert fault debounce time to string representation
 */
static const char* fault_debounce_time_to_string(uint8_t value)
{
    switch (value & 0x07) {
        case FAULT_TIME_IMMEDIATE: return "0ms";
        case FAULT_TIME_0_2MS:     return "0.2ms";
        case FAULT_TIME_0_4MS:     return "0.4ms";
        case FAULT_TIME_1MS:       return "1ms";
        case FAULT_TIME_2MS:       return "2ms";
        case FAULT_TIME_8MS:       return "8ms";
        case FAULT_TIME_32MS:      return "32ms";
        case FAULT_TIME_128MS:     return "128ms";
        default: return "UNKNOWN";
    }
}

/**
 * @brief Convert I2C address to string representation
 */
static const char* i2c_address_to_string(uint8_t value)
{
    switch (value & 0x07) {
        case I2C_ADDR_0x30: return "0x30";
        case I2C_ADDR_0x31: return "0x31";
        case I2C_ADDR_0x32: return "0x32";
        case I2C_ADDR_0x33: return "0x33";
        case I2C_ADDR_0x34: return "0x34";
        case I2C_ADDR_0x35: return "0x35";
        case I2C_ADDR_0x36: return "0x36";
        case I2C_ADDR_0x37: return "0x37";
        default: return "UNKNOWN";
    }
}

/**
 * @brief Convert register address to string representation
 */
static const char* register_address_to_string(uint8_t addr)
{
    switch (addr) {
        case REG_USER_00:    return "USER_00";
        case REG_USER_01:    return "USER_01";
        case REG_USER_02:    return "USER_02";
        case REG_USER_03:    return "USER_03";
        case REG_USER_04:    return "USER_04";
        case REG_DEVICE_ID:  return "DEVICE_ID";
        case REG_DEV_STAT_1: return "DEV_STAT_1";
        case REG_DEV_STAT_2: return "DEV_STAT_2";
        default: return "UNKNOWN";
    }
}

/**
 * @brief Parse USER_00 register (Over-current/Under-voltage protection config)
 */
static void parse_user_00_register(uint8_t value)
{
    USER_00_Register_t reg;
    reg.value = value;
    
    ESP_LOGI(TAG, "USER_00 Register (0x%02X):", value);
    ESP_LOGI(TAG, "  UVPM (Under-voltage protection mode): %s",
             protection_mode_to_string(reg.bits.uvpm));
    ESP_LOGI(TAG, "  UVPLVL (Under-voltage threshold): %d", reg.bits.uvplvl);
    ESP_LOGI(TAG, "  OCPM (Over-current protection mode): %s",
             reg.bits.ocpm ? "DEGLITCH_OFF" : "IMMEDIATE_OFF");
    ESP_LOGI(TAG, "  OCPLVL (Current limit level): %s",
             current_limit_level_to_string(reg.bits.ocplvl));
}

/**
 * @brief Parse USER_01 register (Voltage detection/Over-voltage protection config)
 */
static void parse_user_01_register(uint8_t value)
{
    USER_01_Register_t reg;
    reg.value = value;
    
    ESP_LOGI(TAG, "USER_01 Register (0x%02X):", value);
    ESP_LOGI(TAG, "  VSS (Voltage sense ratio): %s",
             voltage_ratio_to_string(reg.bits.vss));
    ESP_LOGI(TAG, "  OVPM (Over-voltage protection mode): %s",
             reg.bits.ovpm ? "DEGLITCH_OFF" : "IMMEDIATE_OFF");
    ESP_LOGI(TAG, "  OVPLVL (Over-voltage threshold): %d", reg.bits.ovplvl);
}

/**
 * @brief Parse USER_02 register (Fault timing/Output control)
 */
static void parse_user_02_register(uint8_t value)
{
    USER_02_Register_t reg;
    reg.value = value;
    
    ESP_LOGI(TAG, "USER_02 Register (0x%02X):", value);
    ESP_LOGI(TAG, "  ISNS (Current sense output enable): %s",
             reg.bits.isns ? "ENABLED" : "DISABLED");
    ESP_LOGI(TAG, "  ODC (Output discharge enable): %s",
             reg.bits.odc ? "ENABLED" : "DISABLED");
    ESP_LOGI(TAG, "  FPM (Fault pin mode): %s",
             reg.bits.fpm ? "REGISTER_ONLY" : "IMMEDIATE_OFF");
    ESP_LOGI(TAG, "  FOM (Fault off-time multiplier): %d", reg.bits.fom);
    ESP_LOGI(TAG, "  FLTTMR (Fault debounce time): %s",
             fault_debounce_time_to_string(reg.bits.flttmr));
}

/**
 * @brief Parse USER_03 register (Soft-start/I2C address)
 */
static void parse_user_03_register(uint8_t value)
{
    USER_03_Register_t reg;
    reg.value = value;
    
    ESP_LOGI(TAG, "USER_03 Register (0x%02X):", value);
    ESP_LOGI(TAG, "  ADDR (I2C address LSB): %s",
             i2c_address_to_string(reg.bits.addr));
    ESP_LOGI(TAG, "  SSSR (Soft-start slew rate): %s",
             soft_start_slew_rate_to_string(reg.bits.sssr));
    ESP_LOGI(TAG, "  RESERVED: 0x%02X", reg.bits.reserved);
}

/**
 * @brief Parse USER_04 register (Operating mode control)
 */
static void parse_user_04_register(uint8_t value)
{
    USER_04_Register_t reg;
    reg.value = value;
    
    ESP_LOGI(TAG, "USER_04 Register (0x%02X):", value);
    ESP_LOGI(TAG, "  ILCM (Low current mode): %s",
             reg.bits.ilcm ? "ENABLED" : "DISABLED");
    ESP_LOGI(TAG, "  EN2 (Load switch force enable): %s",
             reg.bits.en2 ? "ENABLED" : "DISABLED");
    ESP_LOGI(TAG, "  VMON_EN1 (Voltage monitor enable at EN1): %s",
             reg.bits.vmon_en1 ? "ENABLED" : "DISABLED");
    ESP_LOGI(TAG, "  IHAM (Current sense mode): %s",
             reg.bits.iham ? "HIGH_ACCURACY" : "STANDARD");
    ESP_LOGI(TAG, "  OC_LATCH (Over-current protection latch): %s",
             reg.bits.oc_latch ? "LATCHED" : "AUTO_RETRY");
    ESP_LOGI(TAG, "  OVCLP_INF (Over-voltage infinite clamp): %s",
             reg.bits.ovclp_inf ? "INFINITE_CLAMP" : "LIMITED_CLAMP");
    ESP_LOGI(TAG, "  OV_LATCH (Over-voltage protection latch): %s",
             reg.bits.ov_latch ? "LATCHED" : "AUTO_RETRY");
    ESP_LOGI(TAG, "  RESERVED: 0x%02X", reg.bits.reserved);
}

/* ========================== Public Functions ========================== */

void mpq5857_init(void)
{
    /* Configure I2C master bus */
    i2c_master_bus_config_t i2c_mst_config = {
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .i2c_port = -1,
        .scl_io_num = CONFIG_GPIO_I2C_SCL,
        .sda_io_num = CONFIG_GPIO_I2C_SDA,
        .glitch_ignore_cnt = 7,
        .flags.enable_internal_pullup = true,
    };
    
    esp_err_t ret = i2c_new_master_bus(&i2c_mst_config, &bus_handle);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to create I2C master bus: %s", esp_err_to_name(ret));
        return;
    }

    /* Configure I2C device */
    i2c_device_config_t dev_cfg = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = MPQ5857_DEV_ADDR,
        .scl_speed_hz = MPQ5857_I2C_FREQ_HZ,
    };
    
    ret = i2c_master_bus_add_device(bus_handle, &dev_cfg, &dev_handle);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to add I2C device: %s", esp_err_to_name(ret));
        return;
    }
    
    ESP_LOGI(TAG, "MPQ5857 device 0x%02X initialized", MPQ5857_DEV_ADDR);
}

esp_err_t mpq5857_write_reg(uint8_t reg, uint8_t data)
{
    if (!dev_handle) {
        ESP_LOGE(TAG, "Device not initialized");
        return ESP_ERR_INVALID_STATE;
    }

    ESP_LOGI(TAG, "%x %x", reg, data);
    
    uint8_t buf[2] = {reg, data};
    return i2c_master_transmit(dev_handle, buf, sizeof(buf), MPQ5857_TIMEOUT_MS);
}

esp_err_t mpq5857_read_regs(uint8_t reg, uint8_t *data, size_t len)
{
    if (!dev_handle || !data) {
        ESP_LOGE(TAG, "Invalid parameters");
        return ESP_ERR_INVALID_ARG;
    }
    
    return i2c_master_transmit_receive(dev_handle, &reg, 1, data, len, MPQ5857_TIMEOUT_MS);
}

MPQ5857_Registers_u_t mpq5857_flush(void)
{
    mpq5857_read_regs(0, mpq5857_regs.bytes, sizeof(mpq5857_regs.bytes));
    return mpq5857_regs;
}

esp_err_t mpq5857_set_output(uint32_t state)
{
    return gpio_set_level(CONFIG_GPIO_EN2, state);
}

MPQ5857_Registers_u_t mpq5857_demo(void)
{
    /* Read all registers (0x00-0x07) */
    esp_err_t ret = mpq5857_read_regs(0, mpq5857_regs.bytes, sizeof(mpq5857_regs.bytes));
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to read registers: %s", esp_err_to_name(ret));
        return mpq5857_regs; // TO BE FIX
    }

    /* Log register values */
    ESP_LOGI(TAG, "MPQ5857 Register Dump:");
    for (uint8_t addr = 0; addr < 8; addr++) {
        ESP_LOGI(TAG, "  %s (0x%02X) = 0x%02X",
                 register_address_to_string(addr), addr, mpq5857_regs.bytes[addr]);
    }

    /* Parse and log register contents */
    parse_user_00_register(mpq5857_regs.value.reg00.value);
    parse_user_01_register(mpq5857_regs.value.reg01.value);
    parse_user_02_register(mpq5857_regs.value.reg02.value);
    parse_user_03_register(mpq5857_regs.value.reg03.value);
    parse_user_04_register(mpq5857_regs.value.reg04.value);

    return mpq5857_regs;
}
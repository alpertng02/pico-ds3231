#include "ds3231.h"

/**
 * @brief               Library function to read a specific I2C register adress.
 * 
 * @param[in] i2c       I2C instance used.
 * @param[in] dev_addr  Adress of the I2C device.
 * @param[in] reg_addr  Register adress to be read.
 * @param[in] length    length of the data in bytes to be read.
 * @param[out] data     Buffer to store the read data.
 * @return              0 if succesful, -1 if i2c failure.    
 */
int i2c_read_reg(i2c_inst_t * i2c, uint8_t dev_addr, 
    uint8_t reg_addr, size_t length, uint8_t * data) 
{
    if(!length) 
        return 0;
    uint8_t reg = reg_addr; 
    if(i2c_write_blocking(i2c, dev_addr, &reg, 1, true) == PICO_ERROR_GENERIC) {
        return PICO_ERROR_GENERIC;
    }
    if(i2c_read_blocking(i2c, dev_addr, data, length, false) == PICO_ERROR_GENERIC) {
        return PICO_ERROR_GENERIC;
    }
    return 0;
}

/**
 * @brief               Library function to write to a specific I2C register adress.
 * 
 * @param[in] i2c       I2C instance used.
 * @param[in] dev_addr  Adress of the I2C device.
 * @param[in] reg_addr  Register adress to be written.
 * @param[in] length    Length of the data to be written in bytes.
 * @param[in] data      Pointer to the data buffer.
 * @return              0 if succesful, -1 if i2c failure.
 */
int i2c_write_reg(i2c_inst_t * i2c, uint8_t dev_addr, 
    uint8_t reg_addr, size_t length, uint8_t * data)
{
    if(!length) 
        return 0;
    
    uint8_t messeage[length + 1];
    messeage[0] = reg_addr;
    for(int i = 0; i < length; i++) {
        messeage[i + 1] = data[i];
    }
    i2c_write_blocking(i2c, dev_addr, messeage, (length + 1), false);
    return 0;
}

/**
 * @brief           Library function that takes an 8 bit unsigned integer and converts into
 *  data that can be written to DS3231 registers.
 * 
 * @param[in] data  Number to be converted.
 * @return          Number in hardware form
 */
uint8_t real_to_hw(const uint8_t data) {
    uint8_t ones_digit = (uint8_t)(data % 10);
    uint8_t twos_digit = (uint8_t)(data - ones_digit) / 10;
    return ((twos_digit << 4) + ones_digit);
}

uint8_t real_to_am_pm(uint8_t data) {
    uint8_t temp = real_to_hw(data);
    uint8_t am_pm = 0x00;
    if(data > 12) {
        am_pm = 0x01;
        data -= 12;
    }
    temp |= (am_pm << 5);
    return temp;
}



/**
 * @brief                   Initiliaze ds3231 struct and specify which I2C instance is going to be used.
 * 
 * @param[out] rtc          Pointer to the ds3231 struct.
 * @param[in] i2c           Chosen I2C instance.
 * @param[in] dev_addr      DS3231 device adress. Leave 0 for default adress.
 * @param[in] eeprom_addr   EEPROM device adress. Leave 0 for default adress. IF an incorrect adress is entered, it is set to the default adress.
 * @return                  0 if succesful. 
 */
int ds3231_init(ds3231_t * rtc, i2c_inst_t * i2c, uint8_t dev_addr, uint8_t eeprom_addr) {
    rtc->am_pm_mode = false;
    rtc->i2c = i2c;
    if(dev_addr)
        rtc->ds3231_addr = dev_addr;
    else 
        rtc->ds3231_addr = DS3231_DEVICE_ADRESS;

    switch (eeprom_addr)
    {
    case AT24C32_EEPROM_ADRESS_0:
        rtc->at24c32_addr = AT24C32_EEPROM_ADRESS_0;
        break;
    case AT24C32_EEPROM_ADRESS_1:
        rtc->at24c32_addr = AT24C32_EEPROM_ADRESS_1;    
        break;
    case AT24C32_EEPROM_ADRESS_2:
        rtc->at24c32_addr = AT24C32_EEPROM_ADRESS_2;
        break;
    case AT24C32_EEPROM_ADRESS_3:
        rtc->at24c32_addr = AT24C32_EEPROM_ADRESS_3;
        break;
    case AT24C32_EEPROM_ADRESS_4:
        rtc->at24c32_addr = AT24C32_EEPROM_ADRESS_4;
        break;
    case AT24C32_EEPROM_ADRESS_5:
        rtc->at24c32_addr = AT24C32_EEPROM_ADRESS_5;
        break;
    case AT24C32_EEPROM_ADRESS_6:
        rtc->at24c32_addr = AT24C32_EEPROM_ADRESS_6;
        break;
    case AT24C32_EEPROM_ADRESS_7:
        rtc->at24c32_addr = AT24C32_EEPROM_ADRESS_7;
        break;
    default:
        rtc->at24c32_addr = AT24C32_EEPROM_ADRESS_0;
        break;
    }
    return 0;
}

/**
 * @brief               Enable or disable AM/PM mode of DS3231. By default, it is disabled
 * 
 * @param[in] rtc       DS3231 struct.           
 * @param[in] enable    If true, AM_PM mode is enabled. If false, 24-Hour mode is enabled.
 * @return              0 if succeful.
 */
int ds3231_enable_am_pm_mode(ds3231_t * rtc, bool enable) {
    uint8_t temp = 0;
    if(i2c_read_reg(rtc->i2c, rtc->ds3231_addr, DS3231_HOURS_REG, 1, &temp))
        return -1;
    if(enable) {
        temp |= (0x01 << 6);
        rtc->am_pm_mode = true;
    } else {
        rtc->am_pm_mode = false;
        temp &= ~(0x01 << 6);
    }
    if(i2c_write_reg(rtc->i2c, rtc->ds3231_addr, DS3231_HOURS_REG, 1, &temp))
        return -1;
    return 0;
}

int ds3231_configure_time(ds3231_t * rtc, ds3231_data_t * data) {
    uint8_t temp[7] = {0, 0, 0, 0, 0, 0, 0};
    if(i2c_read_reg(rtc->i2c, rtc->ds3231_addr, DS3231_SECONDS_REG, 7, temp)) 
        return -1;
    
    temp[0] = real_to_hw(data->seconds);

    temp[1] = real_to_hw(data->minutes);

    if(rtc->am_pm_mode) {
        temp[2] = real_to_am_pm(data->hours);
        temp[2] |= (0x01 << 6);
    } else {
        temp[2] = real_to_hw(data->hours);
        temp[2] &= ~(0x01 << 6);
    }

    temp[3] = real_to_hw(data->day);

    temp[4] = real_to_hw(data->date);

    temp[5] = real_to_hw(data->month);

    if(data->century)
        temp[5] |= (0x01 << 7);
    
    temp[6] = real_to_hw(data->year);
    
    if(i2c_write_reg(rtc->i2c, rtc->ds3231_addr, DS3231_SECONDS_REG, 7, temp)) 
        return -1;
    return 0;
}

/**
 * @brief               Reads the timekeeping registers and converts time to real units.
 * 
 * @param[in]   rtc     ds3231 struct.    
 * @param[out]  data    data struct to save converted time units.
 * @return              0 if succesful. 
 */
int ds3231_read_current_time(ds3231_t * rtc, ds3231_data_t * data) {
    uint8_t raw_data[7];
    if(i2c_read_reg(rtc->i2c, rtc->ds3231_addr, DS3231_SECONDS_REG, 7, raw_data)) 
        return -1;
    
    data->seconds = 10 * ((raw_data[0] & 0x70) >> 4) + (raw_data[0] & 0x0F);

    data->minutes = 10 * ((raw_data[1] & 0x70) >> 4) + (raw_data[1] & 0x0F);
    
    if(rtc->am_pm_mode) {
        data->hours   = 10 * ((raw_data[2] & 0x10) >> 4) + (raw_data[2] & 0x0F);
        data->am_pm = ((raw_data[2] & 0x20) >> 5);
    } else {
        data->hours   = 10 * ((raw_data[2] & 0x30) >> 4) + (raw_data[2] & 0x0F);
    }

    data->day     = (raw_data[3] & (0x07));

    data->date    = 10 * ((raw_data[4] & 0x30) >> 4) + (raw_data[4] & 0x0F);

    data->month   = 10 * ((raw_data[5] & 0x10) >> 4) + (raw_data[5] & 0x0F);

    data->century = (raw_data[5] & (0x01 << 7)) >> 7;

    data->year    = 10 * ((raw_data[6] & 0xF0) >> 4) + (raw_data[6] & 0x0F);

    return 0;
}

/**
 * @brief                   Enable alarm on DS3231 alarm 1. Valid alarm triggers enums are:
 *\n ON_EVERY_SECOND,
 *\n ON_MATCHING_SECOND,
 *\n ON_MATCHING_SECOND_AND_MINUTE,
 *\n ON_MATCHING_SECOND_MINUTE_AND_HOUR, 
 *\n ON_MATCHING_SECOND_MINUTE_HOUR_AND_DATE,
 *\n ON_MATCHING_SECOND_MINUTE_HOUR_AND_DAY, 
 *
 * @param[in] rtc           DS3231 struct pointer.
 * @param[in] alarm_time    Struct that holds time data that will trigger the alarm.
 * @param[in] mask          Valid alarm states.
 * @return                  0 if succesful.
 */
int ds3231_set_alarm_1(ds3231_t * rtc, ds3231_alarm_1_t * alarm_time, enum ALARM_1_MASKS mask) {
    uint8_t temp[4] = {0, 0 , 0, 0};
    if(i2c_read_reg(rtc->i2c, rtc->ds3231_addr, DS3231_SECONDS_ALARM_1_REG, 4, temp))
        return -1;
    switch(mask) {
        case ON_EVERY_SECOND:
            for(int i = 0; i < 4; i++) 
                temp[i] |= (0x01 << 7);
        break;

        case ON_MATCHING_SECOND:
            temp[0] = real_to_hw(alarm_time->seconds);
            temp[0] &= ~(0x01 << 7);
            for(int i = 1; i < 4; i++) 
                temp[i] |= (0x01 << 7);
        break;

        case ON_MATCHING_SECOND_AND_MINUTE:
            temp[0] = real_to_hw(alarm_time->seconds);
            temp[1] = real_to_hw(alarm_time->minutes);
            for(int i = 0; i < 2; i++) 
                temp[i] &= ~(0x01 << 7);
            for(int i = 2; i < 4; i++) 
                temp[i] |= (0x01 << 7);
        break;

        case ON_MATCHING_SECOND_MINUTE_AND_HOUR:
            temp[0] = real_to_hw(alarm_time->seconds);
            temp[1] = real_to_hw(alarm_time->minutes);
            if(rtc->am_pm_mode) {
                temp[2] = real_to_am_pm(alarm_time->hours);
                temp[2] |= (0x01 << 6);        
            } else {
                temp[2] = real_to_hw(alarm_time->hours);
                temp[2] &= ~(0x01 << 6);        
            }     
            for(int i = 0; i < 3; i++)
                temp[i] &= ~(0x01 << 7);            
            temp[3] |= (0x01 << 7);
        break;

        case ON_MATCHING_SECOND_MINUTE_HOUR_AND_DATE:
            temp[0] = real_to_hw(alarm_time->seconds);
            temp[1] = real_to_hw(alarm_time->minutes);
            if(rtc->am_pm_mode) {
                temp[2] = real_to_am_pm(alarm_time->hours);
                temp[2] |= (0x01 << 6);        
            } else {
                temp[2] = real_to_hw(alarm_time->hours);
                temp[2] &= ~(0x01 << 6);        
            }     
            temp[3] = real_to_hw(alarm_time->date);
            temp[3] &= ~(0x01 << 6);
            for(int i = 0; i < 3; i++)
                temp[i] &= ~(0x01 << 7);            
            temp[3] |= (0x01 << 7);
        break;

        case ON_MATCHING_SECOND_MINUTE_HOUR_AND_DAY:
            temp[0] = real_to_hw(alarm_time->seconds);
            temp[1] = real_to_hw(alarm_time->minutes);
            if(rtc->am_pm_mode) {
                temp[2] = real_to_am_pm(alarm_time->hours);
                temp[2] |= (0x01 << 6);        
            } else {
                temp[2] = real_to_hw(alarm_time->hours);
                temp[2] &= ~(0x01 << 6);        
            }     
            temp[3] = real_to_hw(alarm_time->day);
            temp[3] |= (0x01 << 6);
            for(int i = 0; i < 3; i++)
                temp[i] &= ~(0x01 << 7);            
            temp[3] |= (0x01 << 7);
        break;

        default:
            return -1;
        break;
    }

    uint8_t alarm_enable = 0;
    if(i2c_read_reg(rtc->i2c, rtc->ds3231_addr, DS3231_CONTROL_REG, 1, &alarm_enable))
        return -1;    
    alarm_enable |= (0x01);
    if(i2c_write_reg(rtc->i2c, rtc->ds3231_addr, DS3231_CONTROL_REG, 1, &alarm_enable))
        return -1;

    if(i2c_write_reg(rtc->i2c, rtc->ds3231_addr, DS3231_SECONDS_ALARM_1_REG, 4, temp))
        return -1;
    return 0;
}

/**
 * @brief                   Enable alarm on DS3231 alarm 2. Valid alarm triggers enums are:
 *\n ON_EVERY_MINUTE,
 *\n ON_MATCHING_MINUTE,
 *\n ON_MATCHING_MINUTE_AND_HOUR,
 *\n ON_MATCHING_MINUTE_HOUR_AND_DATE, 
 *\n ON_MATCHING_MINUTE_HOUR_AND_DAY, 
 *
 * @param[in] rtc           DS3231 struct pointer.
 * @param[in] alarm_time    Struct that holds time data that will trigger the alarm.
 * @param[in] mask          Valid alarm states.
 * @return                  0 if succesful.
 */
int ds3231_set_alarm_2(ds3231_t * rtc, ds3231_alarm_2_t * alarm_time, enum ALARM_2_MASKS mask) {
    uint8_t temp[3] = {0, 0 , 0};
    if(i2c_read_reg(rtc->i2c, rtc->ds3231_addr, DS3231_MINUTES_ALARM_2_REG, 3, temp))
        return -1;
    switch(mask) {
        case ON_EVERY_MINUTE:
            for(int i = 0; i < 3; i++)
                temp[i] |= (0x01 << 7);
        break;

        case ON_MATCHING_MINUTE:
            temp[0] = real_to_hw(alarm_time->minutes);
            temp[0] &= ~(0x01 << 7);
            for(int i = 1; i < 3; i++)
                temp[i] |= (0x01 << 7);
        break;

        case ON_MATCHING_MINUTE_AND_HOUR:
            temp[0] = real_to_hw(alarm_time->minutes);
            if(rtc->am_pm_mode) {
                temp[1] = real_to_am_pm(alarm_time->hours);
                temp[1] |= (0x01 << 6);
            } else {
                temp[1] = real_to_hw(alarm_time->hours);
                temp[1] &= ~(0x01 << 6);
            }
            for(int i = 0; i < 2; i++)
                temp[i] &= ~(0x01 << 7);
            temp[2] |= (0x01 << 7);
        break;

        case ON_MATCHING_MINUTE_HOUR_AND_DATE:
            temp[0] = real_to_hw(alarm_time->minutes);
            if(rtc->am_pm_mode) {
                temp[1] = real_to_am_pm(alarm_time->hours);
                temp[1] |= (0x01 << 6);
            } else {
                temp[1] = real_to_hw(alarm_time->hours);
                temp[1] &= ~(0x01 << 6);
            }
            temp[2] = real_to_hw(alarm_time->date);
            temp[2] &= ~(0x01 << 6);
            for(int i = 0; i < 3; i++)
                temp[i] &= ~(0x01 << 7);
        break;

        case ON_MATCHING_MINUTE_HOUR_AND_DAY:
            temp[0] = real_to_hw(alarm_time->minutes);
            if(rtc->am_pm_mode) {
                temp[1] = real_to_am_pm(alarm_time->hours);
                temp[1] |= (0x01 << 6);
            } else {
                temp[1] = real_to_hw(alarm_time->hours);
                temp[1] &= ~(0x01 << 6);
            }
            temp[2] = real_to_hw(alarm_time->date);
            temp[2] |= (0x01 << 6);
            for(int i = 0; i < 3; i++)
                temp[i] &= ~(0x01 << 7);
        break;

        default:
            return -1;
        break;
    }

    uint8_t alarm_enable = 0;
    if(i2c_read_reg(rtc->i2c, rtc->ds3231_addr, DS3231_CONTROL_REG, 1, &alarm_enable))
        return -1;    
    alarm_enable |= (0x02);
    if(i2c_write_reg(rtc->i2c, rtc->ds3231_addr, DS3231_CONTROL_REG, 1, &alarm_enable))
        return -1;

    if(i2c_write_reg(rtc->i2c, rtc->ds3231_addr, DS3231_MINUTES_ALARM_2_REG, 3, temp))
        return -1;
    return 0;
}


/**
 * @brief               Enable alarm interrupt on DS3231. If either alarm 1 or alarm 2 is enabled,
 * DS3231 will send a signal through INT/SQW pin. Default mode is enabled
 * 
 * @param[in] rtc       DS3231 struct.
 * @param[in] enable    Enabled if true, disabled if false;
 * @return int 
 */
int ds3231_enable_alarm_interrupt(ds3231_t * rtc, bool enable) {
    uint8_t interrupt_enable = 0;
    if(i2c_read_reg(rtc->i2c, rtc->ds3231_addr, DS3231_CONTROL_REG, 1, &interrupt_enable))
        return -1;    
    if(enable)
        interrupt_enable |= (0x01 << 2);
    else 
        interrupt_enable &= ~(0x01 << 2);
    if(i2c_write_reg(rtc->i2c, rtc->ds3231_addr, DS3231_CONTROL_REG, 1, &interrupt_enable))
        return -1;
    return 0;
} 

/**
 * @brief               Enable or disable oscillator on DS3231. It is enabled by default.
 * If disabled, oscillator stops if running on on-board battery (Vbat)  . 
 * 
 * @param[in] rtc       DS3231 struct.
 * @param[in] enable    Enabled if true, disabled if false.
 * @return              0 if succesful. 
 */
int ds3231_enable_oscillator(ds3231_t * rtc, bool enable) {
    uint8_t enable_byte = 0;
    if(i2c_read_reg(rtc->i2c, rtc->ds3231_addr, DS3231_CONTROL_REG, 1, &enable_byte))
        return -1;    
    if(!enable)
        enable_byte |= (0x01 << 7);
    else 
        enable_byte &= ~(0x01 << 7);
    if(i2c_write_reg(rtc->i2c, rtc->ds3231_addr, DS3231_CONTROL_REG, 1, &enable_byte))
        return -1; 
    return 0;   
}

/**
 * @brief               Enable or disable square-wave output from INT/SQW pin. 
 * This functionality is disabled if alarm interrupts are enabled. It is disabled by default.
 * 
 * @param[in] rtc       DS3231 struct.
 * @param[in] enable    Enabled if true, disabled if false.
 * @return              0 if succesful.
 */
int ds3231_enable_battery_backed_square_wave(ds3231_t * rtc, bool enable) {
    uint8_t enable_byte = 0;
    if(i2c_read_reg(rtc->i2c, rtc->ds3231_addr, DS3231_CONTROL_REG, 1, &enable_byte))
        return -1;    
    if(enable) {
        enable_byte |= (0x01 << 6);
        enable_byte &= ~(0x01 << 2);
    }
    else 
        enable_byte &= ~(0x01 << 6);
    if(i2c_write_reg(rtc->i2c, rtc->ds3231_addr, DS3231_CONTROL_REG, 1, &enable_byte))
        return -1;    
    return 0;
}

/**
 * @brief               Set the frequency of the square-wave output from INT_SQW pin.
 * Valid expressions for sqr_fqr are:
 * \n FREQUENCY_1_HZ,
 * \n FREQUENCY_1024_HZ,
 * \n FREQUENCY_4096_HZ,
 * \n FREQUENCY_8192_HZ
 * @param rtc           DS3231 struct.
 * @param sqr_frq       Frequency enum.
 * @return              0 if succesful.
 */
int ds3231_set_square_wave_frequency(ds3231_t * rtc, enum SQUARE_WAVE_FREQUENCY sqr_frq) {
    uint8_t enable_byte = 0;
    if(i2c_read_reg(rtc->i2c, rtc->ds3231_addr, DS3231_CONTROL_REG, 1, &enable_byte))
        return -1;    

    enable_byte &= ~(0x18);

    switch(sqr_frq) {
        case FREQUENCY_1_HZ:
        enable_byte |= (FREQUENCY_1_HZ << 3);
        break;

        case FREQUENCY_1024_HZ:
        enable_byte |= (FREQUENCY_1024_HZ << 3);
        break;

        case FREQUENCY_4096_HZ:
        enable_byte |= (FREQUENCY_4096_HZ << 3);
        break;

        case FREQUENCY_8192_HZ:
        enable_byte |= (FREQUENCY_8192_HZ << 3);
        break;

        default:
            return -1;
        break;
    }

    if(i2c_write_reg(rtc->i2c, rtc->ds3231_addr, DS3231_CONTROL_REG, 1, &enable_byte))
        return -1;   
    return  0;
}


/**
 * @brief               Set a interrupt callback function to trigger whenever DS3231 sends an alarm signal.
 * 
 * @param[out] gpio     Pin to receive the interrupt signal.     
 * @param[out] callback Pointer to the callback function.
 * @return              0 if succesful.
 */
int ds3231_set_interrupt_callback_function(uint gpio, gpio_irq_callback_t callback) {
    gpio_init(gpio);
    gpio_set_dir(gpio, 0);
    gpio_pull_up(gpio);

    gpio_set_irq_enabled_with_callback(gpio, GPIO_IRQ_EDGE_FALL, true, callback);
    return 0;
}

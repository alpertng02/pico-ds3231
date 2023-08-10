#include "pico/stdlib.h"
#include "ds3231.h"
#include <stdio.h>


void ds3231_interrupt_callback(uint gpio, uint32_t event_mask) {
    printf("Alarm Enabled\n");
}

int main() {
    stdio_init_all();
    uint8_t sda_pin = 12; 
    uint8_t scl_pin = 13;
    uint8_t int_pin = 18; 

    ds3231_data_t ds3231_data = {
        .seconds = 25,
        .minutes = 23,
        .hours = 23,
        .day = 4,
        .date = 10,
        .month = 8,
        .year = 23,
        .century = 1,
        .am_pm = false
    };
    ds3231_t ds3231;
    ds3231_init(&ds3231, i2c_default, DS3231_DEVICE_ADRESS, AT24C32_EEPROM_ADRESS_0);

    sleep_ms(200);


    gpio_init(25);
    gpio_set_dir(25, 1);
    gpio_put(25, 1);



    gpio_init(sda_pin);
    gpio_init(scl_pin);
    gpio_set_function(sda_pin, GPIO_FUNC_I2C);
    gpio_set_function(scl_pin, GPIO_FUNC_I2C);
    gpio_pull_up(sda_pin);
    gpio_pull_up(scl_pin);
    i2c_init(ds3231.i2c, 400 * 1000);

    ds3231_alarm_1_t alarm = {
        .seconds = 7,
        .minutes = 25,
        .hours = 0,
        .date = 0,
        .day = 0,
        .am_pm = false
    };

    ds3231_configure_time(&ds3231, &ds3231_data);
    sleep_ms(10);
    ds3231_set_alarm_1(&ds3231, &alarm, ON_MATCHING_SECOND_AND_MINUTE);

    ds3231_set_interrupt_callback_function(int_pin, &ds3231_interrupt_callback);
    sleep_ms(2000);
    printf("Starting Loop:\n");

    while(true) {
        if(ds3231_read_current_time(&ds3231, &ds3231_data)) {
            printf("No data is receivedç\n");
        } else {
            printf("%02u:%02u:%02u  %02u/%02u/20%02u\n", 
            ds3231_data.hours, ds3231_data.minutes, ds3231_data.seconds, ds3231_data.date, ds3231_data.month, ds3231_data.year);
            // printf("%02u:%02u:%02u\n", ds3231_data.hours, ds3231_data.minutes, ds3231_data.seconds);
        }
        gpio_put(25, 0);
        sleep_ms(500);
        gpio_put(25, 1);
        sleep_ms(500);
    }
}
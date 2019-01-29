#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "driver/gpio.h"
#include "driver/i2c.h"
#include "esp_err.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/portmacro.h"
#include "freertos/task.h"
#include "sdkconfig.h"

#include "I2Cbus.hpp"
#include "MPU.hpp"
#include "mpu/math.hpp"
#include "mpu/types.hpp"
//#include "mqtt_app.h"

extern "C" {
	void app_main(void);
    void mqtt_app(void *);
}

void mqtt_caller(void *);
void mpu_reader(void *);


static const char* TAG2 = "example";
float x, y, z;
char x_acc[10], y_acc[10], z_acc[10], dummy_string[20];


static constexpr gpio_num_t SDA = GPIO_NUM_21;
static constexpr gpio_num_t SCL = GPIO_NUM_22;
static constexpr uint32_t CLOCK_SPEED = 100000;  // range from 100 KHz ~ 400Hz

/*
float pow1(int n, int afterpoint){
    float power=1;
    for(int i=0; i<afterpoint; i++)
        power = n*power;
    return power;
}*/
// reverses a string 'str' of length 'len' 
void reverse(char *str, int len) 
{ 
    int i=0, j=len-1, temp; 
    while (i<j) 
    { 
        temp = str[i];
        str[i] = str[j]; 
        str[j] = temp; 
        i++; j--; 
    } 
} 

int intToStr(int x, char str[], int d) 
{ 
    int i = 0; 
    while (x) 
    { 
        str[i++] = (x%10) + '0'; 
        x = x/10; 
    } 
  
    // If number of digits required is more, then 
    // add 0s at the beginning 
    while (i < d) 
        str[i++] = '0'; 
  
    reverse(str, i); 
    str[i] = '\0'; 
    return i; 
} 
  
// Converts a floating point number to string. 
void ftoa(float n, char *res, int afterpoint) 
{ 
    // Extract integer part 
    int ipart = (int)n, i;
    float fpart;
  
     
    if(ipart>n){        // if the number is negative
        res[0] = '-';   // add first
        res[1] = '0';
        n = (-n);
        ipart = (-ipart);
        // Extract floating part 
        fpart = n - (float)ipart;
        // convert integer part to string 
        i = intToStr(ipart, res+2, 0);
        i += 2;
    }
    else{
        res[0] = '0';
        // Extract floating part 
        fpart = n - (float)ipart;
        // convert integer part to string 
        i = intToStr(ipart, res+1, 0);
        i++;
    }
 
  
    // check for display option after point 
    if (afterpoint != 0) 
    { 
        res[i] = '.';  // add dot 
        fpart = fpart * pow(10, afterpoint); 
  
        intToStr((int)fpart, res + i + 1, afterpoint); 
    } 
}


void app_main(void) {

    xTaskCreate(mpu_reader, "MPU_reader", 40960, NULL, 5, NULL);
    xTaskCreate(mqtt_caller, "MQTT", 20480, NULL, 5, NULL);
}

void mpu_reader(void *args){
    printf("$ MPU Driver Example: MPU-I2C\n");
    fflush(stdout);

    // Initialize I2C on port 0 using I2Cbus interface
    i2c0.begin(SDA, SCL, CLOCK_SPEED);

    // Or directly with esp-idf API
    /*
    i2c_config_t conf;
    conf.mode = I2C_MODE_MASTER;
    conf.sda_io_num = SDA;
    conf.scl_io_num = SCL;
    conf.sda_pullup_en = GPIO_PULLUP_ENABLE;
    conf.scl_pullup_en = GPIO_PULLUP_ENABLE;
    conf.master.clk_speed = CLOCK_SPEED;
    ESP_ERROR_CHECK(i2c_param_config(I2C_NUM_0, &conf));
    ESP_ERROR_CHECK(i2c_driver_install(I2C_NUM_0, conf.mode, 0, 0, 0));
    */
    
    MPU_t MPU;  // create a default MPU object
    MPU.setBus(i2c0);  // set bus port, not really needed since default is i2c0
    MPU.setAddr(mpud::MPU_I2CADDRESS_AD0_LOW);  // set address, default is AD0_LOW
    //UBaseType_t res;

    // Great! Let's verify the communication
    // (this also check if the connected MPU supports the implementation of chip selected in the component menu)
    while (esp_err_t err = MPU.testConnection()) {
        ESP_LOGE(TAG2, "Failed to connect to the MPU, error=%#X", err);
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
    ESP_LOGI(TAG2, "MPU connection successful!");

    // Initialize
    ESP_ERROR_CHECK(MPU.initialize());  // initialize the chip and set initial configurations
    // Setup with your configurations
    ESP_ERROR_CHECK(MPU.setSampleRate(100));  // set sample rate to 100 Hz
    // ESP_ERROR_CHECK(MPU.setGyroFullScale(mpud::GYRO_FS_500DPS));
    // ESP_ERROR_CHECK(MPU.setAccelFullScale(mpud::ACCEL_FS_4G));

    // Reading sensor data
    printf("Reading sensor data:\n");
    mpud::raw_axes_t accelRaw;   // x, y, z axes as int16
    mpud::raw_axes_t gyroRaw;    // x, y, z axes as int16
    mpud::float_axes_t accelG;   // accel axes in (g) gravity format
    mpud::float_axes_t gyroDPS;  // gyro axes in (DPS) º/s format
    
    //xTaskCreate(&mqtt_app, "mqtt_task", 102400, NULL, 5, NULL);
    while (true) {
        // Read
        MPU.acceleration(&accelRaw);  // fetch raw data from the registers
        MPU.rotation(&gyroRaw);       // fetch raw data from the registers
        // MPU.motion(&accelRaw, &gyroRaw);  // read both in one shot
        // Convert
        accelG = mpud::accelGravity(accelRaw, mpud::ACCEL_FS_4G);
        gyroDPS = mpud::gyroDegPerSec(gyroRaw, mpud::GYRO_FS_500DPS);
        // Debug*/
        x = (float)accelG.x;
        ftoa(x, x_acc, 3);
        y = (float)accelG.y;
        ftoa(y, y_acc, 3);
        z = (float)accelG.z;
        ftoa(z, z_acc, 3);
/*
        printf("accel: [%s %s %s ] (G) \t", x_acc, y_acc, z_acc);
        printf("gyro: [%+7.2f %+7.2f %+7.2f ] (º/s)\n", gyroDPS[0], gyroDPS[1], gyroDPS[2]);*/
        vTaskDelay(10 / portTICK_PERIOD_MS);
    }
}

void mqtt_caller(void *args){
    mqtt_app((void *)dummy_string);
}

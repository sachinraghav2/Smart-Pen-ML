# Project Makefile #

PROJECT_NAME := mpu-i2c

# Path to MPU Driver
EXTRA_COMPONENT_DIRS += /home/raghav/esp/esp-idf/MPUdriver/include
# Path to I2Cbus
EXTRA_COMPONENT_DIRS += /home/raghav/esp/esp-idf/I2Cbus/include


include $(IDF_PATH)/make/project.mk
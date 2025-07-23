#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/gpio.h> // Ensure this is included for the GPIO_DT_SPEC_GET_OR check

// Include u8g2 library header files
#include "u8g2.h"
#include "u8x8.h"

// Include logging for debugging purposes
#define LOG_LEVEL CONFIG_LOG_DEFAULT_LEVEL // You can temporarily set this to CONFIG_LOG_LEVEL_DBG for max logs
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(main_app, LOG_LEVEL);

#define OLED_I2C_ADDR 0x3C // Confirmed by scan, so stick to this.

// Define the I2C device node from your DTS/overlay
// Make sure this matches the actual I2C node in your board's DTS or app.overlay
// Based on your scan, "xiao_i2c" seems to be the correct label.
#define I2C_DEV_NODE DT_NODELABEL(i2c22)

// Global u8g2 object
u8g2_t u8g2;

// u8g2's Zephyr I2C communication callback function
uint8_t u8x8_byte_zephyr_hw_i2c(u8x8_t *u8x8, uint8_t msg, uint8_t arg_int, void *arg_ptr)
{
    static const struct device *i2c_dev = NULL; // Declared static to get pointer only once

    switch (msg)
    {
    case U8X8_MSG_BYTE_INIT:
        // Get I2C device pointer
        if (i2c_dev == NULL)
        {
            i2c_dev = DEVICE_DT_GET(I2C_DEV_NODE);
            if (!device_is_ready(i2c_dev))
            {
                LOG_ERR("I2C device not ready! Node path: %s", DT_NODE_PATH(I2C_DEV_NODE));
                return 0; // Failure
            }
            LOG_INF("I2C device found: %s", i2c_dev->name);
        }
        break;
    case U8X8_MSG_BYTE_SET_DC:
        // For I2C, D/C is controlled by the control byte (0x00 for command, 0x40 for data)
        // This case is usually for SPI or parallel interfaces.
        break;
    case U8X8_MSG_BYTE_START_TRANSFER:
        // Start transfer, usually no specific operation needed for I2C here
        LOG_DBG("I2C transfer start.");
        break;
    case U8X8_MSG_BYTE_SEND:
        // arg_ptr points to the buffer, arg_int is the number of bytes in that buffer.
        // For I2C, u8g2 driver prepends a control byte (0x00 or 0x40).
        if (i2c_write(i2c_dev, arg_ptr, arg_int, OLED_I2C_ADDR) != 0)
        {
            LOG_ERR("I2C write failed to 0x%02X! Len: %d, First byte: 0x%02X",
                    OLED_I2C_ADDR, arg_int, ((uint8_t *)arg_ptr)[0]);
            return 0; // Failure
        }
        LOG_DBG("I2C written %d bytes to 0x%02X. First byte: 0x%02X",
                arg_int, OLED_I2C_ADDR, ((uint8_t *)arg_ptr)[0]);
        break;
    case U8X8_MSG_BYTE_END_TRANSFER:
        // End transfer, usually no specific operation needed
        LOG_DBG("I2C transfer end.");
        break;
    default:
        LOG_WRN("Unknown U8X8_MSG_BYTE message: %d", msg);
        return 0; // Unknown message
    }
    return 1; // Success
}

// u8g2's Zephyr GPIO/delay callback function
uint8_t u8x8_gpio_and_delay_zephyr(u8x8_t *u8x8, uint8_t msg, uint8_t arg_int, void *arg_ptr)
{
    // If you confirmed RST is not needed, this part becomes simple.
    // If you ever need a GPIO for future displays, this is where you'd put it.

    switch (msg)
    {
    case U8X8_MSG_DELAY_NANO:
        k_busy_wait(arg_int);
        break;
    case U8X8_MSG_DELAY_100NANO:
        k_busy_wait(arg_int * 100);
        break;
    case U8X8_MSG_DELAY_10MICRO:
        k_busy_wait(arg_int * 10);
        break;
    case U8X8_MSG_DELAY_MILLI:
        k_sleep(K_MSEC(arg_int));
        break;
    case U8X8_MSG_DELAY_I2C:
        k_busy_wait(arg_int * 10); // Adjust as needed for I2C clock stretching
        break;
    // Since RST is not needed, these GPIO cases can remain empty or be removed if you prefer.
    // U8X8_MSG_GPIO_RESET will be called by u8g2, but we don't need to do anything.
    case U8X8_MSG_GPIO_RESET:
    case U8X8_MSG_GPIO_DC:
    case U8X8_MSG_GPIO_CS:
    case U8X8_MSG_GPIO_I2C_CLOCK:
    case U8X8_MSG_GPIO_I2C_DATA:
    case U8X8_MSG_GPIO_SPI_CLOCK:
    case U8X8_MSG_GPIO_SPI_DATA:
        break;
    default:
        LOG_WRN("Unknown U8X8_MSG_GPIO_AND_DELAY message: %d", msg);
        return 0; // Unknown message
    }
    return 1; // Success
}

int main(void)
{
    LOG_INF("Starting OLED u8g2 example...");

    // Select the appropriate initialization function
    // U8G2_R0: no rotation
    // u8x8_byte_zephyr_hw_i2c: our custom byte transmission callback
    // u8x8_gpio_and_delay_zephyr: our custom GPIO and delay callback
    u8g2_Setup_ssd1306_i2c_128x64_noname_f(&u8g2, U8G2_R0, u8x8_byte_zephyr_hw_i2c, u8x8_gpio_and_delay_zephyr);

    // Set the OLED's I2C slave address
    u8g2_SetI2CAddress(&u8g2, OLED_I2C_ADDR * 2); // u8g2 internally expects 8-bit address

    // Initialize the u8g2 object
    LOG_INF("Calling u8g2_InitDisplay...");
    u8g2_InitDisplay(&u8g2);
    LOG_INF("u8g2_InitDisplay called.");

    // Wake up the display (if it was in power save mode)
    LOG_INF("Calling u8g2_SetPowerSave...");
    u8g2_SetPowerSave(&u8g2, 0); // 0 = active, 1 = power save
    LOG_INF("u8g2_SetPowerSave called.");

    // Optional: Set contrast to max to ensure visibility
    u8g2_SetContrast(&u8g2, 255);
    LOG_INF("Contrast set to 255.");

    LOG_INF("u8g2 OLED initialized. Entering main loop.");

    while (1)
    {
        u8g2_ClearBuffer(&u8g2); // Clear internal buffer

        u8g2_SetFont(&u8g2, u8g2_font_ncenB14_tr); // Use u8g2 built-in font

        u8g2_SetDrawColor(&u8g2, 1); // Set text color (1 = white, 0 = black)

        u8g2_DrawStr(&u8g2, 0, 15, "nRF54L15");
        u8g2_DrawStr(&u8g2, 0, 35, "hello world");
        u8g2_DrawStr(&u8g2, 0, 55, "from u8g2");

        // Draw a single pixel at (0,0) to check basic drawing
        u8g2_DrawPixel(&u8g2, 0, 0);

        u8g2_SendBuffer(&u8g2); // Transfer buffer contents to OLED

        k_sleep(K_MSEC(1000)); // Update every 1 second
    }
    return 0;
}

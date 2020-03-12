#pragma once

#if (defined STM32L011xx) || (defined STM32L021xx) || \
	(defined STM32L031xx) || (defined STM32L041xx) || \
	(defined STM32L051xx) || (defined STM32L052xx) || (defined STM32L053xx) || \
	(defined STM32L061xx) || (defined STM32L062xx) || (defined STM32L063xx) || \
	(defined STM32L071xx) || (defined STM32L072xx) || (defined STM32L073xx) || \
	(defined STM32L081xx) || (defined STM32L082xx) || (defined STM32L083xx)
#include "stm32l0xx_hal.h"
#elif defined (STM32L412xx) || defined (STM32L422xx) || \
	defined (STM32L431xx) || (defined STM32L432xx) || defined (STM32L433xx) || defined (STM32L442xx) || defined (STM32L443xx) || \
	defined (STM32L451xx) || defined (STM32L452xx) || defined (STM32L462xx) || \
	defined (STM32L471xx) || defined (STM32L475xx) || defined (STM32L476xx) || defined (STM32L485xx) || defined (STM32L486xx) || \
    defined (STM32L496xx) || defined (STM32L4A6xx) || \
    defined (STM32L4R5xx) || defined (STM32L4R7xx) || defined (STM32L4R9xx) || defined (STM32L4S5xx) || defined (STM32L4S7xx) || defined (STM32L4S9xx)
#include "stm32l4xx_hal.h"
#else
#error Platform not implemented
#endif

#include <stdbool.h>

#ifndef PCD8544_SPI_TIMEOUT
#define PCD8544_SPI_TIMEOUT 1000
#endif

#define PCD8544_LCD_WIDTH 84
#define PCD8544_LCD_HEIGHT 48

typedef enum {
	PCD8544_COLOR_WHITE = 0,
	PCD8544_COLOR_BLACK
} pcd8544_color_t;

/**
 * Structure defining a handle describing a PCD8544 display driver device.
 */
typedef struct {

	/**
	 * The handle to the SPI bus for the device.
	 */
	SPI_HandleTypeDef *spi_handle;

	/**
	 * The port of the NSCE pin.
	 */
	GPIO_TypeDef *nsce_port;

	/**
	 * The NSCE pin.
	 */
	uint16_t nsce_pin;

	/**
	 * The port of the DNC pin.
	 */
	GPIO_TypeDef *dnc_port;

	/**
	 * The DNC pin.
	 */
	uint16_t dnc_pin;

	/**
	 * The port of the NRST pin.
	 */
	GPIO_TypeDef *nrst_port;

	/**
	 * The NRST pin.
	 */
	uint16_t nrst_pin;

	/**
	 * The back buffer for the display.
	 */
	uint8_t buffer[PCD8544_LCD_WIDTH * PCD8544_LCD_HEIGHT / 8];

	/**
	 * Set to true if display requires an update.
	 */
	bool update_required;

	/**
	 * The minimum X coordinate for the next display update.
	 */
	uint8_t update_min_x;

	/**
	 * The minimum Y coordinate for the next display update.
	 */
	uint8_t update_min_y;

	/**
	 * The maximum X coordinate for the next display update.
	 */
	uint8_t update_max_x;

	/**
	 * The maximum Y coordinate for the next display update.
	 */
	uint8_t update_max_y;

} pcd8544_handle_t;

/**
 * Initialises a PCD8544 device by performing a hardware reset, setting default bias and contrast values and clearing
 * the back buffer of the handle.
 * @param handle Handle to a PCD8544 device.
 * @return True on success, false otherwise.
 */
bool pcd8544_init(pcd8544_handle_t *handle);

/**
 *
 * @param handle Handle to a PCD8544 device.
 * @param contrast
 * @return True on success, false otherwise.
 */
bool pcd8544_set_contrast(pcd8544_handle_t *handle, uint8_t contrast);

/**
 * Set the display bias.
 * Asserts that the given bias is between 0 and 7.
 * @param handle Handle to a PCD8544 device.
 * @param bias Bias to set.
 * @return True on success, false otherwise.
 */
bool pcd8544_set_bias(pcd8544_handle_t *handle, uint8_t bias);

/**
 * Set the display to (not-)inverted mode.
 * @param handle Handle to a PCD8544 device.
 * @param inverted True for inverted, false for not inverted.
 * @return True on success, false otherwise.
 */
bool pcd8544_set_inverted(pcd8544_handle_t *handle, bool inverted);

/**
 * Sets a pixel of the back buffer to black or white.
 * Asserts that x is smaller than PCD8544_LCD_WIDTH.
 * Asserts that y is smaller than PCD8544_LCD_HEIGHT.
 * @param handle Handle to a PCD8544 device.
 * @param x The x coordinate of the pixel.
 * @param y The y coordinate of the pixel.
 * @param color Color to set.
 */
void pcd8544_set_pixel(pcd8544_handle_t *handle, uint8_t x, uint8_t y, pcd8544_color_t color);

/**
 * Updates the display by transmitting the back buffer to the device.
 * Must be called after all set pixel operations of the current frame.
 * @param handle Handle to a PCD8544 device.
 * @return True on success, false otherwise.
 */
bool pcd8544_update(pcd8544_handle_t *handle);

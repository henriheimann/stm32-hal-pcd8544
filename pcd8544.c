#include "pcd8544.h"

#include <assert.h>
#include <string.h>

#define PCD8544_SET_BIT(BYTE, BIT)      ((BYTE) |= (uint8_t)(1u << (uint8_t)(BIT)))
#define PCD8544_CLEAR_BIT(BYTE, BIT)    ((BYTE) &= (uint8_t)(~(uint8_t)(1u << (uint8_t)(BIT))))
#define PCD8544_MIN(VAL1, VAL2)         (((VAL1)<(VAL2))?(VAL1):(VAL2))
#define PCD8544_MAX(VAL1, VAL2)         (((VAL1)>(VAL2))?(VAL1):(VAL2))

#define PCD8544_EXTENDED_INSTRUCTION    ((uint8_t)0x01)

#define PCD8544_DISPLAYNORMAL   ((uint8_t)0x4)
#define PCD8544_DISPLAYINVERTED ((uint8_t)0x5)

// Instructions after without extended instruction control
#define PCD8544_FUNCTIONSET     ((uint8_t)0x20)
#define PCD8544_DISPLAYCONTROL  ((uint8_t)0x08)
#define PCD8544_SETYADDR        ((uint8_t)0x40)
#define PCD8544_SETXADDR        ((uint8_t)0x80)

// Instructions after setting extended instruction control
#define PCD8544_SETBIAS         ((uint8_t)0x10)
#define PCD8544_SETVOP          ((uint8_t)0x80)

static bool pcd8544_write_u8(pcd8544_handle_t *handle, uint8_t *values, uint16_t size)
{
	HAL_GPIO_WritePin(handle->nsce_port, handle->nsce_pin, GPIO_PIN_RESET);
	HAL_StatusTypeDef status = HAL_SPI_Transmit(handle->spi_handle, values, size, PCD8544_SPI_TIMEOUT);
	HAL_GPIO_WritePin(handle->nsce_port, handle->nsce_pin, GPIO_PIN_SET);
	return status == HAL_OK;
}

static bool pcd8544_data(pcd8544_handle_t *handle, uint8_t *data, uint16_t size)
{
	HAL_GPIO_WritePin(handle->dnc_port, handle->dnc_pin, GPIO_PIN_SET);
	return pcd8544_write_u8(handle, data, size);
}

static bool pcd8544_command(pcd8544_handle_t *handle, uint8_t command)
{
	HAL_GPIO_WritePin(handle->dnc_port, handle->dnc_pin, GPIO_PIN_RESET);
	return pcd8544_write_u8(handle, &command, 1);
}

bool pcd8544_init(pcd8544_handle_t *handle)
{
	assert(handle->spi_handle != NULL);
	assert(handle->dnc_port != NULL);
	assert(handle->nrst_port != NULL);
	assert(handle->nsce_port != NULL);

	// Initially unselect pin.
	HAL_GPIO_WritePin(handle->nsce_port, handle->nsce_pin, GPIO_PIN_SET);

	// Perform hardware reset.
	HAL_GPIO_WritePin(handle->nrst_port, handle->nrst_pin, GPIO_PIN_RESET);
	HAL_Delay(500);
	HAL_GPIO_WritePin(handle->nrst_port, handle->nrst_pin, GPIO_PIN_SET);

	bool success = true;

	// Set bias and contrast to default values.
	success &= pcd8544_set_bias(handle, 4);
	success &= pcd8544_set_contrast(handle, 50);

	// Set display to normal mode.
	success &= pcd8544_set_inverted(handle, true);

	// Clear the back buffer and initialise update variables.
	//memset(handle->buffer, 0, sizeof(handle->buffer));
	handle->update_required = true;
	handle->update_min_x = 0;
	handle->update_min_y = 0;
	handle->update_max_x = PCD8544_LCD_WIDTH - 1;
	handle->update_max_y = PCD8544_LCD_HEIGHT - 1;

	// Perform initial update of the display.
	success &= pcd8544_update(handle);

	return success;
}

bool pcd8544_set_contrast(pcd8544_handle_t *handle, uint8_t contrast)
{
	if (contrast > 0x7f) {
		return false;
	}

	bool success = true;

	success &= pcd8544_command(handle, PCD8544_FUNCTIONSET | PCD8544_EXTENDED_INSTRUCTION);
	success &= pcd8544_command(handle, PCD8544_SETVOP | contrast);
	success &= pcd8544_command(handle, PCD8544_FUNCTIONSET);

	return success;
}

bool pcd8544_set_bias(pcd8544_handle_t *handle, uint8_t bias)
{
	if (bias > 0x07) {
		return false;
	}

	bool success = true;

	success &= pcd8544_command(handle, PCD8544_FUNCTIONSET | PCD8544_EXTENDED_INSTRUCTION);
	success &= pcd8544_command(handle, PCD8544_SETBIAS | bias);
	success &= pcd8544_command(handle, PCD8544_FUNCTIONSET);

	return success;
}

bool pcd8544_set_inverted(pcd8544_handle_t *handle, bool inverted)
{
	if (inverted) {
		return pcd8544_command(handle, PCD8544_DISPLAYCONTROL | PCD8544_DISPLAYNORMAL);
	} else {
		return pcd8544_command(handle, PCD8544_DISPLAYCONTROL | PCD8544_DISPLAYINVERTED);
	}
}

void pcd8544_set_pixel(pcd8544_handle_t *handle, uint8_t x, uint8_t y, pcd8544_color_t color)
{
	assert(x < PCD8544_LCD_WIDTH);
	assert(y < PCD8544_LCD_HEIGHT);

	if (color == PCD8544_COLOR_BLACK) {
		PCD8544_SET_BIT(handle->buffer[x + (y / 8) * PCD8544_LCD_WIDTH], y % 8);
	} else {
		PCD8544_CLEAR_BIT(handle->buffer[x + (y / 8) * PCD8544_LCD_WIDTH], y % 8);
	}

	// If no update was previously required, we just need to update the single pixel.
	if (!handle->update_required) {
		handle->update_min_x = handle->update_max_x = x;
		handle->update_min_y = handle->update_max_y = y;
		handle->update_required = true;

	// Otherwise resize the update bounding box accordingly.
	} else {
		handle->update_min_x = PCD8544_MIN(x, handle->update_min_x);
		handle->update_min_y = PCD8544_MIN(y, handle->update_min_y);
		handle->update_max_x = PCD8544_MAX(x, handle->update_max_x);
		handle->update_max_y = PCD8544_MAX(y, handle->update_max_y);
	}
}

bool pcd8544_update(pcd8544_handle_t *handle)
{
	// Firstly, check if we need to update at all.
	if (!handle->update_required) {
		return true;
	}

	bool success = true;

	uint8_t min_page = handle->update_min_y / 8;
	uint8_t max_page = handle->update_max_y / 8;

	for (uint8_t page = min_page; page <= max_page; page++) {

		uint8_t min_column = handle->update_min_x;
		uint8_t max_column = handle->update_max_x;

		success &= pcd8544_command(handle, PCD8544_SETYADDR | page);
		success &= pcd8544_command(handle, PCD8544_SETXADDR | min_column);

		success &= pcd8544_data(handle, &handle->buffer[PCD8544_LCD_WIDTH * page + min_column], (max_column - min_column) + 1);
		success &= pcd8544_command(handle, PCD8544_SETYADDR);
	}

	if (success) {
		handle->update_required = false;
	}

	return success;
}

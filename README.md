# PCD8544
Driver for the PCD8544 display driver used in Nokia 5110 modules.

## Usage
Connecting the PCD8544 driver to your microcontroller via an SPI (CPOL = Low, CPHA = 1 Edge) bus and the NSCE 
(Output, Pull-Down), DNC (Output, Pull-Down) and NRST (Output, Pull-Up) pins and initialise them  using Cube.
After doing so, you can use this library to interact with the driver as shown in the following example:
```c
// Create the handle for the driver (Includes 504 byte of memory for the back buffer).
pcd8544_handle_t pcd8544_handle = {
        .spi_handle = &hspi1,
        .nsce_port = NOKIA5110_NSCE_GPIO_Port,
        .nsce_pin = NOKIA5110_NSCE_Pin,
        .dnc_port = NOKIA5110_DNC_GPIO_Port,
        .dnc_pin = NOKIA5110_DNC_Pin,
        .nrst_port = NOKIA5110_NRST_GPIO_Port,
        .nrst_pin = NOKIA5110_NRST_Pin,
};

// Initialise driver (performs basic setup and clears back buffer).
pcd8544_init(&pcd8544_handle);

// Set every second row to black (changes are only applied to back buffer).
for (uint8_t x = 0; x < PCD8544_LCD_WIDTH; x++) {
    for (uint8_t y = 0; y < PCD8544_LCD_HEIGHT; y++) {
        pcd8544_set_pixel(&pcd8544_handle, x, y, (pcd8544_color_t)(y % 2 == 0));   
    }
}

// Send back buffer to display.
pcd8544_update(&pcd8544_handle);
```

## Supported Platforms
STM32L0 and STM32L4 are supported. The HAL header includes for other platforms may be added in `pcd8544.h`.
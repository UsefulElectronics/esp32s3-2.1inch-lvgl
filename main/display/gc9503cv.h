/**
 ******************************************************************************
 *							USEFUL ELECTRONICS
 ******************************************************************************/
/**
 ******************************************************************************
 * @file    :  gc9503cv.h
 * @author  :  WARD ALMASARANI
 * @version :  v.1.0
 * @date    :  Dec 20, 2024
 * @link    :  https://www.youtube.com/@usefulelectronics
 *			   Hold Ctrl button and click on the link to be directed to
			   Useful Electronics YouTube channel	
 ******************************************************************************/

#ifndef MAIN_DISPLAY_GC9503CV_H_
#define MAIN_DISPLAY_GC9503CV_H_


/* INCLUDES ------------------------------------------------------------------*/
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_ops.h"
#include "esp_lcd_panel_vendor.h"
#include "esp_lcd_panel_rgb.h"
#include "esp_log.h"

#include "sdkconfig.h"
#include "../priv_include/bsp_err_check.h"
#include "../priv_include/bsp_sub_board.h"
#include "bsp/esp-bsp.h"
/* MACROS --------------------------------------------------------------------*/
/* LCD related parameters */
#define BSP_LCD_H_RES                   (480)
#define BSP_LCD_V_RES                   (480)
#define BSP_LCD_PIXEL_CLOCK_HZ          (10 * 1000 * 1000)
#define BSP_LCD_HSYNC_BACK_PORCH        (20)
#define BSP_LCD_HSYNC_FRONT_PORCH       (40)
#define BSP_LCD_HSYNC_PULSE_WIDTH       (8)
#define BSP_LCD_VSYNC_BACK_PORCH        (20)
#define BSP_LCD_VSYNC_FRONT_PORCH       (50)
#define BSP_LCD_VSYNC_PULSE_WIDTH       (8)
#define BSP_LCD_PCLK_ACTIVE_NEG         (false)

/* LVGL related parameters */
#define LVGL_TICK_PERIOD_MS         (CONFIG_BSP_DISPLAY_LVGL_TICK)
#define LVGL_BUFFER_HEIGHT          (CONFIG_BSP_DISPLAY_LVGL_BUF_HEIGHT)
#if CONFIG_BSP_DISPLAY_LVGL_PSRAM
#define LVGL_BUFFER_MALLOC          (MALLOC_CAP_SPIRAM)
#else
#define LVGL_BUFFER_MALLOC          (MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT)
#endif
/* ENUMORATIONS --------------------------------------------------------------*/

/* STRUCTURES & TYPEDEFS -----------------------------------------------------*/

/* VARIABLES -----------------------------------------------------------------*/
extern esp_lcd_panel_handle_t panel_handle;
/* FUNCTIONS DECLARATION -----------------------------------------------------*/
esp_lcd_panel_handle_t gc9503cv_lcd_init(void *arg);


#endif /* MAIN_DISPLAY_GC9503CV_H_ */

/*************************************** USEFUL ELECTRONICS*****END OF FILE****/

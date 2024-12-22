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

/* ENUMORATIONS --------------------------------------------------------------*/

/* STRUCTURES & TYPEDEFS -----------------------------------------------------*/

/* VARIABLES -----------------------------------------------------------------*/
extern esp_lcd_panel_handle_t panel_handle;
/* FUNCTIONS DECLARATION -----------------------------------------------------*/
esp_lcd_panel_handle_t gc9503cv_lcd_init(void *arg);


#endif /* MAIN_DISPLAY_GC9503CV_H_ */

/*************************************** USEFUL ELECTRONICS*****END OF FILE****/

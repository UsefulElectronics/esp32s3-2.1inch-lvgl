/**
 ******************************************************************************
 *							USEFUL ELECTRONICS
 ******************************************************************************/
/**
 ******************************************************************************
 * @file    :  display.c
 * @author  :  WARD ALMASARANI
 * @version :  v.1.0
 * @date    :  Feb 1, 2023
 * @link    :  https://www.youtube.com/@usefulelectronics
 *			   Hold Ctrl button and click on the link to be directed to
			   Useful Electronics YouTube channel	
 ******************************************************************************/


/* INCLUDES ------------------------------------------------------------------*/
#include "display.h"
#include "bsp/esp-bsp.h"

/* PRIVATE STRUCTRES ---------------------------------------------------------*/

/* VARIABLES -----------------------------------------------------------------*/
lv_disp_drv_t disp_drv;  // contains callback functions

/* DEFINITIONS ---------------------------------------------------------------*/

/* MACROS --------------------------------------------------------------------*/
static const char *TAG = "example";
#if CONFIG_BSP_DISPLAY_LVGL_FULL_REFRESH && CONFIG_BSP_LCD_RGB_BUFFER_NUMS == 3
static void *lvgl_port_rgb_last_buf = NULL;
static void *lvgl_port_rgb_next_buf = NULL;
static void *lvgl_port_flush_next_buf = NULL;
#endif
/* PRIVATE FUNCTIONS DECLARATION ---------------------------------------------*/
extern void example_lvgl_demo_ui(lv_disp_t *disp);
/* FUNCTION PROTOTYPES -------------------------------------------------------*/


bool display_notify_lvgl_flush_ready(esp_lcd_panel_io_handle_t panel_io, esp_lcd_panel_io_event_data_t *edata, void *user_ctx)
{
    lv_disp_drv_t *disp_driver = (lv_disp_drv_t *)user_ctx;
    lv_disp_flush_ready(disp_driver);
    return false;
}

static void example_lvgl_flush_cb(lv_disp_drv_t *drv, const lv_area_t *area, lv_color_t *color_map)
{
    esp_lcd_panel_handle_t panel_handle = (esp_lcd_panel_handle_t) drv->user_data;
    int offsetx1 = area->x1;
    int offsetx2 = area->x2;
    int offsety1 = area->y1;
    int offsety2 = area->y2;
    
    drv->draw_buf->buf1 = color_map;
    drv->draw_buf->buf2 = lvgl_port_flush_next_buf;
    lvgl_port_flush_next_buf = color_map;
    // copy a buffer's content to a specific area of the display
    esp_lcd_panel_draw_bitmap(panel_handle, offsetx1, offsety1, offsetx2 + 1, offsety2 + 1, color_map);
    
    lv_disp_flush_ready(drv);
}

/* Rotate display and touch, when rotated screen in LVGL. Called when driver parameters are updated. */
static void example_lvgl_port_update_callback(lv_disp_drv_t *drv)
{
    esp_lcd_panel_handle_t panel_handle = (esp_lcd_panel_handle_t) drv->user_data;

    switch (drv->rotated) {
    case LV_DISP_ROT_NONE:
        // Rotate LCD display
        esp_lcd_panel_swap_xy(panel_handle, false);
        esp_lcd_panel_mirror(panel_handle, true, false);

        break;
    case LV_DISP_ROT_90:
        // Rotate LCD display
        esp_lcd_panel_swap_xy(panel_handle, true);
        esp_lcd_panel_mirror(panel_handle, true, true);

        break;
    case LV_DISP_ROT_180:
        // Rotate LCD display
        esp_lcd_panel_swap_xy(panel_handle, false);
        esp_lcd_panel_mirror(panel_handle, false, true);

        break;
    case LV_DISP_ROT_270:
        // Rotate LCD display
        esp_lcd_panel_swap_xy(panel_handle, true);
        esp_lcd_panel_mirror(panel_handle, false, false);

        break;
    }
}



static void example_increase_lvgl_tick(void *arg)
{
    /* Tell LVGL how many milliseconds has elapsed */
    lv_tick_inc(EXAMPLE_LVGL_TICK_PERIOD_MS);
    
   // esp_lcd_rgb_panel_refresh(&panel_handle);
}


void displayConfig(void)
{
	void *buf1 = NULL;
    void *buf2 = NULL;
     int buffer_size;
    static lv_disp_draw_buf_t disp_buf; // contains internal graphic buffer(s) called draw buffer(s)
    lv_init();
    gc9503cv_lcd_init(NULL);
    
    #if CONFIG_BSP_DISPLAY_LVGL_AVOID_TEAR
    buffer_size = BSP_LCD_H_RES * BSP_LCD_V_RES;
#if CONFIG_BSP_DISPLAY_LVGL_FULL_REFRESH && CONFIG_BSP_LCD_RGB_BUFFER_NUMS == 3
    ESP_ERROR_CHECK(esp_lcd_rgb_panel_get_frame_buffer(panel_handle, 3, &lvgl_port_rgb_last_buf, &buf1, &buf2));
    lvgl_port_rgb_next_buf = lvgl_port_rgb_last_buf;
    lvgl_port_flush_next_buf = buf2;
#else
    BSP_ERROR_CHECK_RETURN_NULL(esp_lcd_rgb_panel_get_frame_buffer(panel_handle, 2, &buf1, &buf2));
#endif
#else
    buffer_size = BSP_LCD_H_RES * LVGL_BUFFER_HEIGHT;
    buf1 = heap_caps_malloc(buffer_size * sizeof(lv_color_t), LVGL_BUFFER_MALLOC);
    BSP_NULL_CHECK(buf1, NULL);
#endif
    // initialize LVGL draw buffers
    lv_disp_draw_buf_init(&disp_buf, buf1, buf2, buffer_size);

    ESP_LOGD(TAG, "Register display driver to LVGL");
    lv_disp_drv_init(&disp_drv);
    disp_drv.hor_res = BSP_LCD_H_RES;
    disp_drv.ver_res = BSP_LCD_V_RES;
    disp_drv.flush_cb = example_lvgl_flush_cb;
    disp_drv.drv_update_cb = example_lvgl_port_update_callback;
    disp_drv.draw_buf = &disp_buf;
    disp_drv.user_data = panel_handle;
#if CONFIG_BSP_DISPLAY_LVGL_FULL_REFRESH
    disp_drv.full_refresh = 1;
#elif CONFIG_BSP_DISPLAY_LVGL_DIRECT_MODE
    disp_drv.direct_mode = 1;
#endif
    lv_disp_t *disp = lv_disp_drv_register(&disp_drv);

    ESP_LOGI(TAG, "Display LVGL demo");
  
    gpio_set_level(BSP_LCD_BL, 0);


    ESP_LOGI(TAG, "Install LVGL tick timer");
    // Tick interface for LVGL (using esp_timer to generate 2ms periodic event)
    const esp_timer_create_args_t lvgl_tick_timer_args = 
    {
        .callback = &example_increase_lvgl_tick,
        .name = "lvgl_tick"
    };

    esp_timer_handle_t lvgl_tick_timer = NULL;
    ESP_ERROR_CHECK(esp_timer_create(&lvgl_tick_timer_args, &lvgl_tick_timer));
    ESP_ERROR_CHECK(esp_timer_start_periodic(lvgl_tick_timer, EXAMPLE_LVGL_TICK_PERIOD_MS * 1000));


    ESP_LOGI(TAG, "Display LVGL Meter Widget");
    example_lvgl_demo_ui(disp);


}
/*************************************** USEFUL ELECTRONICS*****END OF FILE****/

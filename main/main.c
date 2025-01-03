/**
 ******************************************************************************
 *							USEFUL ELECTRONICS
 ******************************************************************************/
/**
 ******************************************************************************
 * @file    :  main.c
 * @author  :  WARD ALMASARANI
 * @version :  v.1.0
 * @date    :  Jan 30, 2023
 * @link    :  https://www.youtube.com/@usefulelectronics
 *			   Hold Ctrl button and click on the link to be directed to
			   Useful Electronics YouTube channel	
 ******************************************************************************/


/* INCLUDES ------------------------------------------------------------------*/
#include "main.h"
#include "extra/widgets/colorwheel/lv_colorwheel.h"
#include "uart_config.h"
#include "esp_interrupt.h"

#include "hardware/rotary_encoder.h"
#include "hardware/button.h"


#include "display/display.h"

#include "display/lvgl_demo_ui.h"
#include "display/ui_helpers.h"

#include "wireless/sntp.h"


#include "time.h"
#include <math.h>
#include "lwip/ip_addr.h"
#include "timer/systimer.h"

#include "gpio/gpio_config.h"
#include "esp_log.h"
#include <stdbool.h>


/* MACROS --------------------------------------------------------------------*/
#define SYSTEM_BUFFER_SIZE					4
#define SYSTEM_ROTARY_ENCODER_STEP_SIZE		4
#define SYSTEM_LAMP_MODE_COUNT				3 	//see lv_colorwheel_mode_t
/* DEFINITIONS ---------------------------------------------------------------*/

/* PRIVATE STRUCTRES ---------------------------------------------------------*/
typedef struct
{
	uint8_t packet_size;
	uint16_t data[SYSTEM_BUFFER_SIZE];
}system_packet;

typedef struct {
    int16_t hue;
    int8_t saturation;
    int8_t brightness;
} system_hsv_t;

typedef struct
{
	bool on_state;
	system_hsv_t color;
	uint32_t encoder_rotary_angle;
	uint8_t control_mode;
}system_lamp_data_t;

char main_mqtt_topic_buffer[50];
/* VARIABLES -----------------------------------------------------------------*/
static const char *main = "main";
char targetString[10] = {0};

static const char* TAG = "main";
static system_lamp_data_t hLamp = {0};
static QueueHandle_t system_queue;

TaskHandle_t hMain_uiTask 				= NULL;
TaskHandle_t hMain_eventTask			= NULL;
/* PRIVATE FUNCTIONS DECLARATION ---------------------------------------------*/
static void time_config(void);
static void main_encoder_cb(uint32_t knobPosition);
static void main_tempretureStringPrepare(char* tempString, char* targetString);
static void lvgl_time_task(void*param);
static void wirless_init_task(void* param);
static void mqtt_msg_pars_task(void* param);
static void time_handle_task(void* param);
static void uart_reception_task(void *param);
static void event_handle_task(void* param);
static void main_system_timer_task(void* param);

static void main_rotary_button_event(void);
static uint32_t main_get_systick(void);
static char* main_mqtt_topic_string (uint16_t device_index, char* mqtt_topic);
static void main_lamp_init(void);
static void main_lamp_param_update(void);

/* FUNCTION PROTOTYPES -------------------------------------------------------*/
/**
 * @brief 	Initialize system peripherals and create FreeRTOS tasks
 *
 */
void app_main(void)
{
	displayConfig();
	
	main_lamp_init();
	
	systimer_init();

	gpio_config_ext_interrupt(KNOB_BUTTON, GPIO_INTR_NEGEDGE, gpio_isr_handle);

	button_init(main_get_systick, gpio_get_level);
	button_add(KNOB_BUTTON, 1, 1500, main_rotary_button_event);

	encoder_init(main_encoder_cb);
	
	system_queue 		= xQueueCreate(10, sizeof(system_packet));

	xTaskCreatePinnedToCore(wirless_init_task, "WiFi init", 10000, NULL, 4, NULL, 0);

	xTaskCreatePinnedToCore(encoder_handler_task, "encoder_handler", 10000, NULL, 4, NULL, 1);

    xTaskCreatePinnedToCore(lvgl_time_task, "lvgl_time_task", 10000, NULL, 4, NULL, 1);

   //Wait for WiFi and MQTT broker connection to be established.

 	vTaskDelay(pdMS_TO_TICKS(15000));
	
	xTaskCreatePinnedToCore(lvgl_time_task, "lvgl_time_task", 10000, NULL, 4, NULL, 1);
    
    xTaskCreatePinnedToCore(mqtt_msg_pars_task, "MQTT parser", 10000, NULL, 4, NULL, 1);
    
     xTaskCreatePinnedToCore(main_system_timer_task, "system timer task", 10000, NULL, 4, NULL, 1);

     //xTaskCreatePinnedToCore(time_handle_task, "Real time Handler", 10000, NULL, 4, NULL, 1);

     xTaskCreatePinnedToCore(event_handle_task, "lvgl_time_task", 10000, NULL, 4, &hMain_eventTask, 1);
}
/**
 * @brief 	LVGL library timer task. Necessary to run once every 10ms
 *
 */
void lvgl_time_task(void* param)
{
	TickType_t xLastWakeTime = xTaskGetTickCount();

	system_packet system_buffer = {0};
	while(1)
	{

        // The task running lv_timer_handler should have lower priority than that running `lv_tick_inc`
        lv_timer_handler();
		//WiFi connection check
		ui_set_wifi_switch_state(wifi_is_connected());

        button_manager();

        // raise the task priority of LVGL and/or reduce the handler period can improve the performance
        vTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS(10) );

	}
}
/**
 * @brief 	Encoder position read callback.
 *
 * @param 	knobPosition	: Read encoder position value.
 */
static void main_encoder_cb(uint32_t knobPosition)
{
	static uint32_t prev_encoder_value = 0;
	
	int8_t temp_rotation_direction = 0;
	
	if(!knobPosition)
	{
		temp_rotation_direction = SYSTEM_ROTARY_ENCODER_STEP_SIZE;

	}
	else
	{
		temp_rotation_direction = SYSTEM_ROTARY_ENCODER_STEP_SIZE * -1;

	}
	switch (hLamp.control_mode) 
	{
		case 0:
			hLamp.color.hue += temp_rotation_direction * 2;
			
			hLamp.color.hue = fmax(0, fmin(360, hLamp.color.hue));
			break;

		case 1:
			hLamp.color.saturation += temp_rotation_direction;
			
			hLamp.color.saturation = fmax(0, fmin(100, hLamp.color.saturation));
			break;
			
		case 2:
			hLamp.color.brightness += temp_rotation_direction;
			
			// Clamp brightness between 0 and 100
   			hLamp.color.brightness = fmax(0, fmin(100, hLamp.color.brightness));
			break;
	}

	//Start timer to publish light data after 1 s time out 

	ui_set_wheel_color(  &hLamp.color);
	
	systimer_start(40, 1, main_lamp_param_update);
	
	ESP_LOGI(TAG, "hue value %d brightness value %d sat value %d step value %d mode %d", hLamp.color.hue, hLamp.color.brightness ,hLamp.color.saturation, temp_rotation_direction, hLamp.control_mode);
	
	//ui_set_wheel_mode(hLamp.control_mode);
	
	prev_encoder_value = knobPosition;
//	set_value((int32_t) knobPosition, knobButtonStatus);
}

static void uart_reception_task(void *param)
{
   uartHandler_t uartHandler = {0};

   uint8_t movementType = 0;

   int16_t detectedDistance = 0;

   system_packet system_buffer = {0};
   for(;;)
   {
      //Waiting for UART packet to get received.
      if(xQueueReceive(uartRxStore_queue, (void * )&uartHandler, portMAX_DELAY))
      {
    	  detectedDistance = hlk_ld1125h_parse_packet(hUart.uart_rxBuffer,(uint8_t*) &movementType);

    	  if(-1 != detectedDistance)
    	  {
    		//   ESP_LOGI(main, "dis = %d, move type %d", detectedDistance, movementType);

    		  system_buffer.data[0] = detectedDistance;
    		  system_buffer.data[1] = movementType;

    		  system_buffer.packet_size = 2;

    		  xQueueSendToBack(system_queue, &system_buffer, portMAX_DELAY);
    	  }
      }
   }
}

/**
 * @brief Initialize WiFi and connect to The configured WiFi network. and then connect to the MQTT broker
 *
 */
static void wirless_init_task(void* param)
{
	wifi_connect();

	mqtt_app_start();

	vTaskDelete(NULL);
}

static void main_system_timer_task(void* param)
{
	TickType_t xLastWakeTime = xTaskGetTickCount();

	while(1)
	{
		vTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS(10));
		
		systimer_tick_handler();
	}
}

static void mqtt_msg_pars_task(void* param)
{
	mqtt_buffer_t mqttBuffer = {0};

	const char publishRequest = 1;

	while(1)
	{
		if(xQueueReceive(mqttSubscribe_queue, (void *)&mqttBuffer, portMAX_DELAY))
		{
			switch (mqttBuffer.eventType)
			{
				case MQTT_BROKER_CONNECT:
					//_ui_text_wifiIndicate(true);
					main_mqtt_topic_string (1, MQTT_LAMP_GETHSV);
					
					mqtt_publish(main_mqtt_topic_string (1, MQTT_LAMP_GETHSV), &publishRequest, 0);
					
					ui_set_mqtt_switch_state(true);
					break;
				case MQTT_BROKER_DISCONNECT:
					//_ui_text_wifiIndicate(false);
					ui_set_mqtt_switch_state(false);
					break;
				case MQTT_TOPIC_DATA_RX:
					if(strcmp(mqttBuffer.topicString, main_mqtt_topic_string (1, MQTT_LAMP_GETHSV)))
					{
						 sscanf(mqttBuffer.data, "%d, %d, %d",(int*) &hLamp.color.hue, (int*) &hLamp.color.saturation, (int*) &hLamp.color.brightness);
		
						 ESP_LOGI(TAG, "mqtt set hsv %s", mqttBuffer.data);
						 
						 ui_set_wheel_color(  &hLamp.color);
						
						ESP_LOGI(TAG, "MQTT_LAMP_GETHSV, ");
					}
					else if(strcmp(mqttBuffer.topicString, main_mqtt_topic_string (1, MQTT_LAMP_GETON)))
					{
						bool lamp_status = atoi(mqttBuffer.data);
						
						ui_set_lamp_state(lamp_status);
						
						hLamp.on_state = lamp_status;

				 		ESP_LOGI(TAG, "mqtt set status %s", mqttBuffer.data);
					}

					break;
				default:
					break;
			}
		}
	}
}

static void time_handle_task(void* param)
{
	struct tm  Time = {0};

	char tempString[3] = {0};

	TickType_t xLastWakeTime = xTaskGetTickCount();

	const uint32_t tempUpdatePeriod = 60 * 60; //1 hour

	static uint32_t tempUpdateCounter = 0;

	const char publishRequest = 1;
	while(1)
	{
		vTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS(1000) );

		sntp_time_get(&Time);

		sprintf(tempString,"%02d",Time.tm_hour);

		realTime.sel.hour = (tempString[1] << 8);
		realTime.sel.hour |= (tempString[0]);

		sprintf(tempString,"%02d",Time.tm_min);

		realTime.sel.minute = (tempString[1] << 8);
		realTime.sel.minute |= (tempString[0]);

		++tempUpdateCounter;

		if(tempUpdatePeriod <= tempUpdateCounter)
		{
			mqtt_publish(MQTT_REQUEST_TOPIC, &publishRequest, 1);

			tempUpdateCounter = 0;
		}

	}
}
static void event_handle_task(void* param)
{
	while(1)
	{
		//Note: CallbackID is cleared immediately after executing this task
		if(xTaskNotifyWait(0, ULONG_MAX, &interrupt_id, portMAX_DELAY ))
		{
			switch (interrupt_id)
			{
			case EXT_GPIO_INT:
				button_debounce(KNOB_BUTTON);
				break;
			
			default:
				break;
			}
		}
	}
}

/**
 * @brief 	Create a proper temperature value showing string
 *
 * @param 	tempString	:	string received over MQTT
 *
 * @param 	targetString:	string to copy the formed string to
 */
static void main_tempretureStringPrepare(char* tempString, char* targetString)
{
	const char tempUnit[4] 		= " °C";
	const char tempStingLength 	= 3;
	for(uint8_t i = 0; i < tempStingLength; ++i)
	{
		if(!isalnum((int)tempString[i]))
		{
			strcpy(&tempString[i], tempUnit);
			break;
		}
	}

	strcpy(targetString,tempString);

	targetString[6] = 0;

}
static void main_rotary_button_event(void)
{
	char temp_publish_string[5] = {0};
	button_state_t button_state = button_state_get(KNOB_BUTTON);
	if(button_state == BUTTON_CLICKED) 
	{
		ESP_LOGI(TAG, "Button pressed");
		//reverse value for state toggling 
		hLamp.on_state ^= 1;
		
		ui_set_lamp_state(hLamp.on_state);
				
		sprintf(temp_publish_string, "%d",hLamp.on_state);

		mqtt_publish(MQTT_LAMP_SETON, temp_publish_string , strlen(temp_publish_string));
		
		//Change wheel mode

	}
	else if (button_state == BUTTON_LONG_PRESSED)
	{
		++hLamp.control_mode;
		
		if(hLamp.control_mode >= SYSTEM_LAMP_MODE_COUNT)
		{
			hLamp.control_mode = 0;
		}
		
		ui_set_wheel_mode(hLamp.control_mode);
		
	
		ESP_LOGI(TAG, "Button long pressed");
	}
}
static uint32_t main_get_systick(void)
{
	return SYS_TICK();
}

static char* main_mqtt_topic_string (uint16_t device_index, char* mqtt_topic)
{
	memset(main_mqtt_topic_buffer, 0, sizeof(main_mqtt_topic_buffer));
	
	sprintf(main_mqtt_topic_buffer, "%d%s", device_index, mqtt_topic);
	
	return main_mqtt_topic_buffer;
}

static void main_lamp_init(void)
{
	hLamp.color.brightness = 100;
	hLamp.color.saturation = 100;
	hLamp.control_mode     = 0;
}

static void main_lamp_param_update(void)
{
	char temp_publish_string[30] = {0};
	
	sprintf(temp_publish_string, "%d, %d, %d", hLamp.color.hue, hLamp.color.saturation, hLamp.color.brightness);

	mqtt_publish(MQTT_LAMP_SETHSV, temp_publish_string , strlen(temp_publish_string));
}
/*************************************** USEFUL ELECTRONICS*****END OF FILE****/

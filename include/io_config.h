#ifndef _IO_CONFIG_H__
#define _IO_CONFIG_H__

#define EASYQ_RECV_BUFFER_SIZE  4096
#define EASYQ_SEND_BUFFER_SIZE  512 
#define EASYQ_PORT				1085

#define DEVICE_NAME				"lm"
#define EASYQ_LOGIN				DEVICE_NAME
#define FOTA_QUEUE				DEVICE_NAME":fota"
#define FOTA_STATUS_QUEUE		DEVICE_NAME":fota:status"
#define DISPLAY_CHAR_QUEUE		DEVICE_NAME

/* GPIO */

// DATA PIN
#define DATA_MUX			PERIPHS_IO_MUX_MTCK_U	
#define DATA_NUM			13
#define DATA_FUNC			FUNC_GPIO13

// Clock PIN
#define CLOCK_MUX			PERIPHS_IO_MUX_MTMS_U	
#define CLOCK_NUM			14
#define CLOCK_FUNC			FUNC_GPIO14


#endif


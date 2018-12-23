
// Internal 
#include "partition.h"
#include "wifi.h"
#include "config.h"
#include "io_config.h"

// SDK
#include <ets_sys.h>
#include <osapi.h>
#include <gpio.h>
#include <mem.h>
#include <user_interface.h>
#include <driver/uart.h> 
#include <upgrade.h>

// LIB: EasyQ
#include "easyq.h" 
#include "debug.h"


#define VERSION				"2.2.1"

LOCAL EasyQSession eq;

#define T	200
LOCAL ETSTimer t;
LOCAL uint8_t tc = 0;
LOCAL int ta = 0;
void tf(void *args) {
	int i, x;
	display_clear();
	for (i = 3; i >= 0; i--) {
		x = i * 4 - ta % 4;
		display_char(tc + i, x);
		if (x > 0) {
			display_dot(x-1, 7, 1);
		}
	}
	display_draw();
	ta++;
	if (ta % 4 == 0) {
		tc++;
		if (tc >= 90) {
			os_timer_disarm(&t);
			return;
		}
	}
	os_timer_arm(&t, T, 0);
}
void start_t() {
	tc = 65;
	ta = 0;
	os_timer_disarm(&t);
	os_timer_setfn(&t, (os_timer_func_t*) tf, 0);
	os_timer_arm(&t, T, 0);
}


void ICACHE_FLASH_ATTR
fota_report_status(const char *q) {
	char str[50];
	float vdd = system_get_vdd33() / 1024.0;

	uint8_t image = system_upgrade_userbin_check();
	os_sprintf(str, "Image: %s Version: "VERSION" VDD: %d.%03d", 
			(UPGRADE_FW_BIN1 == image)? "FOTA": "APP",
			(int)vdd, 
			(int)(vdd*1000)%1000
		);
	easyq_push(&eq, q, str);
}


void ICACHE_FLASH_ATTR
easyq_message_cb(void *arg, const char *queue, const char *msg, 
		uint16_t message_len) {

	if (strcmp(queue, DISPLAY_DIGIT_QUEUE) == 0) { 
		uint8_t n = atoi(msg);
		display_number(n);
		display_draw();
	}
	else if (strcmp(queue, DISPLAY_CHAR_QUEUE) == 0) { 
		INFO("%d %d\r\n", (uint8_t)msg[0], (uint8_t)msg[1]);
		display_char((uint8_t)msg[0], (uint8_t)msg[1]);
		display_draw();
	}
	else if (strcmp(queue, FOTA_QUEUE) == 0) {
		if (msg[0] == 'R') {
			INFO("Rebooting to FOTA ROM\r\n");
			system_upgrade_flag_set(UPGRADE_FLAG_FINISH);
			system_upgrade_reboot();
		}
		else if (msg[0] == 'I') {
			fota_report_status(FOTA_STATUS_QUEUE);
		}
		else if (msg[0] == 'T') {
			start_t();
		}


	}
}


void ICACHE_FLASH_ATTR
easyq_connect_cb(void *arg) {
	INFO("EASYQ: Connected to %s:%d\r\n", eq.hostname, eq.port);
	INFO("\r\n***** DHT22 "VERSION"****\r\n");
	const char * queues[] = {DISPLAY_DIGIT_QUEUE, DISPLAY_CHAR_QUEUE, 
		FOTA_QUEUE};
	easyq_pull_all(&eq, queues, 3);
}


void ICACHE_FLASH_ATTR
easyq_connection_error_cb(void *arg) {
	EasyQSession *e = (EasyQSession*) arg;
	INFO("EASYQ: Connection error: %s:%d\r\n", e->hostname, e->port);
	INFO("EASYQ: Reconnecting to %s:%d\r\n", e->hostname, e->port);
}


void easyq_disconnect_cb(void *arg)
{
	EasyQSession *e = (EasyQSession*) arg;
	INFO("EASYQ: Disconnected from %s:%d\r\n", e->hostname, e->port);
	easyq_delete(&eq);
}


void wifi_connect_cb(uint8_t status) {
    if(status == STATION_GOT_IP) {
        easyq_connect(&eq);
    } else {
        easyq_disconnect(&eq);
    }
}


void user_init(void) {
    uart_init(BIT_RATE_115200, BIT_RATE_115200);
    os_delay_us(60000);

	EasyQError err = easyq_init(&eq, EASYQ_HOSTNAME, EASYQ_PORT, EASYQ_LOGIN);
	if (err != EASYQ_OK) {
		ERROR("EASYQ INIT ERROR: %d\r\n", err);
		return;
	}
	eq.onconnect = easyq_connect_cb;
	eq.ondisconnect = easyq_disconnect_cb;
	eq.onconnectionerror = easyq_connection_error_cb;
	eq.onmessage = easyq_message_cb;

	display_init();
    WIFI_Connect(WIFI_SSID, WIFI_PSK, wifi_connect_cb);
    INFO("System started ...\r\n");
}


void ICACHE_FLASH_ATTR user_pre_init(void)
{
    if(!system_partition_table_regist(at_partition_table, 
				sizeof(at_partition_table)/sizeof(at_partition_table[0]),
				SPI_FLASH_SIZE_MAP)) {
		FATAL("system_partition_table_regist fail\r\n");
		while(1);
	}
}


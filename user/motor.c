#include <c_types.h>
#include <eagle_soc.h> 
#include <gpio.h>
#include <driver/hw_timer.h>
#include "debug.h"
#include "io_config.h"


#define PWM_PERIOD  2000
#define PWM_DUTY	1000


#define UPDATE_CHANNELS() \
	GPIO_OUTPUT_SET(GPIO_ID_PIN(STEP_NUM), !duty)

#define CLEAR_CHANNELS() \
	GPIO_OUTPUT_SET(GPIO_ID_PIN(STEP_NUM), 0);


LOCAL bool duty;
LOCAL int32_t ticks;
LOCAL void (*stop_callback) (void);


void motor_stop() {
    ETS_FRC1_INTR_DISABLE();
    TM1_EDGE_INT_DISABLE();
    RTC_REG_WRITE(FRC1_CTRL_ADDRESS, 0);
	CLEAR_CHANNELS();
	if (stop_callback) {
		stop_callback();
	}
}


void pwm_timer_func() {
	if (ticks == 0) {
		motor_stop();
		return;
	}
	UPDATE_CHANNELS();
	duty ^= 1;
	if (duty) { 
		hw_timer_arm(PWM_DUTY);
		return;
	} 
	
	ticks += ticks > 0? -1: 1;
	hw_timer_arm(PWM_PERIOD - PWM_DUTY);
}


void motor_rotate(int32_t t) {
	if (ticks != 0) {
		ticks += t;
		return;
	}
	duty = false;
	GPIO_OUTPUT_SET(GPIO_ID_PIN(DIR_NUM), t > 0);

	uint32_t flags = DIVDED_BY_16 | FRC1_ENABLE_TIMER | TM_EDGE_INT;
    RTC_REG_WRITE(FRC1_CTRL_ADDRESS, flags);
    ETS_FRC_TIMER1_NMI_INTR_ATTACH(pwm_timer_func);
	ticks = t;
    TM1_EDGE_INT_ENABLE();
    ETS_FRC1_INTR_ENABLE();
	hw_timer_arm(PWM_PERIOD);
}


void motor_set_stop_callback(void (*done)(void)) {
	stop_callback = done;
}


void motor_init() {
    ETS_FRC1_INTR_DISABLE();
    TM1_EDGE_INT_DISABLE();
    RTC_REG_WRITE(FRC1_CTRL_ADDRESS, 0);

	ticks = 0;
	CLEAR_CHANNELS();
}




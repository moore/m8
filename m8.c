#include "ch32v003fun.h"
#include <stdlib.h>
#include "m8.h"

// GPIO D
#define BUZZER 4

// GPIO C
#define SWICH 0
#define GREEN_LED 1
#define RED_LED 2
#define BLUE_LED 3

#define LED_PWM_FREQ 10000

typedef struct led_state_t
{
	uint32_t start_ticks;
	uint16_t start_value;

	uint16_t attack_value;
	uint32_t attack_duration_ticks;

	uint16_t sustain_value;
	uint32_t sustain_duration_ticks;
	
	uint16_t release_value;
	uint32_t release_duration_ticks;
} LedState;


typedef struct state_t {
	uint32_t health;
	uint32_t last_decay;

	LedState red;
	LedState green;
	LedState blue;
} State;


uint32_t ms_to_ticks(uint32_t ms) {
	return ms * DELAY_MS_TIME;
}

uint32_t hz_to_ticks(uint32_t hz) {
	return (1000 * DELAY_MS_TIME) / hz;
}

int sample_pwm(uint32_t ticks, uint32_t hz, uint32_t duty_ticks) {
	return (ticks % hz) <= duty_ticks;
}

uint32_t compute_value(
	uint32_t now_ticks,
	uint32_t duration_ticks,
	uint32_t start_ticks,
	uint32_t start_value, 
	uint32_t attack_value
	) {
		uint32_t end_ticks = start_ticks + duration_ticks;

		if (now_ticks >= end_ticks ) {
			return attack_value;
		}

		uint16_t change = abs(((int32_t)attack_value) - (int32_t)start_value);
		float percent = ((float)(now_ticks - start_ticks))/((float)duration_ticks);
		uint32_t delta = (uint32_t)(change * percent);

		if (start_value < attack_value) {
			return start_value + delta;
		} else {
			return start_value - delta;
		}
}


int check_led(int pin, uint32_t now, LedState* led, int* done) {
	uint32_t attack_start_ticks = led->start_ticks;
	uint32_t attack_end_ticks = led->start_ticks + led->attack_duration_ticks;
	uint32_t sustain_end_ticks = attack_end_ticks + led->sustain_duration_ticks;
	uint32_t release_end_ticks = sustain_end_ticks + led->release_duration_ticks;


	if (now > release_end_ticks) {
		return OFF(pin);
	}

	*done = 0;

	if (now < attack_start_ticks) {
		return OFF(pin); 
	}

	uint32_t duty;

	if (now <= attack_end_ticks) {
		duty = compute_value(
			now,
			led->attack_duration_ticks,
			attack_start_ticks,
			led->start_value,
			led->attack_value);
	} else if (now <= sustain_end_ticks) {
		duty = compute_value(
			now,
			led->sustain_duration_ticks,
			attack_end_ticks,
			led->attack_value,
			led->sustain_value);
	} else {
		duty = compute_value(
			now,
			led->release_duration_ticks,
			sustain_end_ticks,
			led->sustain_value,
			led->release_value);
	}

	if (sample_pwm(now, LED_PWM_FREQ, duty)) {
		return ON(pin);
	} else {
		return OFF(pin);
	}
}

int update_state(State* state) {
	uint32_t now = SysTick->CNT;
	int done = 1;
	int gpio_c = check_led(RED_LED, now, &state->red, &done)
	           | check_led(GREEN_LED, now, &state->green, &done)
	           | check_led(BLUE_LED, now, &state->blue, &done);

	SET_GPIO(C, gpio_c);

	return done;
}


void set_envelope(
	LedState* led,
	uint32_t now,
	uint32_t delay_ms,
	uint32_t start_value,
	uint32_t attack_value,
	uint32_t attack_duration_ms, 
	uint32_t sustain_value,
	uint32_t sustain_duration_ms, 
	uint32_t release_value,
	uint32_t release_duration_ms
	) {
		led->start_ticks = now + ms_to_ticks(delay_ms);
		led->start_value = start_value;

		led->attack_duration_ticks = ms_to_ticks(attack_duration_ms);
		led->attack_value = attack_value;

		led->sustain_duration_ticks = ms_to_ticks(sustain_duration_ms);
		led->sustain_value = sustain_value;

		led->release_duration_ticks = ms_to_ticks(release_duration_ms);
		led->release_value = release_value;
}

int main()
{
	SystemInit();

	// This delay gives us some time to reprogram the device. 
	// Otherwise if the device enters standby mode we can't 
	// program it any more.
	Delay_Ms(5000);

	State state = {};
	state.last_decay = SysTick->CNT;

	// Enable GPIOs
	RCC->APB2PCENR |= RCC_APB2Periph_GPIOA | RCC_APB2Periph_GPIOD | RCC_APB2Periph_GPIOC;

	
	GPIO_CONFIG_OUT_PP(A, 0, 1, 2, 3, 4, 5, 6, 7);
	GPIO_CONFIG_OUT_PP(C,    RED_LED, GREEN_LED, BLUE_LED, 4, 5, 6, 7);
	GPIO_CONFIG_OUT_PP(D, 0, 1, 2, 3, BUZZER, 5, 6, 7);


	SET_GPIO(C, ON(RED_LED)|OFF(GREEN_LED)|OFF(BLUE_LED));
	Delay_Ms(250);

	SET_GPIO(C, OFF(RED_LED)|ON(GREEN_LED)|OFF(BLUE_LED));
	Delay_Ms(250);

	SET_GPIO(C, OFF(RED_LED)|OFF(GREEN_LED)|ON(BLUE_LED));
	Delay_Ms(250);

	SET_GPIO(C, OFF(RED_LED)|OFF(GREEN_LED)|OFF(BLUE_LED));



	GPIO_CONFIG_ONE_IN_PUPD(C, SWICH);
	GPIOC->BSHR |= GPIO_BSHR_BS0;


	// Set everything to low so it's not floating.
	// This is required to achieve low poser in standby.
	SET_GPIO(D, OFF(0)|OFF(1)|OFF(2)|OFF(3)|OFF(4)|OFF(5)|OFF(6)|OFF(7));
	SET_GPIO(C,        OFF(1)|OFF(2)|OFF(3)|OFF(4)|OFF(5)|OFF(6)|OFF(7));
	SET_GPIO(D, OFF(0)|OFF(1)|OFF(2)|OFF(3)|OFF(4)|OFF(5)|OFF(6)|OFF(7));

	// AFIO is needed for EXTI
	RCC->APB2PCENR |= RCC_AFIOEN;

	// assign pin 0 interrupt from portC (0b10) to EXTI channel 0
	AFIO->EXTICR |= (uint32_t)(0b10 << (2*SWICH));

	// enable line2 interrupt event
	EXTI->EVENR |= EXTI_Line0;
	EXTI->FTENR |= EXTI_Line0;

	// select standby on power-down
	PWR->CTLR |= PWR_CTLR_PDDS;

	// peripheral interrupt controller send to deep sleep
	PFIC->SCTLR |= (1 << 2);
	
	while(1)
	{
		
		uint32_t now = SysTick->CNT;
		set_envelope(&state.red,   now,   0, 100, 4000, 250, 1000, 250, 100, 50);
		set_envelope(&state.green, now, 125, 100, 8000, 125, 3000, 250, 100, 50);
		set_envelope(&state.blue,  now, 190, 100, 8000,  60, 3000, 250, 100, 50);

		int done = 0;
		while (!done)
		{
			done = update_state(&state);
		}
		
		SET_GPIO(C, OFF(RED_LED)|OFF(GREEN_LED)|OFF(BLUE_LED));

		__WFE();
		SystemInit(); // Reset clocks
	}
}


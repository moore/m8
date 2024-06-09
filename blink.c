#include "ch32v003fun.h"
#include <stdio.h>

uint32_t hz_to_ticks(uint32_t hz) {
	return (1000 * DELAY_MS_TIME) / hz;
}

int sample_square_wave(uint32_t ticks, uint32_t lambda) {
	return ((ticks/(lambda/2)) & 1) == 1;
}

int sample_pwm(uint32_t ticks, uint32_t hz, uint32_t duty_ticks) {
	return (ticks % hz) <= duty_ticks;
}

uint32_t compute_freq(
	uint32_t now,
	uint32_t duration,
	uint32_t start_time,
	uint32_t end_time, 
	uint32_t start_hz, 
	uint32_t end_hz
	) {
		if (now >= end_time ) {
			return end_hz;
		}

		uint16_t change = abs(end_hz - start_hz);
		float percent = ((float)(now - start_time))/((float)duration);
		uint32_t delta = (uint32_t)(change * percent);

		if (start_hz < end_hz) {
			return start_hz + delta;
		} else {
			return start_hz - delta;
		}
}

void drive( int pin, uint32_t start_hz, uint32_t end_hz, uint32_t duration_ms) {
	uint32_t duration_ticks = duration_ms * DELAY_MS_TIME;
	uint32_t start = SysTick->CNT;
	uint32_t end = start + duration_ticks;
	uint32_t now = start;

	while ((now - start) < (DELAY_MS_TIME * duration_ms))  {

		uint32_t hz = compute_freq(now, duration_ticks, start, end, start_hz, end_hz);
		uint32_t freq = hz_to_ticks(hz);

		if ( sample_square_wave(now, freq) ) {
			funDigitalWrite( pin, FUN_HIGH );
		} else {
			funDigitalWrite( pin, FUN_LOW );
		}

		now = SysTick->CNT;
	}

	funDigitalWrite(pin, FUN_LOW);
}


void drive_pwm( int pin, uint32_t freq,  uint32_t start_duty, uint32_t end_duty, uint32_t duration_ms) {
	uint32_t duration_ticks = duration_ms * DELAY_MS_TIME;
	uint32_t start = SysTick->CNT;
	uint32_t end = start + duration_ticks;
	uint32_t now = start;

	while ((now - start) < (DELAY_MS_TIME * duration_ms))  {

		uint32_t duty = compute_freq(now, duration_ticks, start, end, start_duty, end_duty);


		if (sample_pwm(now, freq, duty)) {
			funDigitalWrite(pin, FUN_HIGH);
		} else {
			funDigitalWrite(pin, FUN_LOW);
		}

		now = SysTick->CNT;
	}

	funDigitalWrite(pin, FUN_LOW);
}


int main()
{
	SystemInit();

	// Enable GPIOs
	funGpioInitAll();
	
    funPinMode( PC1, GPIO_Speed_10MHz | GPIO_CNF_OUT_PP );
	funPinMode( PC2, GPIO_Speed_10MHz | GPIO_CNF_OUT_PP );
    funPinMode( PC3, GPIO_Speed_10MHz | GPIO_CNF_OUT_PP );
	funPinMode( PD4, GPIO_Speed_50MHz | GPIO_CNF_OUT_PP );

	while(1)
	{
		
		drive_pwm(PC1, 10000, 100, 8000, 1000);
		drive_pwm(PC1, 10000, 8000, 100, 1000);

		drive_pwm(PC2, 10000, 100, 8000, 1000);
		drive_pwm(PC2, 10000, 8000, 100, 1000);

		drive_pwm(PC3, 10000, 100, 8000, 1000);
		drive_pwm(PC3, 10000, 9000, 100, 1000);


		//drive(PD4, 1915, 1915, 1000);
		
	}
}

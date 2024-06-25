#include "ch32v003fun.h"
#include "ch32v003_GPIO_branchless.h"

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

const uint32_t DEBOUNCE_DELAY      = 5 * DELAY_MS_TIME;
const uint32_t LONG_PRESS_DELAY    = 750 * DELAY_MS_TIME;
const uint32_t DOUBLE_CLICK_WINDOW = 500 * DELAY_MS_TIME;


typedef enum button_states_t {
	StartState,
	PressedState,
	ClickState,
	DoubleClickState,
	LongPressState,
	ReleaseState,
	ErrorState,
} ButtonState;

typedef enum button_event_t {
	Idle,
	Down,
	Up,
} ButtonEvent;

typedef enum action_t {
	None,
	Press,
	Click,
	DoubleClick,
	LongPress,
	Release,
	Error,
} Action;

typedef struct button_state_t {
	uint8_t active;
	uint32_t action_start;
	uint32_t last_change;
	uint8_t pressed;
	ButtonState state;
} Button;

typedef struct button_result_t {
	Action action;
	uint32_t start_ticks;
	uint32_t now_ticks;
	ButtonEvent event;
} ButtonResult;


typedef struct state_t {
	uint32_t health;
	uint32_t last_decay;

	Button button;

	LedState red;
	LedState green;
	LedState blue;
} State;

typedef enum pin_state_t {
	Hi,
	Low,
	Cleared,
} PinState;

#define BUFFER_LEN 10
typedef struct button_press_t {
	PinState state;
	uint32_t ticks;

} ButtonPress;

volatile ButtonPress button_buffer[BUFFER_LEN];
volatile int pointer_head = 0;
volatile int pointer_tail = 0;
volatile uint32_t button_debounce_ticks = 0;
volatile uint32_t button_last_event_ticks = 0;


typedef enum buffer_result_t {
	Ok,
	Full,
	Empty,
} BufferResult;

BufferResult write_button(uint32_t now, PinState value) {
	int next_head = (pointer_head + 1) % BUFFER_LEN;

	if (next_head == pointer_tail) {
		return Full;
	}

	button_buffer[next_head].state = value;
	button_buffer[next_head].ticks = now;

	pointer_head = next_head;	

	return Ok;
}

BufferResult read_button(ButtonPress* value) {
	if (pointer_head == pointer_tail) {
		return Empty;
	}

	*value = button_buffer[pointer_tail];

	pointer_tail = (pointer_tail + 1) % BUFFER_LEN;

	return Ok;
}


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

ButtonResult check_button(int pin, uint32_t now, Button* button) {
	ButtonState state = button->state;

	
	ButtonEvent event = Idle;

	ButtonPress pin_state;

	if (read_button(&pin_state) == Ok) {
		uint8_t button_is_pressed = (pin_state.state == Low);

		if (button_is_pressed) {
			event = Down;
		} else {
			event = Up;
		}

		button->pressed = pin_state.ticks;
		button->last_change = now;
	}	

	// BOOG this should be in the buffer
	uint32_t long_press_ticks
		= button->action_start + LONG_PRESS_DELAY;
	uint32_t double_click_ticks
		= button->action_start + DOUBLE_CLICK_WINDOW;

	if (state == StartState) {
		if (event == Down) {
			button->action_start = now; //button_last_event_ticks;
			button->state = PressedState;
			button->active = 1;
		}
		// Ignore Up/Idle
	} else if (state == PressedState) {
		if (event == Up) {
			button->state = ClickState;
		} else if (now > long_press_ticks) {
			button->state = LongPressState;
		} else if (event == Down) {
			// we really should not get here.
			button->state = ErrorState;
		}
		// Ignore idle.
	} else if (state == ClickState) {
		if (event == Down) {
			button->state = DoubleClickState;
		} else if (now > double_click_ticks) {
			button->state = ReleaseState;
		} else if (event == Up) {
			// we really should not get here.
			button->state = ErrorState;
		}
		// Ignore idle.
	}  else if (state == DoubleClickState) {
		if (event == Up) {
			button->state = ReleaseState;
		} else if (event == Down) {
			button->state = ErrorState;
		}
		// Ignore Idle
	} else if (state == LongPressState) {
		if (event == Up) {
			button->state = ReleaseState;
		} else if (event == Down) {
			button->state = ErrorState;
		}
		// Ignore Idle	
	} else if (state == ReleaseState) {
		button->state = StartState;
		button->active = 0;
	} else if (state == ErrorState) {
		button->state = StartState;
		button->active = 0;
	}

	Action action = None;

	if (button->state == state) {
		action = None;
	} else {
		if (button->state == PressedState) {
			action = Press;
		} else if (button->state == ClickState) {
			action = Click;
		} else if (button->state == DoubleClickState) {
			action = DoubleClick;
		} else if (button->state == LongPressState) {
			action = LongPress;
		} else if (button->state == ReleaseState) {
			action = Release;
		} else if (button->state == ErrorState) {
			action = Error;
		} else {
			action = None;
		}
	}

	ButtonResult result = {
		action,
		button->action_start,
		now,
		event,
	};

	return result;
}

int check_led(int pin, uint32_t now, LedState* led, int* done) {
	uint32_t attack_start_ticks = led->start_ticks;
	uint32_t attack_end_ticks = led->start_ticks + led->attack_duration_ticks;
	uint32_t sustain_end_ticks = attack_end_ticks + led->sustain_duration_ticks;
	uint32_t release_end_ticks = sustain_end_ticks + led->release_duration_ticks;


	// If we finish at 0 AKA <= 100 were done and should turn off pin
	// else just keep running the 
	if (now > release_end_ticks) {
		if (led->release_value <= 100) {
			return OFF(pin);
		}
	} else {
		*done = 0;
	}

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
	} else if (now <= release_end_ticks ) {
		duty = compute_value(
			now,
			led->release_duration_ticks,
			sustain_end_ticks,
			led->sustain_value,
			led->release_value);
	} else {
		duty = led->release_value;
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

void set_click_animation(State* state) {
	uint32_t now = SysTick->CNT;
	set_envelope(&state->red,   now,   0, 100, 4000, 250, 1000, 250, 100, 50);
	set_envelope(&state->green, now, 125, 100, 8000, 125, 3000, 250, 100, 50);
	set_envelope(&state->blue,  now, 190, 100, 8000,  60, 3000, 250, 100, 50);
}

void set_double_click_animation(State* state) {
	uint32_t now = SysTick->CNT;
	set_envelope(&state->red,   now,    0, 100, 4000, 250, 1000, 250, 100, 50);
	set_envelope(&state->green, now,  550, 100, 8000, 250, 3000, 250, 100, 50);
	set_envelope(&state->blue,  now, 1100, 100, 8000, 250, 3000, 250, 100, 50);
}

void set_long_press_animation(State* state) {
	uint32_t now = SysTick->CNT;
	set_envelope(&state->red,  now, 0, 0, 0, 0, 0, 0, 0, 0);
	set_envelope(&state->green,  now, 0, 0, 0, 0, 0, 0, 0, 0);
	set_envelope(&state->blue,  now, 0, 100, 8000, 250, 8000, 0, 8000, 0);
}

void set_error_animation(State* state) {
	uint32_t now = SysTick->CNT;
	set_envelope(&state->red,  now, 0, 100, 8000, 100, 8000, 3000, 100, 50);
}

void set_press_animation(State* state) {
	uint32_t now = SysTick->CNT;
	set_envelope(&state->red,   now, 0, 100, 4000, 10, 4000, 5, 100, 5);
	//set_envelope(&state->green, now, 0, 100, 500, 100, 500, 250, 100, 50);
	//set_envelope(&state->blue,  now, 0, 100, 500, 100, 500, 250, 100, 50);

}

void start_animation(State* state) {
uint32_t now = SysTick->CNT;
	set_envelope(&state->red,   now,    0, 100, 4000, 750, 4000, 100, 100, 250);
	set_envelope(&state->green, now,  500, 100, 8000, 750, 8000, 100, 100, 250);
	set_envelope(&state->blue,  now, 1000, 100, 8000, 500, 8000, 100, 100, 250);

	int done = 0;
	while (!done) {
		done = update_state(state);
	}

	now = SysTick->CNT;
	set_envelope(&state->red,   now, 100, 100, 4000, 250, 3000, 100, 100, 50);
	set_envelope(&state->green, now, 100, 100, 8000, 250, 6000, 100, 100, 50);
	set_envelope(&state->blue,  now, 100, 100, 8000, 250, 6000, 100, 100, 50);

	done = 0;
	while (!done) {
		done = update_state(state);
	}
}

//void EXTI0_IRQHandler(void) __attribute__((interrupt));
//void EXTI0_IRQHandler(void)
void EXTI7_0_IRQHandler(void) __attribute__((interrupt));
void EXTI7_0_IRQHandler(void)
{
	uint32_t now = SysTick->CNT;

	if ((button_debounce_ticks <= now)) {

		uint8_t button_is_pressed = !GPIO_digitalRead(GPIOv_from_PORT_PIN(GPIO_port_C, SWICH));

		button_last_event_ticks = now;

		PinState button_pin = Cleared;
		if (button_is_pressed) {
			button_pin = Hi;	
		} else {
			button_pin = Low;
		}

		if(write_button(now, button_pin) != Ok) {
			// Yell about it
			SET_GPIO(C, ON(RED_LED)|OFF(GREEN_LED)|OFF(BLUE_LED));
			Delay_Ms(50);
			SET_GPIO(C, OFF(RED_LED)|OFF(GREEN_LED)|OFF(BLUE_LED));
			Delay_Ms(50);
			SET_GPIO(C, ON(RED_LED)|OFF(GREEN_LED)|OFF(BLUE_LED));
			Delay_Ms(50);
			SET_GPIO(C, OFF(RED_LED)|OFF(GREEN_LED)|OFF(BLUE_LED));
			Delay_Ms(50);
			SET_GPIO(C, ON(RED_LED)|OFF(GREEN_LED)|OFF(BLUE_LED));
			Delay_Ms(50);
			SET_GPIO(C, OFF(RED_LED)|OFF(GREEN_LED)|OFF(BLUE_LED));
			Delay_Ms(50);
			SET_GPIO(C, ON(RED_LED)|OFF(GREEN_LED)|OFF(BLUE_LED));
			Delay_Ms(50);
			SET_GPIO(C, OFF(RED_LED)|OFF(GREEN_LED)|OFF(BLUE_LED));
		}
		
	}

	button_debounce_ticks = now + DEBOUNCE_DELAY;

	// Acknowledge the interrupt
	EXTI->INTFR = EXTI_Line0;
}

int main() {
	SystemInit();
	State state = {};
	state.last_decay = SysTick->CNT;
	state.button.last_change = SysTick->CNT;
	state.button.state = StartState;

	// Enable GPIOs
	RCC->APB2PCENR |= RCC_APB2Periph_GPIOA | RCC_APB2Periph_GPIOD | RCC_APB2Periph_GPIOC;

	
	GPIO_CONFIG_OUT_PP(A, 0, 1, 2, 3, 4, 5, 6, 7);
	GPIO_CONFIG_OUT_PP(C,    RED_LED, GREEN_LED, BLUE_LED, 4, 5, 6, 7);
	GPIO_CONFIG_OUT_PP(D, 0, 1, 2, 3, BUZZER, 5, 6, 7);

	// This delay gives us some time to reprogram the device. 
	// Otherwise if the device enters standby mode we can't 
	// program it any more.
	start_animation(&state);


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
	EXTI->EVENR  |= EXTI_Line0;
	EXTI->RTENR  |= EXTI_Line0;
	EXTI->FTENR  |= EXTI_Line0;
	EXTI->INTENR |= EXTI_Line0;


	// select standby on power-down
	PWR->CTLR |= PWR_CTLR_PDDS;

	// peripheral interrupt controller send to deep sleep
	PFIC->SCTLR |= (1 << 2);
	
	NVIC_EnableIRQ(EXTI7_0_IRQn);

	while(1)
	{
		__WFE();
		SystemInit(); // Reset clocks
		

		// Interrupt swallows the click so we fake it.
		Button button = {
			0,
			0,
			0,
			0,
			Idle,
		};
		
		int done = 0;
		while (!done || button.active) {
			uint32_t now = SysTick->CNT;

			done = update_state(&state);

			ButtonResult result = check_button(SWICH, now, &button);

			if (result.action == Press) {
				set_press_animation(&state);
			} else if (result.action == Click) {
				set_click_animation(&state);
			} else if (result.action == DoubleClick) {
				set_double_click_animation(&state);
			} else if (result.action == LongPress) {
				set_long_press_animation(&state);
				//set_error_animation(&state);
			} else if (result.action == Error) {
				set_error_animation(&state);
			} 
			// Add some sort of sleep to save power
			Delay_Us(1);
		}
		
		SET_GPIO(C, OFF(RED_LED)|OFF(GREEN_LED)|OFF(BLUE_LED));
	}
}


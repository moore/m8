#include "ch32v003fun.h"
#include "m8.h"

// GPIO D
#define BOARD_LED 2
#define BUZZER 4

// GPIO C
#define SWICH 0
#define RED_LED 1
#define GREEN_LED 2
#define BLUE_LED 3

typedef struct state_t {
	uint32_t health;
	uint32_t last_decay;
} State;

int main()
{
	SystemInit();

	// This delay gives us some time to reprogram the device. 
	// Otherwise if the device enters standby mode we can't 
	// program it any more.
	Delay_Ms(5000);

	State state = {0, SysTick->CNT};

	// Enable GPIOs
	RCC->APB2PCENR |= RCC_APB2Periph_GPIOA | RCC_APB2Periph_GPIOD | RCC_APB2Periph_GPIOC;

	
	GPIO_CONFIG_OUT_PP(A, 0, 1, 2, 3, 4, 5, 6, 7);
	GPIO_CONFIG_OUT_PP(C,    RED_LED, GREEN_LED, BLUE_LED, 4, 5, 6, 7);
	GPIO_CONFIG_OUT_PP(D, 0, 1, 2, 3, BUZZER, 5, 6, 7);


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

		SET_GPIO(D, ON(BOARD_LED));

		SET_GPIO(C, ON(RED_LED)|OFF(GREEN_LED)|OFF(BLUE_LED));
		Delay_Ms(250);

		SET_GPIO(C, OFF(RED_LED)|ON(GREEN_LED)|OFF(BLUE_LED));
		Delay_Ms(250);

		SET_GPIO(C, OFF(RED_LED)|OFF(GREEN_LED)|ON(BLUE_LED));
		Delay_Ms(250);

		//SET_GPIO(D, OFF(BOARD_LED));
		SET_GPIO(C, OFF(RED_LED)|OFF(GREEN_LED)|OFF(BLUE_LED));

		__WFE();
		SystemInit(); // Reset clocks
	}
}


#include "ch32v003fun.h"
#include "m8.h"

// GPIO D
#define BOARD_LED 2
#define BUZZER 4

// GPIO C
#define RED_LED 1
#define GREEN_LED 2
#define BLUE_LED 3

int main()
{
	SystemInit();

	// Enable GPIOs
	RCC->APB2PCENR |= RCC_APB2Periph_GPIOD | RCC_APB2Periph_GPIOC;

	GPIO_CONFIG_OUT_PP(C, RED_LED, GREEN_LED, BLUE_LED);
	GPIO_CONFIG_OUT_PP(D, BOARD_LED, BUZZER);


	while(1)
	{
		SET_GPIO(D, ON(BOARD_LED));

		SET_GPIO(C, ON(RED_LED)|OFF(GREEN_LED)|OFF(BLUE_LED));
		Delay_Ms(250);

		SET_GPIO(C, OFF(RED_LED)|ON(GREEN_LED)|OFF(BLUE_LED));
		Delay_Ms(250);

		SET_GPIO(C, OFF(RED_LED)|OFF(GREEN_LED)|ON(BLUE_LED));
		Delay_Ms(250);

		SET_GPIO(D, OFF(BOARD_LED));
		SET_GPIO(C, OFF(RED_LED)|OFF(GREEN_LED)|OFF(BLUE_LED));

		Delay_Ms(1000);
	}
}


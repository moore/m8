#include "ch32v003fun.h"

#define GPIO_CONFIG_CLEAR(GROUP, PIN) \
    GPIO##GROUP->CFGLR &= ~(0xf << (4 * PIN));

#define GPIO_CONFIG_SET_OUT_PP(GROUP, PIN) \
    GPIO##GROUP->CFGLR |= (GPIO_Speed_10MHz | GPIO_CNF_OUT_PP) << (4 * PIN);

#define GPIO_CONFIG_ONE_OUT_PP(GROUP, PIN) \
	GPIO_CONFIG_CLEAR(GROUP, PIN) \
	GPIO_CONFIG_SET_OUT_PP(GROUP, PIN)

#define GPIO_CONFIG_OUT_PP(GROUP, ...) \
    do { \
        int pins[] = {__VA_ARGS__}; \
        for (int i = 0; i < sizeof(pins)/sizeof(pins[0]); ++i) { \
            GPIO_CONFIG_ONE_OUT_PP(GROUP, pins[i]); \
        } \
    } while (0)


#define ON(PIN) \
	(1<<PIN)

#define OFF(PIN) \
	(1<<(16+PIN))

#define SET_GPIO(GROUP, PINS) \
	GPIO##GROUP->BSHR = PINS

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
		Delay_Ms(250);

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


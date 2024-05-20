
#define GPIO_CONFIG_CLEAR(GROUP, PIN) \
    GPIO##GROUP->CFGLR &= ~(0xf << (4 * PIN));

#define GPIO_CONFIG_SET_OUT_PP(GROUP, PIN) \
    GPIO##GROUP->CFGLR |= (GPIO_Speed_10MHz | GPIO_CNF_OUT_PP) << (4 * PIN);

#define GPIO_CONFIG_SET_IN_PUPD(GROUP, PIN) \
	GPIO##GROUP->CFGLR |= (GPIO_CNF_IN_PUPD<<(4*PIN))

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

#define GPIO_CONFIG_ONE_IN_PUPD(GROUP, PIN) \
	GPIO_CONFIG_CLEAR(GROUP, PIN) \
	GPIO_CONFIG_SET_IN_PUPD(GROUP, PIN)


#define ON(PIN) \
	(1<<PIN)

#define OFF(PIN) \
	(1<<(16+PIN))

#define SET_GPIO(GROUP, PINS) \
	GPIO##GROUP->BSHR = PINS

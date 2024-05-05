import time
import board
from digitalio import DigitalInOut, Direction, Pull
from rainbowio import colorwheel
import neopixel
import asyncio
import async_button


# setup onboard NeoPixel
pixel_pin = board.A3
num_pixels = 1

pixels = neopixel.NeoPixel(pixel_pin, num_pixels, brightness=0.07, auto_write=True)


class Mode:
    START = 1
    LISTEN_MALE = 2

class State:
    mode = Mode.START



# rainbow cycle function
async def rainbow_cycle(wait):
    n = 6
    for j in range(6):
        pixels[0] = colorwheel(j * (255//n))
        pixels.show()
        await asyncio.sleep(wait)


async def main():  # Don't forget the async!
    #led_task = asyncio.create_task(rainbow_cycle(0.1))
    #await asyncio.gather(led_task)  # Don't forget the await!
    pixels.brightness = 0.07
    await rainbow_cycle(0.1)
    pixels.brightness = 0


    button = async_button.Button(board.A2, False)

    while True:
        click = await button.wait(button.ALL_EVENTS)
        print(click)
        if button.DOUBLE in click:
            pixels.brightness = 0.07
            await rainbow_cycle(0.1)
            pixels.brightness = 0.00


asyncio.run(main())

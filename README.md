# UnicornHat

Messing around with a [Pimoroni Unicorn HAT HD](https://shop.pimoroni.com/products/unicorn-hat-hd)

It's a 16x16 matrix of RGB LEDs, but helpfully Pimoroni have done the job of wiring up a bunch
of driver ICs and even a handy little ARM core to wrangle those, so all we have to do is send
a single command over SPI:

`0x72 [buffer]`

where `buffer` is a 768-byte buffer representing the display pixels as RGB byte triples (from
top-left to bottom-right, I think).

They provide a Python library to work with it; the code here is in C.

# License

All code in this repository is MIT Licensed unless otherwise specified. See [LICENSE](LICENSE).

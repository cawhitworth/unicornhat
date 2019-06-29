#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdint.h>
#include <stdarg.h>
#include <stdbool.h>
#include <fcntl.h>
#include <stdio.h>
#include <errno.h>
#include <time.h>
#include <signal.h>

#include <sys/ioctl.h>
#include <linux/spi/spidev.h>

#define DISPLAY_WIDTH   16
#define DISPLAY_HEIGHT  16
#define BYTES_PER_PIXEL  3
#define DISPLAY_SIZE    (DISPLAY_WIDTH * DISPLAY_WIDTH * BYTES_PER_PIXEL)

static const char *devicePath = "/dev/spidev0.0";

static bool exitRequested = false;
static uint8_t display[DISPLAY_SIZE];
static int deviceFd;

static void Error_Fatal(const char *s, ...);
static void SPI_Xfer(int fd, uint8_t* txBuffer, uint8_t* rxBuffer, uint32_t length,
                     uint16_t delayUs, uint32_t speedHz, uint8_t bitsPerWord);

#define CHECK_PARAM(param, max) \
    if (param > max) {\
        Error_Fatal("Parameter '%s' should not be greater than %d\n", #param, max);\
    }

static void Display_SetPixel(unsigned x, unsigned y, unsigned r, unsigned g, unsigned b)
{
    CHECK_PARAM(x, DISPLAY_WIDTH)
    CHECK_PARAM(y, DISPLAY_HEIGHT)
    CHECK_PARAM(r, 0xff)
    CHECK_PARAM(g, 0xff)
    CHECK_PARAM(b, 0xff)

    unsigned base = BYTES_PER_PIXEL * ((y * DISPLAY_WIDTH) + x);
    display[base] = r;
    display[base+1] = g;
    display[base+2] = b;
}

static void Display_SetAll(unsigned r, unsigned g, unsigned b)
{
    CHECK_PARAM(r, 0xff)
    CHECK_PARAM(g, 0xff)
    CHECK_PARAM(b, 0xff)

    for(unsigned x = 0; x<DISPLAY_WIDTH; x++) {
        for(unsigned y = 0; y<DISPLAY_HEIGHT; y++) {
            Display_SetPixel(x,y,r,g,b);
        }
    }
}

static void Display_Update()
{
    static const uint8_t bitsPerWord = 8;
    static const uint32_t speedHz = 9000000;
    static const uint16_t delayUs = 0;
    static const uint32_t bufferLen = DISPLAY_SIZE + 1;
    uint8_t txBuffer[bufferLen]; // Should really avoid this allocation :(
    uint8_t rxBuffer[bufferLen];

    rxBuffer[0] = 0;
    txBuffer[0] = 0x72;
    memcpy(&txBuffer[1], display, sizeof(display));

    SPI_Xfer(deviceFd, txBuffer, rxBuffer, bufferLen, delayUs, speedHz, bitsPerWord);
}

static void SPI_Xfer(int fd, uint8_t* txBuffer, uint8_t* rxBuffer, uint32_t length,
                     uint16_t delayUs, uint32_t speedHz, uint8_t bitsPerWord)
{
    struct spi_ioc_transfer xfer = {
        .tx_buf = (uint32_t)txBuffer,
        .rx_buf = (uint32_t)rxBuffer,
        .len = length,
        .delay_usecs = delayUs,
        .speed_hz = speedHz,
        .bits_per_word = bitsPerWord,
    };

    int ret = ioctl(fd, SPI_IOC_MESSAGE(1), &xfer);

    if (ret < 1) {
        Error_Fatal("Failed to send SPI transfer: %s (%d).\n", strerror(errno), errno);
    }
}

static void Error_Fatal(const char *s, ...)
{
    va_list args;
    va_start(args, s);
    vfprintf(stderr, s, args);
    va_end(args);
    abort();
}

static void Signal_HandleInt(int signal)
{
    exitRequested = true;
}

int main(int argc, char *argv[])
{
    struct sigaction sa;
    sa.sa_handler = &Signal_HandleInt;

    if (sigaction(SIGINT, &sa, NULL) == -1) {
        Error_Fatal("Failed to install SIGINT handler: %s (%d)\n", strerror(errno), errno);
    }

    deviceFd = open(devicePath, O_RDWR);

    if (deviceFd < 0) {
        Error_Fatal("Failed to open SPI device: %s - %s (%d)\n", devicePath, strerror(errno), errno);
    }

    uint8_t brightness = 0;
    uint8_t increase = -1;

    struct timespec sleepTime = {
        .tv_sec = 0,
        .tv_nsec = 5 * 1000 * 1000
    };

    while(!exitRequested) {
        Display_SetAll(brightness, brightness, brightness);
        Display_Update();

        if (brightness == 0 || brightness == 0xff) {
            increase = -increase;
        }
        brightness += increase;

        nanosleep(&sleepTime, NULL);
    }

    if (deviceFd) {
        // Sleep to give any interrupted transactions time to complete

        nanosleep(&sleepTime, NULL);

        // Blank the display
        Display_SetAll(0, 0, 0);
        Display_Update();

        // And close the device
        close(deviceFd);
    }

    return 0;
}

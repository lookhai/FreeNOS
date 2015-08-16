/*
 * Copyright (C) 2015 Niek Linnenbank
 * Copyright (C) 2013 Goswin von Brederlow <goswin-v-b@web.de>
 * 
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <FreeNOS/System.h>
#include <FreeNOS/API.h>
#include "RaspiSerial.h"

#warning The I/O base is mapped as cachable and not as Device!

RaspiSerial::RaspiSerial()
{
    init();
}

void RaspiSerial::init()
{
    // Disable PL011.
    IO::write(PL011_CR, 0x00000000);

    // Setup the GPIO pin 14 && 15.
    // Disable pull up/down for all GPIO pins & delay for 150 cycles.
    IO::write(GPPUD, 0x00000000);
    delay(150);

    // Disable pull up/down for pin 14,15 & delay for 150 cycles.
    IO::write(GPPUDCLK0, (1 << 14) | (1 << 15));
    delay(150);

    // Write 0 to GPPUDCLK0 to make it take effect.
    IO::write(GPPUDCLK0, 0x00000000);

    // Clear pending interrupts.
    IO::write(PL011_ICR, 0x7FF);

    // Set integer & fractional part of baud rate.
    // Divider = UART_CLOCK/(16 * Baud)
    // Fraction part register = (Fractional part * 64) + 0.5
    // UART_CLOCK = 3000000; Baud = 115200.

    // Divider = 3000000/(16 * 115200) = 1.627 = ~1.
    // Fractional part register = (.627 * 64) + 0.5 = 40.6 = ~40.
    IO::write(PL011_IBRD, 1);
    IO::write(PL011_FBRD, 40);

    // Disable FIFO, use 8 bit data transmission, 1 stop bit, no parity
    IO::write(PL011_LCRH, PL011_LCRH_WLEN_8BIT);

    // Enable Rx/Tx interrupts
    IO::write(PL011_IMSC,
         PL011_IMSC_RXIM | PL011_IMSC_TXIM);

    // Enable PL011, receive & transfer part of UART.
    IO::write(PL011_CR, (1 << 0) | (1 << 8) | (1 << 9));
}

void RaspiSerial::put(u8 byte)
{
    // Temporary disable interrupts
    u32 imsc = IO::read(PL011_IMSC);
    IO::write(PL011_IMSC, 0);

    // wait for UART to become ready to transmit
    while(true)
        if (!(IO::read(PL011_FR) & (1 << 5)))
            break;

    IO::write(PL011_DR, byte);

    // wait for UART to empty the transmit queue
    while(true)
        if (!(IO::read(PL011_FR) & (1 << 5)))
            break;

    // Restore interrupts
    IO::write(PL011_IMSC, imsc);
}

u8 RaspiSerial::get(void)
{
    // wait for UART to have recieved something
    while(true)
    {
        if (!(IO::read(PL011_FR) & (1 << 4)))
        {
            break;
        }
    }
    return IO::read(PL011_DR);
}

void RaspiSerial::write(const char *str)
{
    while(*str)
    {
        RaspiSerial::put(*str++);
    }
}

void RaspiSerial::delay(s32 count)
{
    asm volatile("1: subs %[count], %[count], #1; bne 1b"
         : : [count]"r"(count));
}

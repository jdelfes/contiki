/*
 * Copyright (c) 2013, Instituto Nokia de Tecnologia - INdT
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the Institute nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE INSTITUTE AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE INSTITUTE OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * This file is part of the Contiki operating system.
 */

/**
 * \file
 *   Implementation of Contiki's clock module for CC2540 SoC.
 *
 *   Literature Number: SWRU191D. April 2009–Revised March 2013.
 *
 * \author
 *   Andre Guedes <andre.guedes@openbossa.org>
 */

#include "contiki-conf.h"

#include "cc2540.h"
#include "sys/etimer.h"

#define JIFFY (1000 / CLOCK_CONF_SECOND)

#define SLEEP_TIMER_CLOCK (32U * 1024U) /* 32 kHz */

/* This macro calculates the value which should be added to sleep timer in
 * order to generate 128 ticks per second (CLOCK_CONF_SECOND).
 */
#define SLEEP_TIMER_VALUE (SLEEP_TIMER_CLOCK / CLOCK_CONF_SECOND)

/* Number of ticks since chip reset */
static volatile clock_time_t ticks = 0;

void
init_system_clock(void)
{
  /* Select 32 MHz XOSC system clock source.
   *
   * According to CC2540 user guide (see reference in file header), operation
   * of the RF transceiver requires 32-MHz crystal oscillator.
   */
  CLKCONCMD &= ~BIT(6);

  /* Set system clock frequency to 32 MHz */
  CLKCONCMD &= ~(BIT(2) | BIT(1) | BIT(0));

  /* Wait until changes take effect */
  while(CLKCONSTA != CLKCONCMD);
}

void
update_timer_value(void)
{
  uint32_t value;

  /* Read sleep timer value */
  value = ST0;
  value |= ((uint32_t) ST1) << 8;
  value |= ((uint32_t) ST2) << 16;

  /* Update sleep timer value */
  value += SLEEP_TIMER_VALUE;

  /* Wait LDRDY bit in STLOAD register be set back to 1 in order to write into
   * ST2:ST1:ST0. This bit is likely to be already set to 1, but in order to
   * be strickly compliant with the SoC spec we should wait.
   */
  while (STLOAD == 0);

  /* Write sleep timer compare value */
  ST2 = (uint8_t) (value >> 16);
  ST1 = (uint8_t) (value >> 8);
  ST0 = (uint8_t) value;
}

void
init_sleep_timer(void)
{
  update_timer_value();

  /* Enable sleep timer interrupt */
  STIE = 1;
}

void
sleep_timer_isr(void) __interrupt(ST_IRQ)
{
  ticks++;

  update_timer_value();

  if (etimer_pending() && etimer_next_expiration_time() <= ticks) {
    etimer_request_poll();
  }

  /* Clear sleep timer interrupt flag */
  STIF = 0;
}

void
clock_init(void)
{
  init_system_clock();
  init_sleep_timer();
}


CCIF clock_time_t
clock_time(void)
{
  return ticks;
}

CCIF unsigned long
clock_seconds(void)
{
  return ticks / CLOCK_CONF_SECOND;
}

/* The documentation in core/sys/clock.h is not clear about the "delay"
 * argument. We assume it is in milliseconds.
 */
void
clock_delay(unsigned int delay)
{
  clock_time_t expire_ticks = ticks + (delay/JIFFY);

  while (ticks < expire_ticks);
}

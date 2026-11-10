#ifndef REMOTE_BOARD_H
#define REMOTE_BOARD_H

/* CH583M QFN48: checked against CH583DS1_en.PDF and the KiCad PCB.
 * SW1 PA13/pad45, SW3 PB4/pad20, SW4 PB2/pad22, SW5 PB1/pad23,
 * SW6 PB3/pad21 on every revision. Active low. SW2 depends on BOARD_REV:
 *   rev1 (fabricated 2026-10-28): SW2 PB0/pad24; BOOT/RESET on jumpers JP2/JP1.
 *   rev2 (current PCB): SW2 PB22/pad26, the BOOT pin, so holding SW2 at
 *        power-up enters the WCH bootloader. JP1/JP2 removed.
 * Select with -DBOARD_REV=1 or 2 (tools/build.py --board rev1|rev2). */
#ifndef BOARD_REV
#define BOARD_REV 2
#endif
#define BOARD_BUTTON_A_MASK (1UL << 13)
#if BOARD_REV == 1
#define BOARD_SW2_PB_PIN 0U
/* GPIO port B pins: PB0-PB4. Interrupt bits are the same pins. */
#define BOARD_BUTTON_B_MASK 0x1FUL
#define BOARD_BUTTON_B_INT_MASK 0x1FUL
#define BOARD_SW2_INT_REMAP 0
#elif BOARD_REV == 2
#define BOARD_SW2_PB_PIN 22U
/* GPIO port B pins (R32_PB_* registers): PB22 and PB1-PB4. */
#define BOARD_BUTTON_B_MASK ((1UL << 22) | 0x1EUL)
/* Same pins in the 16-bit R16_PB_INT_* registers. PB22 uses interrupt bit 8
 * once RB_PIN_INTX remaps INT24 from PB8 (unconnected) to PB22. */
#define BOARD_BUTTON_B_INT_MASK ((1UL << 8) | 0x1EUL)
#define BOARD_SW2_INT_REMAP 1
#else
#error "BOARD_REV must be 1 or 2"
#endif
#define BOARD_DCDC_INDUCTOR_UH 22
/* User-confirmed Y1: YXC X201632MMB4SI, CL=10pF; C6/C7 not fitted.
 * Reported 4pF stray is an estimate, not a measured tuning parameter.
 * Keep SDK oscillator tuning unchanged while testing the HID latency fix. */
/* Preserve PB10/11 USB, PB14/15 debug, PB22/23 BOOT/reset (PB22 is SW2 on rev2),
 * PB8 (its interrupt is remapped on rev2),
 * PA10/11 possible 32k pins, and all RF/clock/power pins. */
#endif

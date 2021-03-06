#ifndef __CONTROLS_H__
#define __CONTROLS_H__

extern void setup_start_io(void);
extern void setup_sleep_io(void);

#define bit_pos(A) \
((A) == 1       ? 0 \
: (A) == (1 << 1) ? 1 \
: (A) == (1 << 2) ? 2 \
: (A) == (1 << 3) ? 3 \
: (A) == (1 << 4) ? 4 \
: (A) == (1 << 5) ? 5 \
: (A) == (1 << 6) ? 6 \
: (A) == (1 << 7) ? 7 \
:  -1)


typedef enum
{
	OFF_POS = 0,
	ON_POS = 1,
	ZERO_POS = 2
} POWER_POS;

typedef enum
{
	UP_POS = 0,
	DOWN_POS = 1
} BUTTON_POS;

#define MAX_NUM_POS (13)	// positions 1-13 are regular slots
#define HQ_TIME_POS (14)	// T1 - Havequick time (RS-232/RS-485)
#define SG_TIME_POS (15)	// T2 - SINCGARS time (DS-102)
#define PC_POS 		(16)	// A - PC Connection via RS-232

extern byte switch_pos;
extern byte prev_switch_pos;

extern byte power_pos;
extern byte prev_power_pos;

// Functions to get external controls states
extern byte get_switch_state(void);	// 1-13,T1,T2,A 
extern byte get_power_state(void);
extern byte get_button_state(void);

// Control PIN A connection - Ground or +6V
// If pin A is GND, then all other pins are 0 - +5V
// If pin A is +6V, then all other pins are -5V - 0V
extern void set_pin_a_as_gnd(void);
extern void set_pin_a_as_power(void);

// Control PIN F connection - +6V or IO
extern void set_pin_f_as_io(void);
extern void set_pin_f_as_power(void);
//
// Possible modes:
// DS102, when pin_A is +6V, and all other pins are in negative logic
// RS232 or RS485 mode, when pin_A is GND, pin_F supplies power, and all pin_B-pin_E are positive logic
//

// Functions to read pins using ADC 
//  and to assign HIGH or LOW if it crosses the threshold
extern char pin_B(void);
extern char pin_C(void);
extern char pin_D(void);
extern char pin_E(void);
extern char pin_F(void);

extern char is_bootloader_active(void);
extern void BootloadMode(void);

// Enable/disable HQII timestream
extern void enable_tx_hqii(void);
extern void disable_tx_hqii(void);
//-------------------------------------------------
//  LED support fucntions
//------------------------------------------------
extern void set_led_state(char on_time, char off_time);
extern void set_led_on(void);
extern void set_led_off(void);

#endif	// __CONTROLS_H__
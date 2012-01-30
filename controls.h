#ifndef __CONTROLS_H__
#define __CONTROLS_H__

extern void setup_start_io(void);
extern void setup_sleep_io(void);

#define bit_pos(A) \
((A) == 1       ? 0 \
: (A) == 1 << 1 ? 1 \
: (A) == 1 << 2 ? 2 \
: (A) == 1 << 3 ? 3 \
: (A) == 1 << 4 ? 4 \
: (A) == 1 << 5 ? 5 \
: (A) == 1 << 6 ? 6 \
: (A) == 1 << 7 ? 7 \
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


extern byte switch_pos;
extern byte prev_switch_pos;

extern byte power_pos;
extern byte prev_power_pos;


extern byte get_switch_state(void);
extern byte get_power_state(void);
extern byte get_button_state(void);

extern void set_pin_a_as_gnd(void);
extern void set_pin_a_as_power(void);
extern void set_pin_f_as_io(void);
extern void set_pin_f_as_power(void);

extern char is_bootloader_active(void);

extern void BootloadMode(void);

//-------------------------------------------------
//  LED support fucntions
//------------------------------------------------
extern void set_led_state(char on_time, char off_time);

#endif	// __CONTROLS_H__
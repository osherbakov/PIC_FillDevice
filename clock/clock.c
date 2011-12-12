#include "config.h"
#include "clock.h"


static char MSB[NUM_SAMPLES];
static int LSB[NUM_SAMPLES];
static char samples_number;
volatile char curr_msb;	// MSB for the current sample (8 bits)
volatile int curr_lsb;	// LSB for the current sample (16 bits)

// Initialize all clock correction data
void InitClockData(void)
{
	int i;
	samples_number = -1;	// Initial value to force skipping of the first sample
	for(i = 0; i < NUM_SAMPLES; i++)
	{
		LSB[i] = 0;
		MSB[i] = 0;
	}
	curr_msb = 0;
	curr_lsb = 0;
}

void UpdateClockData()
{
	char i;
	if(samples_number < 0)
	{
		samples_number = 0;
		return;
	}
	if(samples_number < NUM_SAMPLES)
	{
		LSB[samples_number] = curr_lsb;
		MSB[samples_number] = curr_msb;
		samples_number++;
	}else
	{
		/// Shift all samples 1 position to the left
		for(i = 1; i < (NUM_SAMPLES - 1); i++)
		{
			LSB[i-1] = LSB[i];
			MSB[i-1] = MSB[i];
		}
		// Add latest sample
		LSB[NUM_SAMPLES - 1] = curr_lsb;
		MSB[NUM_SAMPLES - 1] = curr_msb;
	}
	curr_lsb = 0;
	curr_msb = 0;
}


void ProcessClockData()
{
	char	num_above, num_below;
	char i;

  // Not enough samples - exit
	if(samples_number < NUM_SAMPLES)
	{
		return;
	}

	// Calculate how many samples are above and below the ideal number
	num_above = 0;
	num_below = 0;
	for(i = 1; i < NUM_SAMPLES ; i++)
	{
		// First chack MSBs only
		if(MSB[i] > (ONE_SEC_TICKS >> 16))
		{
			num_above++;
		}else if (MSB[i] < (ONE_SEC_TICKS >> 16))
		{
			num_below++;
		}else
		{
			// MSB are the same - chack LSBs
			if(LSB[i] > (ONE_SEC_TICKS & 0xFFFFL))
			{
				num_above++;
			}else if(LSB[i] < (ONE_SEC_TICKS & 0xFFFFL))
			{
				num_below++;
			}
		}
	}
	// Make sure that the difference is bigger than the margin, and only then adjust
	if(num_above > (num_below + NUM_MARGIN))
	{
		// Too fast - reduce the internal osc freq
		OSCTUNEbits.TUN--;
	}else if(num_below > (num_above + NUM_MARGIN))
	{
		// Too slow - reduce the internal osc freq
		OSCTUNEbits.TUN++;
	}
}

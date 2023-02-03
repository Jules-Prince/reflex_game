#include <stdio.h>
#include <unistd.h>
#include <time.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/alt_timestamp.h>
#include <signal.h>
#include "system.h"
#include "altera_avalon_pio_regs.h"
#include "altera_avalon_timer_regs.h"

#define HEX_VALUE_NONE 0b1111111111
#define HEX_VALUE_0    0b1111000000
#define HEX_VALUE_1    0b1111111001
#define HEX_VALUE_2    0b1110100100
#define HEX_VALUE_3    0b1110110000
#define HEX_VALUE_4    0b1110011001
#define HEX_VALUE_5    0b1110010010
#define HEX_VALUE_6    0b1110000010
#define HEX_VALUE_7    0b1111111000
#define HEX_VALUE_8    0b1111111001
#define HEX_VALUE_9    0b1110010000

#define BUTTON_1 1
#define BUTTON_2 2
#define BUTTON_3 4
#define BUTTON_4 8

#define SW_0 0b0000000001
#define SW_1 0b0000000010
#define SW_2 0b0000000100
#define SW_3 0b0000001000
#define SW_4 0b0000010000
#define SW_5 0b0000100000
#define SW_6 0b0001000000
#define SW_7 0b0010000000
#define SW_8 0b1110000000
#define SW_9 0b1000000000

#define TIME_RANGE_MIN 500
#define TIME_RANGE_MAX 2500

volatile int edge_capture; // valeur bouton

int nbGame = 0;
int averageTime_inMs = 0;
int sum = 0;

// TODO: remplacer int par alt_u32
static void handle_button_interrupts(void *context, int id)
{
// Cast context to edge_capture's type. It is important that this
// be declared volatile to avoid unwanted compiler optimization.
	volatile int *edge_capture_ptr = (volatile int *)context;
// Read the edge capture register on the button PIO. Store value.
	edge_capture_ptr =
	IORD_ALTERA_AVALON_PIO_EDGE_CAP(KEY_BASE);
// Write to the edge capture register to reset it.
	IOWR_ALTERA_AVALON_PIO_EDGE_CAP(KEY_BASE, 0);
	/ Read the PIO to delay ISR exit. This is done to prevent a
	spurious interrupt in systems with high processor -> pio
latency and fast interrupts.*/
	IORD_ALTERA_AVALON_PIO_EDGE_CAP(KEY_BASE);
}

static void init_button_pio()
{
// Recast the edge_capture pointer to match the alt_irq_register() function
//prototype.
	void *edge_capture_ptr = (void *)&edge_capture;
// Enable all 4 button interrupts.
	IOWR_ALTERA_AVALON_PIO_IRQ_MASK(KEY_BASE, 0xf);
// Reset the edge capture register.
	IOWR_ALTERA_AVALON_PIO_EDGE_CAP(KEY_BASE, 0x0);
// Register the ISR.
	alt_irq_register(KEY_IRQ, edge_capture_ptr, handle_button_interrupts);
}

void init()
{
	srand(time(NULL));
	printOnDisplay(HEX0_BASE, 10);
	printOnDisplay(HEX1_BASE, 10);
	printOnDisplay(HEX2_BASE, 10);
	printOnDisplay(HEX3_BASE, 10);
	printOnDisplay(HEX4_BASE, 10);
	printOnDisplay(HEX5_BASE, 10);

	init_button_pio();
	alt_timestamp_start();

IOWR_ALTERA_AVALON_TIMER_PERIODL(TIMER_BASE, 0);	  // 16 bits de poids faible
IOWR_ALTERA_AVALON_TIMER_PERIODH(TIMER_BASE, 0xFFFF); // 16 bits de poids fort
IOWR_ALTERA_AVALON_TIMER_CONTROL(TIMER_BASE, ALTERA_AVALON_TIMER_CONTROL_START_MSK);
IOWR_ALTERA_AVALON_TIMER_SNAPL(TIMER_BASE, 0x01);
}

void printOnDisplay(int hexAddress, int value)
{
	int lookUpTable[11] = {HEX_VALUE_0, HEX_VALUE_1, HEX_VALUE_2,
	HEX_VALUE_3, HEX_VALUE_4, HEX_VALUE_5, HEX_VALUE_6,
	HEX_VALUE_7, HEX_VALUE_8, HEX_VALUE_9, HEX_VALUE_NONE};
	printf("%d", value);
	IOWR_ALTERA_AVALON_PIO_DATA(hexAddress, lookUpTable[value]);
}

void printTimeOnDisplay(int time)
{
	int second = time / 1000;
	int milli = time % 1000;
	printOnDisplay(HEX5_BASE, second / 10);
	printOnDisplay(HEX4_BASE, second % 10);
	printf(".");
	printOnDisplay(HEX3_BASE, milli / 100);
	printOnDisplay(HEX2_BASE, (milli % 100) / 10);
	printOnDisplay(HEX1_BASE, milli % 10);
	printf("\n");
}

void determineAverage(int newValue)
{
	nbGame++;
	sum += newValue;
	averageTime_inMs = sum / nbGame;
}

int whatTimeIsIt_inMs(){
	return alt_timestamp()/(alt_timestamp_freq()/1000);
}

void newRound()
{
	IOWR_ALTERA_AVALON_PIO_DATA(LEDS_BASE, 0x3FF);
	int randTime = (rand() % (TIME_RANGE_MAX - TIME_RANGE_MIN + 1)) + TIME_RANGE_MIN;
	printf("rand time : %d\n", rand());
	float start = whatTimeIsIt_inMs();
	printf("¨start : %f\n", start);
	while (whatTimeIsIt_inMs()-start < randTime)
	{
	}
	printf("PRESS SPACE BAR NOWWWW\\n");
	IOWR_ALTERA_AVALON_PIO_DATA(LEDS_BASE, 0);
	start = whatTimeIsIt_inMs();
	edge_capture=0;
	while (1)
	{
		if(edge_capture==BUTTON_4)
			break;
	}
	double dif = (whatTimeIsIt_inMs() - start);
	printf("Well done your time was %lf\\n\\n", dif);
	printTimeOnDisplay(dif);
	determineAverage(dif);
}

void startGame()
{
	printf("-------REFLEX GAME-------\n\n\n");
	printf("Press 1 or 2 !\n");

	while (1)
	{


		switch (edge_capture)
		{
		case BUTTON_1:
		// startGame
			printf("New ROUND\\n");
			newRound();
			break;
		case BUTTON_2:
		// display average
			printf("The average time is : %d\\n\\n", averageTime_inMs);
			printTimeOnDisplay(averageTime_inMs);
			edge_capture=0;
			break;
		default:

			break;
		}
	}

}

int main()
{
	init();
	startGame();
}
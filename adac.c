//  adac.c
//
//  Created by Kristoffer Kjærnes on 21.02.2017.
//  Inspiration taken from the SPI example by Mike McCauley, author of the bcm2835 library
/*
bcm2835 library documentation on http://www.airspayce.com/mikem/bcm2835/modules.html

After installing bcm2835, you can build this with something like:
gcc -o adac adac.c -l bcm2835 -lrt -lm
where -l bcm2835 links to the library header file, -lrt to the realtime timer library and -lm to math.h
Then you run the executable with:
sudo ./adac
Alternatively, if you want better process priority in Linux, use:
sudo nice -n -20 ./adac
to get the highest priority available. 
If you want to reserve one of the cpu cores for this program only, see the labmanual for how 
to do so. You can then use the linux command taskset to run it on the reserved core. Remember 
that even if you do this, interrupts may still interfere with the program.

Add -lrt flag to the compile command if any bcm2835_delay... function is used!

Tutorials and reference for C-programming are wonderfully provided on
http://www.cprogramming.com/ and/or http://www.cplusplus.com/
*/

#include <bcm2835.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>

#define PI 3.14159265

/* Define global variables */
uint64_t systemTime = 0;     // Var to hold system time at last check
uint32_t elapsedTime = 0;    // Var to hold time elapsed since start/first check
uint32_t timeDifference = 0; // Var to hold time between two last checks

/* Function declarations (implemented below main program) */
void chipSelect(uint8_t chip);
void spiSetup();
void timerFunction(uint64_t *lastSysTime, uint64_t *newSysTime, uint32_t *elapTime, uint32_t *timeDiff);
void sawtooth(uint32_t timeDiff, double sigFreq, uint16_t *dacVal);
void linChirp(double startFreq, double endFreq, double duration, uint32_t elapTime, uint16_t *dacVal);

/* Main program starts */
int main(int argc, char **argv){
    /* Assign sampling specific variables */
    uint32_t samples = 10000; // Take this many samples before ending
    uint32_t adcLen = 3;       // We need a buffer of 3 bytes per sample per ADC channel.
    uint32_t dacLen = 2;       // We need a buffer of 2 bytes per DAC sequence.
    uint32_t adc_chan = 6;     // We will use 6 ADC channels, 3x mic, 2x radar, 1x DAC. Can be from 1-8
    uint16_t dacOut = 0;       // Variable to hold DAC output value
    uint8_t startByte = 1;     // 0b00000001; 7 leading zeros followed by the start bit to initiate ADC
    
    /* Allocate memory for data and system time */
    printf("Allocating memory buffers for data and system time...\n");
    char *adcBuf = malloc(adcLen * adc_chan * samples * sizeof(*adcBuf));    // Allocate a data buffer of length adcLen*adc_chan for each ADC sampling
    char *dacBuf = malloc(dacLen * samples * sizeof(*dacBuf));               // Allocate a data buffer of length dacLen for each DAC output
    uint64_t *adcTimeBuf = malloc(adc_chan * samples * sizeof(*adcTimeBuf)); // Allocate a time buffer to hold the system time of successive sampling sequences.
    uint64_t *dacTimeBuf = malloc(samples * sizeof(*dacTimeBuf));            // Allocate a time buffer to hold the system time of successive DAC output sequences
    if (adcBuf == NULL || dacBuf == NULL || adcTimeBuf == NULL || dacTimeBuf == NULL){ // Check that none failed
        /* allocation failed, abort or take corrective action */
        printf("Allocation of one or more memory buffers failed, program ended without success!\n");
        return 1;
    }

    /* Assign correct bit masks for the ADC channel mux. */ /* 
    Need to send in units of bytes, and we communicate with MSB first. Thus, 
    initialization bits should be the 4 MSBs of the byte. */
    uint8_t ch1 = 0x80;                                     //0b1000 0000 = 0x80 for 4 MSB first, then 4 LSB don't care

    /* Assign dummy/helper variables */
    int i, j;
    uint16_t data = 0;    // 16 bit variable to hold 10 bits of ADC sample data as LSB.
    uint8_t read_MSB;     // 8 bit variable for 8 MSB
    uint8_t read_LSB;     // 8 bit variable for 8 LSB
    uint16_t counter = 0; // Dummy counter for testing purposes
    
    /* Initialize the HW SPI module and get ready for SPI transfers: */
    if (!bcm2835_init()){
        printf("bcm2835_init failed. Are you running as root (sudo)??\n");
        return 1;
    }
    if (!bcm2835_spi_begin()){
        printf("bcm2835_spi_begin failedg. Are you running as root (sudo)??\n");
        return 1;
    }

    /* Activate the SPI user-defined settings */
    spiSetup();
    printf("Successfully initialized and started SPI. ADC and DAC are now running...\n");

    /* Start running the ADC and DAC */
    for (i = 0; i < samples; i++){
        /* Write value to DAC */
        chipSelect(0); // LTC1451. Get ready for updating the DAC
        // Update system time variables prior to DAC output sequence, write newSysTime to DAC time buffer.
        timerFunction(&systemTime, &dacTimeBuf[i], &elapsedTime, &timeDifference);
        // Sawtooth signal generation for the DAC output.
        sawtooth(timeDifference, 200, &dacOut); // Fundamental frequency in Hz.
        // Linear frequecy sweep (chirp-function) generation for the DAC output.
        //linChirp(1000,7000,1,elapsedTime,&dacOut); // Start freq should not be lower than 200 Hz when using the test speaker!!
        // Update buffer for communication with the DAC and push the bytes out.
        dacBuf[i * dacLen + 1] = dacOut & 0x00FF;         // Mask out bits 1-8 first, then
        dacBuf[i * dacLen] = (dacOut >> 8) & 0x00FF;      // Shift right to mask out bits 9-12(9-16, but the 4 MSB should never be ones anyway)
        bcm2835_spi_writenb(&dacBuf[i * dacLen], dacLen); // Send bytes to the DAC for output conversion

        /* Sample the ADC */
        chipSelect(1); // MCP3008. Get ready for sampling ADC
        // Sample channel 1-6 in sequence. Can reduce/increase number of channels above if wanted.
        for (j = 0; j < adc_chan; j++){ // Must start with j=0 or will fuck up the indexing, but adc_chan may be reduced/increased above.
            // Update buffer for communication with the ADC and push the bytes out. Data coming back from ADC will overwrite the buffer.
            adcBuf[j * 3 + i * (adcLen * adc_chan)] = startByte;            // Byte to initiate communication/sampling
            adcBuf[j * 3 + 1 + i * (adcLen * adc_chan)] = ch1 + j * 0x10;   // Need to pass the ADC setup bits MSB first. 4 LSB of buf[] is don't care.
            adcTimeBuf[i * adc_chan + j] = bcm2835_st_read();               // System time prior to ADC sampling, write to ADC time buffer.
            bcm2835_spi_transfern(&adcBuf[j * 3 + i * (adcLen * adc_chan)], adcLen); //transfern(...) transfers adcLen bytes to the
            // ADC as stored in adcBuf, and subsequently writes the converted data to the same indices in adcBuf
        }

        /* Sleep at the least critical point in each loop, in hope that the scheduler will halt here if it 
        needs to, and not on a random place earlier in the loop */
        bcm2835_delayMicroseconds(5); //Sleep periodically so scheduler doesn't penalise us (THIS REQUIRES -lrt ADDING AS A COMPILER FLAG OR IT WILL CAUSE LOCK UP)
    }
    /* End running the ADC and DAC */
    
    /* Write data to files */
    printf("ADC and DAC finished converting data. Now writing data and timestamps to files...\n");
    FILE *adcTm;
    FILE *adcDat;
    FILE *dacTm;
    FILE *dacDat;
    /////////// Update this info with the correct path to your data directory ///////////
    adcTm = fopen("adcTiming.bin", "wb+"); // wb+ means write, binary, overwrite if existing
    adcDat = fopen("adcData.bin", "wb+");
    dacTm = fopen("dacTiming.bin", "wb+");
    dacDat = fopen("dacData.bin", "wb+");
    fwrite(adcTimeBuf, sizeof(adcTimeBuf[0]), adc_chan * samples, adcTm);
    fwrite(adcBuf, sizeof(adcBuf[0]), adcLen * adc_chan * samples, adcDat);
    fwrite(dacTimeBuf, sizeof(dacTimeBuf[0]), samples, dacTm);
    fwrite(dacBuf, sizeof(adcBuf[0]), dacLen * samples, dacDat);
    fclose(adcTm);
    fclose(adcDat);
    fclose(dacTm);
    fclose(dacDat);

    /* Print ADC sample data and DAC setvalue to terminal for sanity check. */
    /* This is just for convenience/verification and does not alter any data */
    // Shift ADC bytes, rearrange and print (only the first sampling instance)
    printf("First sample conversion from ADC channels 1-%d (or 0-%d ref datasheet):\n", adc_chan, adc_chan - 1);
    printf("| ");
    for (j = 0; j < adc_chan; j++){
        read_MSB = adcBuf[j * 3 + 1];
        read_LSB = adcBuf[j * 3 + 2];
        // Bit shift operations
        data = ((read_MSB << 8) | read_LSB) & 1023; // Shift MSB byte 8 bits to the left, put it together with the 8 LSB
        // and cancel out bits 11 to 16 (values larger than [2^10 - 1]=1023)
        printf("Channel %d: %d | ", j + 1, data);
    }
    // Check the last setvalue on the DAC (should now be converted on the DAC output):
    printf("\nFinal (last) DAC output value:\n");
    printf("DAC set value = %d | DAC most significant byte = %d | DAC least significant byte = %d \n", dacOut, dacBuf[(i - 1) * dacLen], dacBuf[(i - 1) * dacLen + 1]);

    /* End SPI, clean up and terminate the bcm2835 driver */
    bcm2835_spi_end();
    bcm2835_close();
    printf("Program ended successfully\n");
    return 0;
}
/* Main program ends */

/* Function implementations */

/* Set up the SPI parameters */
void spiSetup(){
    /* SPI clock frequency setup */ /* 
    Selects from power-of-2 enumerator, but the datasheet is wrong. You can use whatever multiple 
    of 2 in [2^1,2^16]. E.g not limited to powers of 2! In other words, the clock speed will be 
    250MHz/2*n, where n is any integer from 1 to 32768. */
    //bcm2835_spi_setClockDivider(BCM2835_SPI_CLOCK_DIVIDER_65536);   // The default.
    //bcm2835_spi_setClockDivider(12);                                // Max speed for IO-lines when looped back
    //bcm2835_spi_setClockDivider(32768);                             // Testspeed for setup; 7.63kHz
    //bcm2835_spi_setClockDivider(32);                                // Max speed for setup; 7.8125MHz!!! But this gives faulty conversions
    //bcm2835_spi_setClockDivider(70);                                // 3.57MHz = 250MHz/70.
    bcm2835_spi_setClockDivider(70); // Current clock freq: 250MHz/divider.

    /* SPI bit order setup */                                /*
    Most vs least significant bit first (MSB vs LSB) */
    bcm2835_spi_setBitOrder(BCM2835_SPI_BIT_ORDER_MSBFIRST); // The default

    /* SPI data transfer mode */
    bcm2835_spi_setDataMode(BCM2835_SPI_MODE3); /* 
    Clock idles high + inverted phase. Thus, data are captured on clock's rising edge and data 
    is output on a falling edge. This corresponds to MCP3008 where data is clocked in on rising 
    and out on falling edge. We use inverted clock since the level converters idle high when GPIOs 
    and ADC data pins are in HiZ.*/
    //bcm2835_spi_setDataMode(BCM2835_SPI_MODE0);                     // The default, also works but has idle low.

    /* Set up chip select settings */ /*
    Chip selects commented out, since they are set at appropriate times later with other function. */
    //bcm2835_spi_chipSelect(BCM2835_SPI_CS0);                        // LTC1451
    //bcm2835_spi_chipSelect(BCM2835_SPI_CS1);                        // MCP3008
    bcm2835_spi_setChipSelectPolarity(BCM2835_SPI_CS0, LOW); // the default
    bcm2835_spi_setChipSelectPolarity(BCM2835_SPI_CS1, LOW); // the default
}

/* SPI chip selection */
void chipSelect(uint8_t chip){
    if (chip == 1){
        bcm2835_spi_chipSelect(BCM2835_SPI_CS1); // ADC MCP3008
    }
    else if (chip == 0){
        bcm2835_spi_chipSelect(BCM2835_SPI_CS0); // DAC LTC1451
    }
    else{
        printf("Chip select out of range! HW SPI has only CS0/CS1 by default.\n"); /* Executed if no other statement is */
    }
}

/* Timer function to take system time and update variables for current time, elapsed time and time since last check */
void timerFunction(uint64_t *lastSysTime, uint64_t *newSysTime, uint32_t *elapTime, uint32_t *timeDiff){
    /*
    The function bcm2835_st_read() uses a 1MHz system clock to keep track of time. Thus, we will know 
    with +- 1us accuracy, the time difference between one sample and the next.
    */
    *newSysTime = bcm2835_st_read(); // Update with current system time
    if (*lastSysTime > 0){ // Normal operation, find microseconds passed since last run
        *timeDiff = *newSysTime - *lastSysTime;
        *elapTime = *elapTime + *timeDiff;
    }
    else{ // First run, we thus define t = 0 now.
        *timeDiff = 0;
        *elapTime = 0;
    }
    *lastSysTime = *newSysTime; // Prepare lastSysTime variable for the next run
}

/* Function to generate a sawtooth signal for the DAC */
void sawtooth(uint32_t timeDiff, double sigFreq, uint16_t *dacVal){
    // TimeDiff and sigPeriod in units of us, sigFreq in units of Hz. dacVal in 0-4095.
    double sigPeriod = 1000000 / sigFreq;
    if (timeDiff > 0){ // Update DAC with new value.
        *dacVal = *dacVal + round(timeDiff * 4095 / sigPeriod); // add value according to ramp rate and time since last run.
    }
    else{ // We just started the program, and it's nice to first set ground level.
        *dacVal = 0;
    }
    // Check if we are trying to write a larger value than the DAC can represent.
    if (*dacVal > 4095){ // We have passed another period, thus must subtract 2^12 to start over.
        *dacVal = *dacVal - 4096;
    }
}

/* Function to generate a linear chirp signal for the DAC (12 bit resolution) */
void linChirp(double startFreq, double endFreq, double duration, uint32_t elapTime, uint16_t *dacVal){
    /* start and end frequencies in Hz, duration in seconds. elapTime is total system time in us 
    and *dacVal should be in range 0-4095. */
    if (elapTime > (1000000 * duration)){ // Sweep time exceeded already, set DAC to ground value.
        *dacVal = 0;
    }
    else{ // Sweep is commencing, update with value according to given time
        double chirpyness = (endFreq - startFreq) / duration;
        double chirpVal = 4095 * (0.5 + 0.5 * cos(2 * PI * (startFreq + chirpyness / 2 * elapTime / 1000000) * elapTime / 1000000 - PI));
        *dacVal = round(chirpVal); // Round to nearest integer. This will always be in [0,4095] as cos is always in [-1,1]
    }
}

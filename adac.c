//
//  adac.c
//
//
//  Created by Kristoffer Kjærnes on 24.01.2017.
//  Strongly inspired by examples from Mike McCauley, author of the bcm2835 library
//
//
// After installing bcm2835, you can build this
// with something like:
// gcc -o adac adac.c -l bcm2835 -lrt
// and run with:
// sudo ./adac
// Add -lrt flag to the compile command if any bcm2835_delay... function is used!

#include <bcm2835.h>
#include <stdio.h>
#include <time.h>

// Main program
int main(int argc, char **argv)
{
    // First initialize the SPI and start a SPI session:
    if (!bcm2835_init())
    {
        printf("bcm2835_init failed. Are you running as root??\n");
        return 1;
    }
    if (!bcm2835_spi_begin())
    {
        printf("bcm2835_spi_begin failedg. Are you running as root??\n");
        return 1;
    }
    // Set up the SPI parameters
    bcm2835_spi_setBitOrder(BCM2835_SPI_BIT_ORDER_MSBFIRST);        // The default
    bcm2835_spi_setDataMode(BCM2835_SPI_MODE3);                     // Clock idles high + inverted phase. Thus:
    // data are captured on clock's rising edge and data is output on a falling edge. This corresponds to 
    // MCP3008 where data is clocked in on rising and out on falling edge. We use inverted clock since the
    // level converters idle high when GPIOs and ADC data pins are in HiZ.
    //bcm2835_spi_setDataMode(BCM2835_SPI_MODE0);                     // The default, also works but has idle low.
    
    //bcm2835_spi_setClockDivider(BCM2835_SPI_CLOCK_DIVIDER_65536);   // The default. Selects from enumerator, but
    // the datasheet is wrong. You can use whatever multiple of 2 in [2^1,2^16]. E.g not limited to powers of 2!
    // In other words, the clock speed will be 250MHz/2*n, where n is any integer from 1 to 32768.
    //bcm2835_spi_setClockDivider(12);                                // Max speed for IO-lines when looped back
    //bcm2835_spi_setClockDivider(32768);                             // Testspeed for setup; 7.63kHz
    //bcm2835_spi_setClockDivider(32);                                // Max speed for setup; 7.8125MHz!!!
    bcm2835_spi_setClockDivider(500);                                // Current clock freq: 250MHz/divider.
    //bcm2835_spi_setClockDivider(70);                                // 3.57MHz = 250MHz/70.
    //bcm2835_spi_chipSelect(BCM2835_SPI_CS0);                        // LTC1451
    //bcm2835_spi_chipSelect(BCM2835_SPI_CS1);                        // MCP3008
    bcm2835_spi_setChipSelectPolarity(BCM2835_SPI_CS0, LOW);        // the default
    bcm2835_spi_setChipSelectPolarity(BCM2835_SPI_CS1, LOW);        // the default
    
    int i;
    int j;
    // Assign correct bit masks for the ADC channel mux. Need to send in units of bytes, e.g init should be 4MSB.
    uint8_t ch1 = 0x80; //0b1000 0000; 0x80 for 4 MSB first, then 4 LSB don't care
    // uint8_t ch2 = 0x90; //0b1001 0000; 0x90 for 4 MSB first, then 4 LSB don't care
    // uint8_t ch3 = 0xA0; //0b1010 0000; 0xA0 for 4 MSB first, then 4 LSB don't care
    // uint8_t ch4 = 0xB0; //0b1011 0000; 0xB0 for 4 MSB first, then 4 LSB don't care
    // uint8_t ch5 = 0xC0; //0b1100 0000; 0xC0 for 4 MSB first, then 4 LSB don't care
    // uint8_t ch6 = 0xD0; //0b1101 0000; 0xD0 for 4 MSB first, then 4 LSB don't care
    // uint8_t ch7 = 0xE0; //0b1110 0000; 0xE0 for 4 MSB first, then 4 LSB don't care
    // uint8_t ch8 = 0xF0; //0b1111 0000; 0xF0 for 4 MSB first, then 4 LSB don't care
    
    uint8_t start_byte = 1; //0b00000001; 7 leading zeros followed by the start bit
    uint8_t read_MSB;
    uint8_t read_LSB;
    uint16_t data = 0;                      // 16 bit variable to hold 10 bits of sample data as LSB.
    uint16_t counter = 0;                   // Counter for DAC ramping
    uint16_t temp;
    uint32_t samples = 1000;                 // Take this many samples before ending
    uint32_t len = 3;                       // We need a buffer of 3 bytes per sample per ADC channel.
    uint32_t adc_chan = 6;                  // We will use six ADC channels, 3x mic, 2x radar, 1x DAC
    char buf[(len*adc_chan+2)*samples];     // Allocate a data buffer of length len*adc_chan+(2 DAC-bytes)
    uint64_t timeBuf[samples];              // Allocate a time buffer to hold the system time of successive
                                            // sampling sequences.
    uint64_t start, end, time_diff;         // 64 bit integers to hold the system start and end time
    start = bcm2835_st_read();              // bcm2835_st_read() uses a 1MHz system clock to keep track of time
                                            // Thus, we will know with +- 1us accuracy, the time difference between
                                            // one sample and the next.
    
    // Start taking data
    bcm2835_spi_chipSelect(BCM2835_SPI_CS1);    // MCP3008. Get ready for sampling
    for (i=0;i<samples;i++){
        timeBuf[i] = bcm2835_st_read();             //Write time of sampling to time buffer.
        // Sample channel 1-6 in sequence. Can reduce number of channels if wanted.
        for (j=0;j<6;j++){
            buf[j*3+i*(len*adc_chan+2)] = start_byte;        // Initiate communication/sampling
            buf[j*3+1+i*(len*adc_chan+2)] = ch1 + j*0x10;    // Need to pass the ADC setup bits MSB first. 4 LSB of buf[1] is don't care.
            bcm2835_spi_transfern(&buf[j*3+i*(len*adc_chan+2)], len);   //transfern transfers len bytes to the
            // ADC as written in buf, and simultanously writes the converted data over
        }
        // Write value to DAC
        bcm2835_spi_chipSelect(BCM2835_SPI_CS0);    // LTC1451. Get ready for setting the DAC
        temp = counter;
        buf[(j+1)*3+1+i*(len*adc_chan+2)] = temp & 0x00FF;      // Mask out bits 1-8 first, then
        buf[(j+1)*3+i*(len*adc_chan+2)] = (temp>>8) & 0x00FF;   // Shift right to mask out bits 9-12(9-16, but the 4 MSB should never be ones anyway)
        bcm2835_spi_writenb(&buf[(j+1)*3+i*(len*adc_chan+2)], 2);      //2 final bytes for the DAC
        counter = counter + 1023;
        if (counter > 4095){   //Make sure it starts over instead of increasing further
            counter = 0;
        }
        bcm2835_spi_chipSelect(BCM2835_SPI_CS1);    // MCP3008. Get ready for sampling again
        //bcm2835_delayMicroseconds(8); //Sleep occasionally so scheduler doesn't penalise us (THIS REQUIRES -lrt ADDING AS A COMPILER FLAG OR IT WILL CAUSE LOCK UP)
    }
    // Sampling complete!
    end = bcm2835_st_read();    // Time at end of run
    time_diff = end - start;
    
    // Write data to files
    //FILE *tme;
    //FILE *dat;
    //tme = fopen("/mnt/home/raspi/c-code/timeDiff.bin", "wb+");
    //dat = fopen("/mnt/home/raspi/c-code/sampleBuf.bin", "wb+");
    //fwrite(timeBuf, sizeof(timeBuf[0]), samples, tme);
    //fwrite(buf, sizeof(buf[0]), (len*adc_chan+2)*samples, dat);
    //fclose(tme);
    //fclose(dat);

    // Shift ADC bytes, rearrange and print for sanity check (only the first sampling instance)
    // Something seems to be wrong with the DAC data and the time data here.
    printf("| ");
    for (j=0;j<6;j++){
        read_MSB = buf[j*3+1];
        read_LSB = buf[j*3+2];
        // Bit shift operations
        data = ((read_MSB<<8) | read_LSB) & 1023;   // Shift MSB byte 8 bits to the left, put it together with the 8 LSB
        // and null out bits 13 to 16 (values larger than 2^10=1023)
        printf("Channel %d: %d | ", j+1, data);
    }
    // Check the last data set on the DAC (should now be converted on the DAC output):
    printf("\nDAC set = %d | DAC MSB = %d | DAC LSB = %d \n", temp, buf[(j+1)*3+i*(len*adc_chan+2)],buf[(j+1)*3+1+i*(len*adc_chan+2)]);
    //printf("\nDAC set = %d | DAC MSB = %d | DAC LSB = %d \n", temp, buf[(j+1)*3],buf[(j+1)*3+1]);
    // Print time data/duration (something's not right here...)
    printf("CPU start: %d | CPU end: %d | Total CPU time: %d us\n", start, end, time_diff);
    // Print the last three sample times (something's not right here...)
    printf("Time buffer last three samples: %d, %d, %d\n", timeBuf[i-2], timeBuf[i-1], timeBuf[i]);
    
    // End SPI, clean up and terminate the bcm2835 driver
    bcm2835_spi_end();
    bcm2835_close();
    return 0;
}

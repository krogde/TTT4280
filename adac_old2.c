//
//  adac.c
//
//
//  Created by Kristoffer Kjærnes on 24.01.2017.
//  Strongly inspired by examples from Mike McCauley, author of the bcm2835 library
//
// library documentation on http://www.airspayce.com/mikem/bcm2835/modules.html
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
    //bcm2835_spi_setClockDivider(32);                                // Max speed for setup; 7.8125MHz!!! But
                                                                      // this gives faulty conversions
    bcm2835_spi_setClockDivider(70);                                // Current clock freq: 250MHz/divider..
    //bcm2835_spi_chipSelect(BCM2835_SPI_CS0);                        // LTC1451
    //bcm2835_spi_chipSelect(BCM2835_SPI_CS1);                        // MCP3008
    bcm2835_spi_setChipSelectPolarity(BCM2835_SPI_CS0, LOW);        // the default
    bcm2835_spi_setChipSelectPolarity(BCM2835_SPI_CS1, LOW);        // the default
    
    int i;
    int j;
    // Assign correct bit masks for the ADC channel mux. Need to send in units of bytes, e.g init should be 4MSB.
    uint8_t ch1 = 0x80; //0b1000 0000; 0x80 for 4 MSB first, then 4 LSB don't care
    
    uint8_t start_byte = 1; //0b00000001; 7 leading zeros followed by the start bit to initiate ADC session
    uint8_t read_MSB;
    uint8_t read_LSB;
    uint16_t data = 0;                      // 16 bit variable to hold 10 bits of sample data as LSB.
    uint16_t counter = 0;                   // Counter for DAC ramping
    uint16_t temp;                          // Temporary variable to hold DAC output value
    uint32_t samples = 10000;               // Take this many samples before ending
    uint32_t adcLen = 3;                    // We need a buffer of 3 bytes per sample per ADC channel.
    uint32_t dacLen = 2;                    // We need a buffer of 2 bytes per DAC sequence.
    uint32_t adc_chan = 6;                  // We will use 6 ADC channels, 3x mic, 2x radar, 1x DAC
    char adcBuf[adcLen*adc_chan*samples];   // Allocate a data buffer of length adcLen*adc_chan for each ADC sampling
    char dacBuf[dacLen*samples];            // Allocate a data buffer of length dacLen for each DAC output
    uint64_t adcTimeBuf[adc_chan*samples];  // Allocate a time buffer to hold the system time of successive
                                            // sampling sequences.
    uint64_t dacTimeBuf[samples];           // Allocate a time buffer to hold the system time of successive
                                            // DAC output sequences
    // The function bcm2835_st_read() uses a 1MHz system clock to keep track of time. Thus, we will know 
    // with +- 1us accuracy, the time difference between one sample and the next.
    
    // Start taking data
    bcm2835_spi_chipSelect(BCM2835_SPI_CS1);    // MCP3008. Get ready for sampling
    for (i=0;i<samples;i++){
        // Sample channel 1-6 in sequence. Can reduce number of channels if wanted.
        for (j=0;j<adc_chan;j++){
            adcBuf[j*3+i*(adcLen*adc_chan)] = start_byte;       // Byte to initiate communication/sampling
            adcBuf[j*3+1+i*(adcLen*adc_chan)] = ch1 + j*0x10;   // Need to pass the ADC setup bits MSB first. 4 LSB of buf[] is don't care.
            adcTimeBuf[i*adc_chan+j] = bcm2835_st_read();       // System time prior to ADC sampling, write to ADC time buffer.
            bcm2835_spi_transfern(&adcBuf[j*3+i*(adcLen*adc_chan)], adcLen);   //transfern(...) transfers adcLen bytes to the
            // ADC as stored in adcBuf, and subsequently writes the converted data to the same indices in adcBuf
        }
        // Write value to DAC
        bcm2835_spi_chipSelect(BCM2835_SPI_CS0);    // LTC1451. Get ready for setting the DAC
        temp = counter;                                     // Update temp variable with new output value
        dacBuf[i*dacLen+1] = temp & 0x00FF;         // Mask out bits 1-8 first, then
        dacBuf[i*dacLen] = (temp>>8) & 0x00FF;      // Shift right to mask out bits 9-12(9-16, but the 4 MSB should never be ones anyway)
        dacTimeBuf[i] = bcm2835_st_read();          // System time prior to DAC output sequence, write to DAC time buffer.
        bcm2835_spi_writenb(&dacBuf[i*dacLen], dacLen);// Send bytes to the DAC for output conversion
        // Generate a sawtooth signal for the DAC
        counter = counter + 15;
        if (counter > 4095){   //Make sure it starts over instead of increasing further. The DAC is 12 bits only remember (2^12=4096)
            counter = 0;
        }
        bcm2835_spi_chipSelect(BCM2835_SPI_CS1);    // MCP3008. Get ready for ADC sampling again
        //bcm2835_delayMicroseconds(38); //Sleep periodically so scheduler doesn't penalise us (THIS REQUIRES -lrt ADDING AS A COMPILER FLAG OR IT WILL CAUSE LOCK UP)
    }
    
    // Write data to files
    FILE *adcTm;
    FILE *adcDat;
    FILE *dacTm;
    FILE *dacDat;
    /////////// Update this info with the correct path to your data directory ///////////
    adcTm = fopen("adcTiming.bin", "wb+"); // wb+ means write, binary, overwrite if existing
    adcDat = fopen("adcData.bin", "wb+");
    dacTm = fopen("dacTiming.bin", "wb+");
    dacDat = fopen("dacData.bin", "wb+");
    fwrite(adcTimeBuf, sizeof(adcTimeBuf[0]), adc_chan*samples, adcTm);
    fwrite(adcBuf, sizeof(adcBuf[0]), adcLen*adc_chan*samples, adcDat);
    fwrite(dacTimeBuf, sizeof(dacTimeBuf[0]), samples, dacTm);
    fwrite(dacBuf, sizeof(adcBuf[0]), dacLen*samples, dacDat);
    fclose(adcTm);
    fclose(adcDat);
    fclose(dacTm);
    fclose(dacDat);
    

    // Shift ADC bytes, rearrange and print for sanity check (only the first sampling instance)
    printf("First sample conversion from ADC channels 1-6 (or 0-5 ref datasheet):\n");
    printf("| ");
    for (j=0;j<adc_chan;j++){
        read_MSB = adcBuf[j*3+1];
        read_LSB = adcBuf[j*3+2];
        // Bit shift operations
        data = ((read_MSB<<8) | read_LSB) & 1023;   // Shift MSB byte 8 bits to the left, put it together with the 8 LSB
        // and cancel out bits 11 to 16 (values larger than [2^10 - 1]=1023)
        printf("Channel %d: %d | ", j+1, data);
    }
    // Check the last data set on the DAC (should now be converted on the DAC output):
    printf("\nFinal (last) DAC output value:\n");
    printf("DAC set value = %d | DAC most significant byte = %d | DAC least significant byte = %d \n", temp, dacBuf[(i-1)*dacLen],dacBuf[(i-1)*dacLen+1]);
    
    // End SPI, clean up and terminate the bcm2835 driver
    bcm2835_spi_end();
    bcm2835_close();
    printf("Program ended successfully\n");
    return 0;
}

int f0; //Chrip start frequency
int f1; //Chrip end frequency
double T; //Duration of chirp in microseconds
uint16_t A; //Amplitude of chirp, unsigned 12bit int

double k = (f1-f0)/T; //Chirpyness 

uint16_t adcVal = A * sin( 2*pi * 1000000 * (f0*dacTimeBuf[i] + (k/2)*dacTimeBuf[i]*dacTimeBuf[i]) ); //Value to write to adc
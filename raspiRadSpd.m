raspiAnalyze;
% Plot DAC signal sampled back and output requested.
figure(3);
plot(allTimes.DAC_Sampled*1e-6 ,allData.DAC_Sampled, '-o',...
    allTimes.DAC_Output*1e-6 ,allData.DAC_Output, '-o'...
    );
title('DAC send/recived')
ylim([0, 4095]) % 12 bit DAC gives only values 0-4095. Note that the 
% sampled DAC signal will be in the ADC range 0-1023.
xlabel('t [s]');
ylabel('Conversion/set value');
legend('DAC sampled in','DAC requested output');
% Plot radar signals
figure(2);
plot(allTimes.RadarIF_I*1e-6 ,allData.RadarIF_I, '-o',...
    allTimes.RadarIF_Q*1e-6 ,allData.RadarIF_Q, '-o'...
    );
title('Radar data');
ylim([0, 1023]) % 10 bit ADC gives only values 0-1023
xlabel('t [s]');
ylabel('Conversion value');
legend('Radar in-phase','Radar quadrature');


%Doppler shift
Ts = 40*10^-6; %Time between samples
I = fft(allData.RadarIF_I);
%Q = fft(allData.RadarIF_Q);
L = length(I);
f = (1/Ts)*(0:(L-1))/L;
f = transpose(f);
figure(4)
plot(f,abs(I))
ylim([0 30000]);
xlim([0 500]);
[val,fmax] = max(abs(I));
v = freq2speed(f(fmax));
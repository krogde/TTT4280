clearvars; close all;
path = '';
[samples, channels, rawData, rawTimes] = raspiImport(path);

allData = struct(...    % Organize all numerical data, 10 bit ADC, 12 bit DAC output
    'Mic1',rawData(:,1),... 
    'Mic2',rawData(:,2),...
    'Mic3',rawData(:,3),... 
    'RadarIF_I',rawData(:,4),...    % In-phase signal
    'RadarIF_Q',rawData(:,5),...    % Quadrature signal
    'DAC_Sampled',rawData(:,6),...  % DAC signal sampled back with ADC
    'DAC_Output',rawData(:,end)...    % DAC signal as requested on its output
     );
% allData.RadarIF_I = allData.RadarIF_I - 512;
% allData.RadarIF_Q = allData.RadarIF_Q - 512;

allTimes = struct(...   % Organize all timestamps accordingly, unit is us
    'Mic1',rawTimes(:,1),...
    'Mic2',rawTimes(:,2),...
    'Mic3',rawTimes(:,3),...
    'RadarIF_I',rawTimes(:,4),...
    'RadarIF_Q',rawTimes(:,5),...
    'DAC_Sampled',rawTimes(:,6),...
    'DAC_Output',rawTimes(:,end)...
    );

% Setting all data with to long time diff to zero
% Using 60 as max time between samples
%MaxTimeDiff = 50;
%allData.Mic1([999; diff(allTimes.Mic1)] > MaxTimeDiff) = 0;
%allData.Mic2([999; diff(allTimes.Mic2)] > MaxTimeDiff) = 0;
%allData.Mic3([999; diff(allTimes.Mic3)] > MaxTimeDiff) = 0;

%PLOT
% Plot all microphone signals (or ADC channels 1, 2, 3)
figure(1);
subplot(2,1,1);
plot(allTimes.Mic1*1e-6 ,allData.Mic1, '-o',...
    allTimes.Mic2*1e-6 ,allData.Mic2, '-o',...
    allTimes.Mic3*1e-6 ,allData.Mic3, '-o'...
    );
title('Mic data');
ylim([0, 1023]) % 10 bit ADC gives only values 0-1023
xlabel('t [s]');
ylabel('Conversion value');
legend('Mic1','Mic2','Mic3');

% Plot time difference for the first mic
subplot(2,1,2);
plot(diff(allTimes.Mic1), '-o');
title('Time diff between samples')
xlabel('Sample number');
ylabel('Time between samples [us]');
legend('ADC channel 1 (0 in datasheet)');

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

% Calculating correlations
a = 0.12; %Distance between mics
cair = 343;
delay = 7*10^-6; %delay between the different mics
tmax = a/cair + delay; %Max time between mics
Ts = 40*10^-6; %Time between samples
nmax = tmax/Ts; %Max number of samples between mics

sig1 = interp(allData.Mic1,100);
sig2 = interp(allData.Mic2,100);
sig3 = interp(allData.Mic3,100);

[c21,lags] = xcorr(sig2,sig1);
[temp,iv] = max(c21);
t21 = lags(iv);

[c31,lags] = xcorr(sig3,sig1);
[temp,iv] = max(c31);
t31 = lags(iv);

[c32,lags] = xcorr(sig3,sig2);
[temp,iv] = max(c32);
t32 = lags(iv);

% Calculate estimated angles
innerfunc = sqrt(3)*(t21+t31)/(t21-t31-2*t32);
thetavec = atan(innerfunc);


%Doppler shift
Ts = 40*10^-6; %Time between samples
I = fft(allData.RadarIF_I);
Q = fft(allData.RadarIF_Q);
L = length(I);
f = (1/Ts)*(0:(L-1))/L;
f = transpose(f);
figure(4)
subplot(2,1,1)
plot(f,abs(I))
ylim([0 1000]);
xlim([0 1/(2*Ts)]);
subplot(2,1,2)
plot(f,abs(Q))
ylim([0 1000]);
xlim([0 1/(2*Ts)]);



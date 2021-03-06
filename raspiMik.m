raspiAnalyze;
% Plot all microphone signals (or ADC channels 1, 2, 3)
timeDiff = diff(allTimes.Mic1);
for i=1:1:(length(allData.Mic1)-1)
   if(timeDiff(i)> 50) 
      allData.Mic1(i+1) = allData.Mic1(i);
      allData.Mic2(i+1) = allData.Mic2(i);
      allData.Mic3(i+1) = allData.Mic3(i);
   end
end
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

% Calculating correlations
a = 0.12; %Distance between mics
cair = 343;
delay = 7*10^-6; %delay between the different mics
tmax = a/cair + delay; %Max time between mics
Ts = 40*10^-6; %Time between samples
nmax = tmax/Ts; %Max number of samples between mics

sig1 = interp((allData.Mic1(1000:end)-512),100);
sig2 = interp((allData.Mic2(1000:end)-512),100);
sig3 = interp((allData.Mic3(1000:end)-512),100);

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


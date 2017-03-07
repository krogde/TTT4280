% This script will import the binary data from files written by the adac.c
% program on Raspberry Pi. You need to update the path for your respective
% remote computer system(s) below in order to load the files.
%
% The script uses the function raspiImport to do the actual import and
% conversion from binary data to numerical values. Make sure you have
% downloaded it as well from itsLearning.
%
% After the import function is finished, the data and timestamps are
% written to separate structs where more intuitive names are used. This
% part of the script assumes that you are sampling 6 channels in sequence
% according to the scheme in the block diagram of the labmanual. The data
% and timestamps are finally plotted according to this scheme. If you did
% not use the default sampling and DAC output scheme, this part will not
% work properly.


%% Open, import and close binary data files from Raspberry Pi

% First clear everything that was before. Comment this out if you want to
% keep something that is already open
clearvars; close all;

% Check computer platform and assign correct path to data files
%%%%% IMPORTANT %%%%%
% BE SURE TO HAVE THE FINAL SLASH PRESENT IN THE PATH
%%%%% IMPORTANT END %%%%%

path = '';

% Run function to import all data and timestamps from the files. It only
% cares that four data files with specified names exist in the path 
% location. If you change the names or want to read more/fewer files, you
% must change the function accordingly.
[samples, channels, rawData, rawTimes] = raspiImport(path);

%% Write all translated data and timestamps to data structs
% Here we will use sensible names for the data and timestamps. This part
% assumes that you have sampled 6 ADC channels with connections according
% to the block diagram in labmaual and set the DAC output on each loop.
% This is the default setup in the distributed adac.c program.
%
% If you want to do different things, just comment out, change or delete
% the rest of the script.

% Generate structs
allData = struct(...    % Organize all numerical data, 10 bit ADC, 12 bit DAC output
    'Mic1',rawData(:,1),...
    'Mic2',rawData(:,2),...
    'Mic3',rawData(:,3),...
    'RadarIF_I',rawData(:,4),...    % In-phase signal
    'RadarIF_Q',rawData(:,5),...    % Quadrature signal
    'DAC_Sampled',rawData(:,6),...  % DAC signal sampled back with ADC
    'DAC_Output',rawData(:,end)...    % DAC signal as requested on its output
    );

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
%MaxTimeDiff = 60;
%allData.Mic1([999; diff(allTimes.Mic1)] > MaxTimeDiff) = 0;
%allData.Mic2([999; diff(allTimes.Mic2)] > MaxTimeDiff) = 0;
%allData.Mic3([999; diff(allTimes.Mic3)] > MaxTimeDiff) = 0;

%% Plot all translated data with its respective timestamp
% Plot all microphone signals (or ADC channels 1, 2, 3)
figure(1);
subplot(2,1,1);
%Plot to see the connection between large time diff and weird sample data
%n = 1:1:10000;
%plot(n,allData.Mic1, '-o',...
%    n,allData.Mic2, '-o',...
%    n,allData.Mic3, '-o'...
%    );
plot(allTimes.Mic1*1e-6 ,allData.Mic1, '-o',...
    allTimes.Mic2*1e-6 ,allData.Mic2, '-o',...
    allTimes.Mic3*1e-6 ,allData.Mic3, '-o'...
    );
title('Mic data');
ylim([0, 1023]) % 10 bit ADC gives only values 0-1023
xlabel('t [s]');
ylabel('Conversion value');
legend('Mic1','Mic2','Mic3');

% Plot time difference between successive samples. We here use the first
% data column as an example. This info tells you the nominal sample period 
% for the system, and clearly reveals where the linux scheduler has halted
% our program. 
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
a = 0.03; %Distance between mics
cair = 343;
delay = 10*10^-6; %delay between the different mics
tmax = a/cair + delay; %Max time between mics
Ts = 60*10^-6; %Time between samples
nmax = tmax/Ts; %Max number of samples between mics

[c21,lags] = xcorr(allData.Mic2,allData.Mic1);
[temp,iv] = max(c21);
t21 = lags(iv);

[c31,lags] = xcorr(allData.Mic3,allData.Mic1);
[temp,iv] = max(c31);
t31 = lags(iv);

[c32,lags] = xcorr(allData.Mic3,allData.Mic2);
[temp,iv] = max(c32);
t32 = lags(iv);

% Calculate estimated angles
innerfunc = sqrt(3)*(t21+t31)/(t21-t31-2*t32);
thetavec = atan(innerfunc);



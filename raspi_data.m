clearvars; close all;
%% Open, import and close data files from C on Raspberry Pi
% Make file IDs

fidTmDf = fopen('timeDiff.bin');
fidSpDt = fopen('sampleBuf.bin');

% Read binary data to local variables
tmSample = fread(fidTmDf);
dtBuf = fread(fidSpDt);
% Close files properly after import
fclose(fidTmDf);
fclose(fidSpDt);
% Clear file IDs and ans
clearvars fidSpDt fidTmDf ans;

%% Organize data
% ADC data comes in chunks of 3 bytes per channel, but only the last two
% are actually containing the data. Moreover, only the 2LSB of the MSB byte
% should be counted (ADC has only 10 bit resolution)

lenData = length(dtBuf);    % Total number of bytes
samples = lenData/20;       % 3bytes*6channels + 2bytes for DAC per round
tempData = zeros(samples,8);

msbByte = 0;
lsbByte = 0;
idx=1;
for i = 1:samples
    for j = 1:7
        if j<7
            msbByte = bitshift(bitand(dtBuf(idx+(j-1)*3+1), 3, 'uint8'),8);
            lsbByte = dtBuf(idx+(j-1)*3+2);
            tempData(i,j) = uint16(msbByte + lsbByte);
        elseif j==7
            msbByte = bitshift(bitand(dtBuf(idx+(j-1)*3), 15, 'uint8'),8);
            lsbByte = dtBuf(idx+(j-1)*3+1);
            tempData(i,j) = uint16(msbByte + lsbByte);
        else
            error('Something went wrong!');
        end
    end
    idx = idx + 20;
    tempData(i,8) = typecast(uint8(tmSample((i-1)*8+1:(i-1)*8+8)), 'uint64');
end

tmDiff = diff(tempData(:,8));   % Find the difference in us between
% successive samples. A 1MHz system timer is used for timing.
sampleData = struct('M1',tempData(:,1),'M2',tempData(:,2),'M3',...
    tempData(:,3),'Ri',tempData(:,4),'Rq',tempData(:,5),...
    'DacRx',tempData(:,6),'DacTx',tempData(:,7),'T_sample',tempData(:,8),...
    'T_diff',tmDiff);

tmRelative = zeros (1,samples);
dummy = 0;
for i = 1:(samples-1)
    tmRelative(i) = dummy;
    dummy = dummy + sampleData.T_diff(i); % This one is samples-1 long
end
tmRelative(i+1) = dummy;  % Add the final difference
tmRelative = tmRelative*1e-6; % Convert from us to seconds

% In this example, the time difference between each channel was ~9us
% Has been added simply as additions.
% There is something wrong with the DAC Tx data (12 bit out).  The Rx data
% are more reasonable (10 bit).
subplot(2,2,1)
plot(tmRelative,sampleData.M1, '-o',tmRelative+9e-6, sampleData.M2, '-o',...
    tmRelative+18e-6, sampleData.M3, '-o');
title('Sample data mic')
ylim([0, 1023])
xlabel('t [s]');
ylabel('Conversion value');
subplot(2,2,2)
plot(tmRelative+27e-6, sampleData.Ri, '-o',tmRelative+36e-6, sampleData.Rq, '-o');
title('Sample data radar')
ylim([0, 1023])
xlabel('t [s]');
ylabel('Conversion value');
subplot(2,2,3)
plot(tmRelative+45e-6,sampleData.DacRx, '-o',tmRelative+54e-6, sampleData.DacTx, '-o');
title('DAC')
ylim([0, 4095])
xlabel('t [s]');
ylabel('Conversion value'); 
subplot(2,2,4)
plot(sampleData.T_diff, '-o');
title('T diff')
xlabel('Sample number');
ylabel('Time between samples [us]');

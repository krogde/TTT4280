% raspiImport takes in a path string and imports four specified binary data
% files from this path. It returns the raw data and timestamps from the ADC
% and DAC as well as the number of samples and ADC channels used.
function [samples, channels, numData, timestamps] = raspiImport(path)

% Make file IDs
fidAdcTime = fopen(strcat(path,'adcTiming.bin'));
fidAdcData = fopen(strcat(path,'adcData.bin'));
fidDacTime = fopen(strcat(path,'dacTiming.bin'));
fidDacData = fopen(strcat(path,'dacData.bin'));

% Read binary data to local variables
adcTime = fread(fidAdcTime);
adcData = fread(fidAdcData);
dacTime = fread(fidDacTime);
dacData = fread(fidDacData);

% Close files properly after import
fclose(fidAdcTime);
fclose(fidAdcData);
fclose(fidDacTime);
fclose(fidDacData);

% Clear file IDs and ans
clearvars fidAdcTime fidDacTime fidAdcData fidDacData ans;

%% Interpret ADC and DAC data plus the corresponding timestamps
% Save them to raw data and timing matrices. The final column in both
% rawData and rawTimes is data for the DAC. All columns before that
% contain the information for the number of ADC channels that were used.
% Useful variables
lenAdcData = length(adcData);       % Total number of ADC data bytes
lenDacData = length(dacData);       % Total number of DAC data bytes
samples = lenDacData/2;             % 2 bytes per DAC data output
channels = lenAdcData/(samples*3);  % 3 bytes per sample per ADC channel
numData = zeros(samples,channels+1);    % Raw data matrix
timestamps = zeros(samples,channels+1); % Raw timestamp matrix
% Dummy variables
msbByte = 0;    % Most significant bits-byte
lsbByte = 0;    % Least significant bits-byte
idx=1;          % Index dummy for ADC data
for i = 1:samples
    % ADC data:
    for j = 1:channels
        % ADC data comes in chunks of 3 bytes per channel, but only the 
        % last two are actually containing the data. Moreover, only the 
        % 2LSB of the MSB byte should be counted (ADC has only 10 bit 
        % resolution). Bitshifts and bitmasks are used to pull out the
        % correct values from the bytes.
        msbByte = bitshift(bitand(adcData(idx+(j-1)*3+1), 3, 'uint8'),8);
        lsbByte = adcData(idx+(j-1)*3+2);
        numData(i,j) = uint16(msbByte + lsbByte);  % Save data to temp
        % ADC timings (you really need to sit down and draw this if you
        % want to see how it becomes like this!)
        timestamps(i,j) = typecast(uint8(adcTime(((i-1)*channels*8) +...
            ((j-1)*8) + 1:((i-1)*channels*8) + ((j-1)*8) + 8)), 'uint64');
    end
    % DAC data:
    % DAC data comes in chunks of 2 bytes per sequence, but only the 4LSB 
    % of the MSB byte should be counted (DAC has only 12 bit resolution). 
    % Bitshifts and bitmasks are used to pull out the correct values from 
    % the bytes.
    msbByte = bitshift(bitand(dacData(i*2-1), 15, 'uint8'),8);
    lsbByte = dacData(i*2);
    numData(i,j+1) = uint16(msbByte + lsbByte);    % Save data to temp
    % DAC timings
    timestamps(i,j+1) = typecast(uint8(dacTime((i-1)*8+1:(i-1)*8+8)),'uint64');
    % Update loop
    idx = idx + 3*channels;
end

% Subtract initial time from all timestamps to relate everything to zero
% diff = tempTimes(1,1) - tempTimes(1,7);
startTime = min(timestamps); % First find all minimum values of each column
startTime = min(startTime); % Then find the true start value from these
% It is either the first ADC or the first DAC depending on which you ran
% first.
% Now subtract the minimal value from all timestamps to relate all of them
% to zero as a relative starting point
timestamps = timestamps - startTime;

% Clean up
clearvars lsbByte msbByte i j idx startTime;
end
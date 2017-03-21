function [y_red, y_green, y_blue] = acquire(video_file)
%% Function returns the red, green and blue signal ready to be analyzed
%% Load video recording
if ischar(video_file),
    display(['Loading file ' video_file]);
    v = VideoReader(video_file);
else
    v = video_file;
end

numFrames = v.NumberOfFrames;

%display(['Total frames: ' num2str(numFrames)]);

y_red = zeros(1, numFrames);
y_green = zeros(1, numFrames);
y_blue = zeros(1, numFrames);
v = VideoReader(video_file);
i = 1;


%% Read video frame by frame
while hasFrame(v)
    frame = v.readFrame();
    
    if i == 1
        close all;
        imshow(frame)
        choice = questdlg('Choose region?','Choose region or whole image','Yes','No','No');
        switch choice
            case 'Yes'
                r = round(getrect);
            case 'No'
                r = [0 0 size(frame,2) size(frame,1)];
        end
        close all;
    end
    frame = imcrop(frame, r);

    redPlane = frame(:,:,1);
    greenPlane = frame(:,:,2);
    bluePlane = frame(:,:,3);
    y_red(i) = sum( sum( redPlane ) ) / ( size(frame, 1) * size(frame, 2) );
    y_green(i) = sum( sum( greenPlane ) ) / ( size(frame, 1) * size(frame, 2) );
    y_blue(i) = sum( sum( bluePlane ) ) / ( size(frame, 1) * size(frame, 2) );
    i = i+1;
end


display('Signal acquired.');
display(' ');
display(['Sampling rate is ' num2str(v.FrameRate) '. You can now run process(your_signal_variable, ' num2str(v.FrameRate) ')']);

end


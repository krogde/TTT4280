Fs = 2000;      %Smpl freq.
f0 = 0;        %Min freq.
f1 = 10;      %Max freq.
T  = 20;        %End time
A  = 0.69;      %Amplitude

k = (f1-f0)/T;  %Chirpyness

t = 0:1/Fs:T;

x = A*sin( 2*pi * (f0*t + k/2*t.^2) );

figure(1);
plot(t,x);
xlabel('Time'); ylabel('Amplitude');

w = hamming(250);
figure(2);
spectrogram(x,w,20,1024,Fs);
sound(x,Fs);
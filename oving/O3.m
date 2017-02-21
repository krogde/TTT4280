%--Variables--
f0 = 0.25;
f1 = 0.35;
fc0 = 0.20;
fc1 = 0.40;
d = 400;
%------------
n = 0:99;

%Signal
sig = 2.3 * (sin(2*pi*f0*n) + sin(2*pi*f1*n));

%Signal, delayed signal
m = 0:999;
x = [sig zeros(1, 900)] + randn(1, 1000);
x_d = [zeros(1, d), sig, zeros(1, 900-d)] + randn(1,1000);

%Filter
X = fft(x);
X_d = fft(x_d);
f = 0:1/(length(X)-1):1;

%Ideal bandpass
filter = zeros(1, length(X));
filter(length(X)*fc0:length(X)*fc1) = ones(1,length(X)*fc1-length(X)*fc0+1);
filter = filter + fliplr(filter);

%Filter the signals
y = ifft(filter.*X);
y_d = ifft(filter.*X_d);

%Find lag unfiltered
[r, lag] = xcorr(x_d,x);
[~, idx] = max(abs(r));
D = lag(idx);

%Find lag filtered
[rf, lagf] = xcorr(y_d,y);
[~, idxf] = max(abs(rf));
Df = lagf(idxf);

figure(1);
subplot(2,1,1); stem(m, x, '.');
subplot(2,1,2); stem(m, x_d, '.');

%figure(2);
%subplot(2,1,1); stem(f, abs(X), '.'); xlim([0 1]);
%subplot(2,1,2); stem(f, abs(X_d), '.'); xlim([0 1]);

%figure(3);
%subplot(2,1,1); stem(m, y, '.');
%subplot(2,1,2); stem(m, y_d, '.');

%figure(4)
%subplot(2,1,1); stem(lag, abs(r),'.');
%subplot(2,1,2); stem(lagf, abs(rf),'.');

%fprintf('Real lag = %d,\nUnflitered lag = %d,\nFiltered lag = %d\n', d, D, Df);

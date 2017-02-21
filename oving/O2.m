%Set delay
d = 50;

%Original signal
n = -pi : 2*pi/99 : pi;
s = sin(n);
x = [ zeros(1,100) s zeros(1,100) ];

%Delayed signal
x_d = [ zeros(1,100 + d) s zeros(1,100 - d) ];

%Some axises
axis = -150 : 300/299 : 150;
l = -300 : 600/598 : 300;

%Find the lag
[r, lag] = xcorr(x_d,x);
[~, idx] = max(abs(r));
D = lag(idx);

%Plot everything
figure(1)
subplot(2,1,1);
stem(axis, x, '.');
subplot(2,1,2);
stem(axis, x_d, '.');

figure(2)
stem(l, abs(r), '.'); xlim([-150, 150]);  
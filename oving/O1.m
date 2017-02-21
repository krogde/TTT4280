%True impulse response
ir_true = zeros(1,1000);
ir_true(10) = 1;
ir_true(100) = 0.6;

%Stimulus signal
n = 0:999;
stimulus_signal = sin(n);
    
% Simulate a measurement situation
output_signal_from_system = conv(stimulus_signal,ir_true);

%Estimation of impulse response
X = fft(stimulus_signal, 2048);
Y = fft(output_signal_from_system, 2048);
ir_estimate = abs(ifft(Y./X));

%Plot responses
figure(1)
subplot(2,1,1);
stem(ir_true,'.'); xlim([0,200]); ylim([0, 1.2]);
title('True impulse response h[n]');
xlabel('n'); ylabel('h_{est}[n]');

subplot(2,1,2);
stem(ir_estimate,'.'); xlim([0,200]); ylim([0, 1.2]);
title('Estimated impulse response');
xlabel('n'); ylabel('h_{est}[n]');

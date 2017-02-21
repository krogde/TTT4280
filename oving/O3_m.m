n = 10000;

v  = zeros(1, n);
vf = zeros(1, n);
for i = 1:n
    run('O3');
    v(i) = D;
    vf(i) = Df;
end

subplot(2,1,1); hist(v, 50);
subplot(2,1,2); hist(vf, 50);
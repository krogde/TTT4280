function v_r = freq2speed(f_d)
    c = 299792458;
    f_0 = 24.02*10^9;
    v_r = (f_d*c)/(2*f_0);
end
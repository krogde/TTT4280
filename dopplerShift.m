function f_d = dopplerShift(f_0, v_r)
    c = 299792458;
    f_d = (2*f_0*v_r)/c;
end

import struct
from numpy import sqrt
from scipy.signal import max_len_seq

seq = max_len_seq(8)

with open("m_sequence.bin", "wb") as f:
    for s in seq[0]:
        s = float(s)
        if s == 0:
            s = -1
        s_real = s/sqrt(2)
        s_imag = s_real
        f.write(struct.pack("f", s_real))
        f.write(struct.pack("f", s_imag))


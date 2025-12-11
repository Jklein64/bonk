import numpy as np
import scipy as sp

t = np.linspace(0, 15, 15 * 1000000)
out = np.load("tmp/out.npy")
t = t[:len(out)]
# poor man's frequency detection
peaks, _ = sp.signal.find_peaks(out, height=0)
freq_est = np.mean(1 / (t[peaks[1:]] - t[peaks[:-1]]))
print(freq_est)

import matplotlib.pyplot as plt
plt.plot(t, out)
plt.show()
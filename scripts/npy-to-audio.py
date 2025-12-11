import numpy as np
from scipy.io import wavfile

out = np.load("tmp/out.npy")
out /= np.max(out)
wavfile.write("tmp/out.wav", rate=50_000, data=out.astype(np.float32))
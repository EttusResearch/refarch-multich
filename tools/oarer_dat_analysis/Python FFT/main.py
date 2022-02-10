import numpy as np
from matplotlib import pyplot


## set constants
fs = 1000
timestep = 1/fs
N = 10000

## Time domain plot
t = np.arange(1,N) * timestep
td_sig = 5*np.sin(100*np.pi*t) + np.random.randn(t.size)
window = np.hanning(len(td_sig))

## Frequency domain plot
fd_sig = np.fft.fft(td_sig*window)
freqs = np.fft.fftfreq(td_sig.size,d=timestep)
fft_mag = np.abs(fd_sig)
fft_phase = np.arctan(np.imag(fd_sig)/np.real(fd_sig))


# Find peak frequency on the positive end of the spectrum
pos_freq_mask = np.where(freqs > 0)
pos_freqs = freqs[pos_freq_mask]
peak_index = fft_mag[pos_freq_mask].argmax()
peak_freq = freqs[peak_index]
peak_phase = fft_phase[peak_index]

fd_sig_filtered = fd_sig.copy()
fd_sig_filtered[np.abs(freqs) > peak_freq+2*fs/N] = 0
fd_sig_filtered[np.abs(freqs) < peak_freq-2*fs/N] = 0


## Outputs signals
filtered_td_sig = np.fft.ifft(fd_sig_filtered)
residual_td_sig = td_sig - filtered_td_sig

## Get amplitude
td_sig_filtered_amplitude = np.real(np.sqrt(2*sum(filtered_td_sig**2/N)))

plt = pyplot
fig, axs = plt.subplots(2,2)
axs[0,0].set_title("Input Signal")
axs[0,0].plot(t,td_sig)
axs[1,0].set_title("FFT Mag")
axs[1,0].plot(freqs,fft_mag)
axs[0,1].set_title("Extracted Signal (Freq = %.2f, Amp. = %.2f)"%(peak_freq,td_sig_filtered_amplitude))
axs[0,1].plot(t,filtered_td_sig)
axs[1,1].set_title("Residual Signal")
axs[1,1].plot(t,residual_td_sig)
plt.savefig("test.png")    
plt.show()
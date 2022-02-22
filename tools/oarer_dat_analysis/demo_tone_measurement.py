from tone_measurement_module import ToneMeasurement
import numpy as np

##### Parameters #####
a = 3.5  # amplitude
f = 1  # frequency in Hz
phi = 50  # phase in degrees
N = 5000  # number of samples
fs = 100  # sample frequency in Hz
plot_data = True
add_noise = True
######################


time = np.arange(1, N) / fs
time_signal = a * np.sin((2 * np.pi * f * time) + np.pi * phi / 180)
if add_noise:
    noise = np.random.randn(N - 1)
    time_signal += noise


test = ToneMeasurement()
result = test.extract_single_tone_simple(
    time_signal,
    fs,
    plot=plot_data,
)

print(
    f"\nAmplitude: {result[0]:.2f}\nFrequency: {result[1]:.2f} Hz\nPhase: {result[2]:.2f} Degrees\n"
)

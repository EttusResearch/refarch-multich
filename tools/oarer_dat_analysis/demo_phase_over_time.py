import numpy as np
from tone_measurement_module import ToneMeasurement

##### Parameters #####
a = 5  # amplitude
f = 1  # frequency in Hz
phi = 1.25  # phase difference between signals
snr = 10  # signal to noise ratio (if noise enabled)
points_per_segment = 500  # datapoints per segment
N = 50000  # number of samples
fs = 100  # sample frequency in Hz
plot_result = True
add_noise = True
print_result = False
######################

time = np.arange(1, N) / fs
time_signal_0 = a * np.sin((2 * np.pi * f * time) + np.pi * phi / 180)
time_signal_1 = a * np.sin((2 * np.pi * f * time))


if add_noise:
    noise = np.random.randn(N - 1) * (a / snr)
    time_signal_0 += noise
    noise = np.random.randn(N - 1) * (a / snr)
    time_signal_1 += noise

test = ToneMeasurement()
phases_0 = test.extract_phase_over_time(time_signal_0, fs, points_per_segment)
phases_1 = test.extract_phase_over_time(time_signal_1, fs, points_per_segment)

diff = abs(phases_1 - phases_0)

if plot_result:
    from matplotlib import pyplot

    plt = pyplot
    fig, axs = plt.subplots(3, 1)
    axs[0].set_title("Input Signals")
    axs[0].plot(time_signal_0)
    axs[0].plot(time_signal_1)

    axs[1].set_title("Phases")
    axs[1].plot(phases_0)
    axs[1].plot(phases_1)
    axs[2].set_title("Phase Difference")
    axs[2].plot(diff)
    plt.show()
if print_result:
    print(diff)

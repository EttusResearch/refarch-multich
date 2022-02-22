import numpy as np
from matplotlib import pyplot


class ToneMeasurement:
    def __init__(self):
        pass

    def _windowed_fft(self, time_signal):
        fft_size = len(time_signal)
        h_window = np.hanning(fft_size) * 4 / (fft_size * np.sqrt(2))
        windowed_time_signal = np.multiply(h_window, time_signal)
        complex_spectrum = np.fft.fft(windowed_time_signal)
        complex_spectrum = complex_spectrum[
            0 : ((1 + fft_size) // 2)
        ]  ## get first half of spectrum, exclude nyquist
        complex_spectrum[0] = complex_spectrum[0] / np.sqrt(2)  ## correct DC
        return complex_spectrum

    def _three_fingersearch(
        self,
        complex_spectrum,
        fs,
        search_frequency=None,
        percent_tolerance=10,
    ):
        if not search_frequency == None:
            df = fs / len(complex_spectrum)
            search_frequency_index = search_frequency / df
            tolerance = percent_tolerance * 0.01 * search_frequency / df
            search_start_index = int((search_frequency_index - tolerance) // 1)
            search_end_index = int((search_frequency_index - tolerance) // 1 + 1)
            if search_start_index < 1:
                search_start_index = 1
            if search_end_index < 1:
                search_end_index = 1
            if search_end_index - search_start_index < 2:
                search_start_index = 0
                search_end_index = len(complex_spectrum)
                print(
                    "Unable to perform advanced search due to lack of frequency resolution. Please increase the sample size."
                )
        else:
            search_start_index = 0
            search_end_index = len(complex_spectrum)
        mag_spectrum = np.abs(complex_spectrum)
        peak_index = (
            np.argmax(mag_spectrum[search_start_index:search_end_index]) + search_start_index
        )
        three_fingers = [None] * 3
        for i in range(-1, 2):
            three_fingers[i + 1] = (peak_index + i, mag_spectrum[peak_index + i])
        return three_fingers

    def _amplitude_estimate(self, three_fingers):
        center_finger_index = three_fingers[1][0]
        magnitudes = [three_fingers[0][1], three_fingers[1][1], three_fingers[2][1]]
        if magnitudes[0] == magnitudes[2]:
            amplitude_estimate = magnitudes[1]
        else:
            if magnitudes[0] > magnitudes[2]:
                R0 = magnitudes[1] / magnitudes[0]
                R = (R0 - 2) / (R0 + 1)
            if magnitudes[0] < magnitudes[2]:
                R0 = magnitudes[1] / magnitudes[2]
                R = (2 - R0) / (R0 + 1)
            amplitude_estimate = magnitudes[1] * (1 - R * R) / np.sinc(R)
            peak_index_estimate = center_finger_index + R
        return peak_index_estimate, amplitude_estimate

    def _alias_correction(self, complex_spectrum, peak_index_estimate, peak_amplitude_estimate):
        fft_size = len(complex_spectrum)
        aa_complex_spectrum = complex_spectrum
        # Correct DC Aliasing
        for n in range(0, 4):
            index = int(peak_index_estimate // 1 - 1 + n)
            r = (
                peak_amplitude_estimate
                * np.sinc(index + peak_index_estimate)
                / ((index + peak_index_estimate) ** 2 - 1)
            )
            phi = -1 * np.angle(complex_spectrum[index] + np.pi)
            aa_complex_spectrum[index] - r * np.exp(complex(0, 1) * phi)

        # Correct Fs/2 Aliasing
        for n in range(0, 4):
            index = int(peak_index_estimate // 1 - 1 + n)
            r = (
                peak_amplitude_estimate
                * np.sinc(index + peak_index_estimate - fft_size)
                / ((index + peak_index_estimate - fft_size) ** 2 - 1)
            )
            phi = -1 * np.angle(complex_spectrum[index])
            aa_complex_spectrum[index] = complex_spectrum[index] - r * np.exp(complex(0, 1) * phi)

        return aa_complex_spectrum

    def extract_single_tone_simple(self, time_signal, fs, plot=False):
        raw_comp_spec = self._windowed_fft(time_signal)
        initial_three_fingers = self._three_fingersearch(raw_comp_spec, fs)
        peak_index_estimate, peak_amplitude_estimate = self._amplitude_estimate(
            initial_three_fingers
        )
        aa_comp_spec = self._alias_correction(
            raw_comp_spec, peak_index_estimate, peak_amplitude_estimate
        )
        aa_mag_spectrum = np.abs(aa_comp_spec)
        aa_three_fingers = self._three_fingersearch(aa_mag_spectrum, fs)
        peak_index, peakAmplitude = self._amplitude_estimate(aa_three_fingers)

        amplitude = peakAmplitude * np.sqrt(2)
        frequency = peak_index * (fs / len(time_signal))
        phase = (
            180 * np.angle(aa_comp_spec[int(peak_index_estimate // 1)]) / np.pi
            - 180 * (peak_index % 1)
            + 90
        )

        if plot:
            self._plotResult(time_signal, fs, (amplitude, frequency, phase))

        return amplitude, frequency, phase

    def extract_single_tone_advanced(
        self, time_signal, fs, search_frequency, percent_tolerance=10, plot=False
    ):
        raw_comp_spec = self._windowed_fft(time_signal)
        initial_three_fingers = self._three_fingersearch(
            raw_comp_spec,
            fs,
            search_frequency=search_frequency,
            percent_tolerance=percent_tolerance,
        )
        peak_index_estimate, peak_amplitude_estimate = self._amplitude_estimate(
            initial_three_fingers
        )
        aa_comp_spec = self._alias_correction(
            raw_comp_spec, peak_index_estimate, peak_amplitude_estimate
        )
        aa_mag_spectrum = np.abs(aa_comp_spec)
        aa_three_fingers = self._three_fingersearch(
            aa_mag_spectrum,
            fs,
            search_frequency=search_frequency,
            percent_tolerance=percent_tolerance,
        )
        peak_index, peakAmplitude = self._amplitude_estimate(aa_three_fingers)

        amplitude = peakAmplitude * np.sqrt(2)
        frequency = peak_index * (fs / len(time_signal))
        phase = (
            180 * np.angle(aa_comp_spec[int(peak_index_estimate // 1)]) / np.pi
            - 180 * (peak_index % 1)
            + 90
        )

        if plot:
            self._plotResult(time_signal, fs, (amplitude, frequency, phase))

        return amplitude, frequency, phase

    def _plotResult(self, time_signal, fs, extracted_signal_pars):
        time_array = np.arange(0, len(time_signal)) / fs

        estimated_amplitude = extracted_signal_pars[0]
        estimated_frequency = extracted_signal_pars[1]
        estimated_phase = extracted_signal_pars[2]
        extracted_signal = estimated_amplitude * np.sin(
            2 * np.pi * estimated_frequency * time_array + estimated_phase * np.pi / 180
        )

        ymin = np.min(time_signal) - np.abs(np.min(time_signal)) * 0.25
        ymax = np.max(time_signal) + np.abs(np.max(time_signal)) * 0.25

        plt = pyplot
        fig, axs = plt.subplots(3, 1)
        axs[0].set_title("Input Signal")
        axs[0].plot(time_array, time_signal)
        axs[0].set_ylim(ymin, ymax)
        axs[1].set_title(
            f"Extracted Signal: {estimated_amplitude:.1f}sin({estimated_frequency:.1f}*t + {estimated_phase:.1f} deg)"
        )
        axs[1].plot(time_array, extracted_signal)
        axs[1].set_ylim(ymin, ymax)
        axs[2].set_title("Residual Signal")
        axs[2].plot(time_array, time_signal - extracted_signal)
        axs[2].set_ylim(ymin, ymax)
        plt.show()

    def extract_phase_over_time(self, time_signal, fs, samples_per_segment):
        num_segments = len(time_signal) // samples_per_segment
        last_sample = num_segments * samples_per_segment
        segments = np.reshape(time_signal[0:last_sample], (num_segments, samples_per_segment))
        phases = np.array([])
        for segment in segments:
            result = self.extract_single_tone_simple(segment, fs)
            phases = np.append(phases, result[2])

        return phases

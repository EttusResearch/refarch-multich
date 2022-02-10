import numpy as np
from numpy import ma
from numpy.fft.helper import fftshift
from matplotlib import pyplot

class ToneMeasure:
    def __init__(self, signal, fs):
        self._rawCompSpec = []
        self._antiAliasedCompSpec = []
        self._rawMagSpec = []
        self._rawPeakIndex = None
        self._rawThreeFingers = []
        self._fs = fs
        self._fftSize = len(signal)
        self._signal = signal
        self._df = fs/len(signal)
        self._extractedSignal = np.arange(0,1)
        self._dt = 1/fs
        self._timeArray = np.arange(0,len(signal)) / fs

    def _computeWindowedFFT(self):
        hWindow = np.hanning(self._fftSize)*4/(self._fftSize*np.sqrt(2))
        windowedSignal = np.multiply(hWindow,self._signal)
        compSpec = np.fft.fft(windowedSignal)
        compSpec = compSpec[0:((1+self._fftSize)//2)] ## get first half of spectrum, exclude nyquist
        compSpec[0] = compSpec[0]/np.sqrt(2) ## correct DC
        return compSpec

    def _threeFingerSearch(self, compSpec, advanced = False, searchFrequency = None, searchTolerancePercent = None):
        if advanced:
            searchFreqIndex = searchFrequency/self._df
            tolerance = searchTolerancePercent*.01*searchFrequency/self._df
            searchStartIndex = int((searchFreqIndex-tolerance)//1)
            searchEndIndex = int((searchFreqIndex-tolerance)//1 + 1)
            if searchStartIndex < 1: searchStartIndex = 1
            if searchEndIndex < 1: searchEndIndex = 1
        else: 
            searchStartIndex = 0
            searchEndIndex = self._fftSize
        magSpectrum = np.abs(compSpec)
        peakIndex = np.argmax(magSpectrum[searchStartIndex:searchEndIndex]) + searchStartIndex
        threeFingers = [None] * 3
        for i in range(-1,2):
            threeFingers[i+1] = (peakIndex+i,magSpectrum[peakIndex+i])
        return threeFingers

    def _computeAmpEstimate(self, threeFingers):
        print(threeFingers)
        midFingerIndex = threeFingers[1][0]
        magnitudes = [threeFingers[0][1], threeFingers[1][1], threeFingers[2][1]]
        if magnitudes[0] == magnitudes[2]:
            ampEstimate = magnitudes[1]
            estimateIndex = midFingerIndex
        else: 
            if magnitudes[0] > magnitudes[2]:
                R0 = magnitudes[1]/magnitudes[0]
                R = (R0-2)/(R0+1)
            if magnitudes[0] < magnitudes[2]:
                R0 = magnitudes[1]/magnitudes[2]
                R = (2-R0)/(R0+1)
            ampEstimate = magnitudes[1]*(1-R*R)/np.sinc(R)
            peakIndexEstimate = midFingerIndex + R
        return peakIndexEstimate, ampEstimate
    
    def _initalEstimateAliasCorrection(self, compSpec, peakIndexEstimate, peakAmpEstimate):
        antiAliasedCompSpec = compSpec
        # Correct DC Aliasing
        for n in range(0,4):
            index = int(peakIndexEstimate//1 - 1 + n)
            r = peakAmpEstimate*np.sinc(index + peakIndexEstimate)/((index+peakIndexEstimate)**2 -1)
            phi = -1*np.angle(compSpec[index] + np.pi)
            antiAliasedCompSpec[index] - r*np.exp(complex(0,1)*phi)

        # Correct Fs/2 Aliasing
        for n in range(0,4):
            index = int(peakIndexEstimate//1 - 1 + n)
            r = peakAmpEstimate*np.sinc(index + peakIndexEstimate - self._fftSize)/((index + peakIndexEstimate - self._fftSize)**2 -1)
            phi = -1*np.angle(compSpec[index])
            antiAliasedCompSpec[index] = compSpec[index] - r*np.exp(complex(0,1)*phi)

        return antiAliasedCompSpec

    def extractSingleToneSimple(self):
        self._rawCompSpec = self._computeWindowedFFT()
        initialThreeFingers = self._threeFingerSearch(self._rawCompSpec)
        peakIndexEstimate, peakAmpEstimate = self._computeAmpEstimate(initialThreeFingers)
        print(peakIndexEstimate)
        self._antiAliasedCompSpec = self._initalEstimateAliasCorrection(self._rawCompSpec, peakIndexEstimate, peakAmpEstimate)
        antiAliasedMagSpec = np.abs(self._antiAliasedCompSpec)
        antiAliasedFingers = self._threeFingerSearch(antiAliasedMagSpec)
        peakIndex, peakAmplitude = self._computeAmpEstimate(antiAliasedFingers)

        amplitude = peakAmplitude*np.sqrt(2)
        frequency = peakIndex*self._df
        phase = 180*np.angle(self._antiAliasedCompSpec[int(peakIndexEstimate//1)])/np.pi - 180*(peakIndex % 1) + 90 
        
        self._extractedSignal = amplitude*np.sin(2*np.pi*frequency*self._timeArray+phase*np.pi/180)
        
        print(self._extractedSignal)
        return amplitude, frequency, phase

    def plotResult(self):
        plt = pyplot
        fig, axs = plt.subplots(2,2)
        axs[0,0].set_title("Input Signal")
        axs[0,0].plot(self._timeArray,self._signal)
        axs[0,0].set_ylim(-1.25,1.25)
        axs[1,0].set_title("Extracted Signal")
        axs[1,0].plot(self._timeArray,self._extractedSignal)
        axs[1,0].set_ylim(-1.25,1.25)
        axs[0,1].set_title("Residual Signal")
        axs[0,1].plot(self._timeArray,self._signal-self._extractedSignal)
        axs[0,1].set_ylim(-1.25,1.25)
        plt.show()


signal = np.genfromtxt("C:\\GitHub\\PythonSandbox\\FFT Python\\Python FFT\\Signals\\f=70000 fs=1050000.csv",delimiter=",")
fs = 1050000
test = ToneMeasure(signal, fs)
test.extractSingleToneSimple()
test.plotResult()





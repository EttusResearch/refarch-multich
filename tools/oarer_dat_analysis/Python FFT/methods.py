import numpy as np
from numpy.core.fromnumeric import searchsorted
from numpy.fft import fft
from matplotlib import pyplot
from numpy.fft.helper import fftshift

def getWindowedFFT(signal, fs):
    fftSize = len(signal)
    hWindow = np.hanning(fftSize)*4/(fftSize*np.sqrt(2))
    windowedSignal = multiplyElementWise(hWindow,signal)
    complexSpectrum = np.fft.fft(windowedSignal)
    complexSpectrum = complexSpectrum[0:((1+fftSize)//2)] ## get first half of spectrum, exclude nyquist
    complexSpectrum[0] = complexSpectrum[0]/np.sqrt(2) ## correct DC

    return complexSpectrum, fftSize


def extract3Bins(magSpectrum, fftSize, fs, advancedSearch = (-1, -1)):
    df = fs/fftSize
    approxFreq = advancedSearch[0]
    percentTolerance = advancedSearch[1]

    if approxFreq > 0:
        approxFreqIndex = approxFreq/df
        tolerance = percentTolerance*.01*approxFreq/df
        searchStart = int((approxFreqIndex-tolerance)//1)
        searchEnd = int((approxFreqIndex+tolerance)//1 + 1)
        if searchStart < 1: searchStart = 1
        if searchEnd < 1: searchEnd = 1

    else: 
        searchStart = 0
        searchEnd = fftSize
    print(searchStart,searchEnd)
    peakIndex = np.argmax(magSpectrum[searchStart:searchEnd]) + searchStart
    bins = (magSpectrum[peakIndex-1], magSpectrum[peakIndex], magSpectrum[peakIndex+1])
    return peakIndex, bins


def firstEstimate(peakIndex, bins):
    if bins[0] > bins[2]:
        R0 = bins[1]/bins[0]
        R = (R0-2)/(R0+1)
        amplitude = bins[1]*(1-R*R)/np.sinc(R)
        peakFreqLocation = peakIndex + R
    elif bins[2] > bins[0]:
        R0 = bins[1]/bins[2]
        R = (2-R0)/(R0+1)
        amplitude = bins[1]*(1-R*R)/np.sinc(np.pi*R)
        peakFreqLocation = peakIndex + R
    else:
        amplitude = bins[1]
        peakFreqLocation = peakIndex
    
    return peakFreqLocation, amplitude

def correctForAliasing(complexSpectrum, estimatedPeakIndex, estimatedAmplitude, fftSize):
    # Correct DC Aliasing
    for n in range(0,4):
        index = int(estimatedPeakIndex//1 - 1 + n)
        r = estimatedAmplitude*np.sinc(index + estimatedPeakIndex)/((index+estimatedPeakIndex)**2 -1)
        phi = -1*np.angle(complexSpectrum[index] + np.pi)
        complexSpectrum[index] - r*np.exp(complex(0,1)*phi)

    # Correct Fs/2 Aliasing
    for n in range(0,4):
        index = int(estimatedPeakIndex//1 - 1 + n)
        r = estimatedAmplitude*np.sinc(index + estimatedPeakIndex - fftSize)/((index + estimatedPeakIndex - fftSize)**2 -1)
        phi = -1*np.angle(complexSpectrum[index])
        complexSpectrum[index] = complexSpectrum[index] - r*np.exp(complex(0,1)*phi)

    return complexSpectrum

def multiplyElementWise(A,B):
    return [x*y for x,y in zip(A,B)]

def loadSignal(path):
    data = np.genfromtxt(path, delimiter=',')
    return data

##############
signal = np.genfromtxt("C:\\GitHub\\PythonSandbox\\FFT Python\\Python FFT\\Signals\\f=70000 fs=1050000.csv",delimiter=",")
fs = 1050000
timestep = 1/fs
N = len(signal)
t = np.arange(0,N) * timestep


spectrum, fftSize = getWindowedFFT(signal, fs)
magSpectrum = np.abs(spectrum)
rawPeakIndex, domBins = extract3Bins(magSpectrum, fftSize, fs)
estimatedPeakIndex1, estimatedAmplitude = firstEstimate(rawPeakIndex, domBins)
print(estimatedPeakIndex1)
spectrum = correctForAliasing(spectrum, estimatedPeakIndex1, estimatedAmplitude, fftSize)
newDomBins = [np.abs(spectrum[rawPeakIndex-1]),np.abs(spectrum[rawPeakIndex]), np.abs(spectrum[rawPeakIndex+1])]
estimatedPeakIndex2, estimatedAmplitude = firstEstimate(rawPeakIndex, domBins)

amplitude = estimatedAmplitude*np.sqrt(2)
frequency = estimatedPeakIndex2*fs/fftSize
phase = 180*np.angle(spectrum[int(estimatedPeakIndex1//1)])/np.pi - 180*(estimatedPeakIndex2 % 1) + 90 

print(amplitude, phase)
extractedSignal = amplitude*np.sin(2*np.pi*frequency*t+phase*np.pi/180)

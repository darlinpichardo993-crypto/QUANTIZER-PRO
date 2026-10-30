#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_audio_formats/juce_audio_formats.h>
#include <juce_audio_utils/juce_audio_utils.h>
#include <juce_dsp/juce_dsp.h>
#include "QuantEngine.h"

class QuantizerProcessor : public juce::AudioProcessor,
                           public juce::ChangeBroadcaster
{
public:
    QuantizerProcessor();
    ~QuantizerProcessor() override;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout&) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }
    const juce::String getName() const override { return "Quantizer Pro"; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }
    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}
    void getStateInformation (juce::MemoryBlock&) override;
    void setStateInformation (const void*, int) override;

    // ---- API para la interfaz ----
    void loadAudioFile (const juce::File& f);
    void startCapture();
    void stopCaptureAndAnalyze();
    void analyze();
    void quantizeNow();
    bool exportQuantizedWav (const juce::File& f);
    void clearClip();

    std::shared_ptr<const qp::Clip> getClip() const { const juce::SpinLock::ScopedLockType l (clipLock); return clip; }
    qp::Params readParams (const qp::Clip* c) const;
    double effectiveBpm() const;

    juce::AudioProcessorValueTreeState apvts;

    std::atomic<bool>   busy { false }, capturing { false }, hostPlaying { false };
    std::atomic<float>  peakL { 0.f }, peakR { 0.f };
    std::atomic<double> hostBpm { 0.0 }, hostPosSamples { 0.0 };
    std::atomic<int>    capturedSamples { 0 };
    std::atomic<int>    quantizeStamp { 0 };     // sube cada vez que se cuantiza (anima las lineas)
    double sampleRateNow() const { return currentSr; }

private:
    static juce::AudioProcessorValueTreeState::ParameterLayout createLayout();
    void setClip (std::shared_ptr<qp::Clip> c);
    void runAnalysis (std::shared_ptr<const qp::Clip> c, bool thenQuantize);

    mutable juce::SpinLock clipLock;
    std::shared_ptr<const qp::Clip> clip;
    double currentSr = 44100.0;

    // captura en vivo
    juce::AudioBuffer<float> capBuf;
    std::atomic<int> capLen { 0 };
    std::atomic<long long> capStart { -1 };

    juce::CriticalSection pathLock;
    juce::String audioPath;
    float lastGain = 1.f;
    juce::ThreadPool pool { 1 };
    juce::AudioFormatManager formats;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (QuantizerProcessor)
};

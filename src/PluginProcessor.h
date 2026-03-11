#pragma once

#include <JuceHeader.h>

class KimuVerbAudioProcessor : public juce::AudioProcessor
{
public:
    KimuVerbAudioProcessor();
    ~KimuVerbAudioProcessor() override;

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

   #ifndef JucePlugin_PreferredChannelConfigurations
    bool isBusesLayoutSupported(const BusesLayout& layouts) const override;
   #endif

    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    const juce::String getName() const override;

    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram(int index) override;
    const juce::String getProgramName(int index) override;
    void changeProgramName(int index, const juce::String& newName) override;

    void getStateInformation(juce::MemoryBlock& destData) override;
    void setStateInformation(const void* data, int sizeInBytes) override;

    using APVTS = juce::AudioProcessorValueTreeState;
    APVTS& getValueTreeState() { return parameters; }
    static APVTS::ParameterLayout createParameterLayout();

    void rtLog(const juce::String& msg) noexcept;

private:
    APVTS parameters;

    juce::dsp::Reverb reverb;
    juce::dsp::ProcessSpec spec;

    std::vector<float> preDelayBufferL;
    std::vector<float> preDelayBufferR;
    size_t preDelayWriteL = 0;
    size_t preDelayWriteR = 0;

    juce::AudioBuffer<float> dryBuffer;

    juce::dsp::LinkwitzRileyFilter<float> highPass;
    juce::dsp::LinkwitzRileyFilter<float> lowPass;
    juce::dsp::Limiter<float> limiter;

    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> mixSmoothed;
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> preDelayMsSmoothed;

    float envL = 0.0f;
    float envR = 0.0f;

    std::unique_ptr<juce::FileLogger> logger;

    // RT-safe ring buffer log and background flusher
    struct LogRing
    {
        explicit LogRing(int cap) : fifo(cap), buffer((size_t)cap) {}
        juce::AbstractFifo fifo;
        std::vector<juce::String> buffer;
        void push(const juce::String& s) noexcept
        {
            int start1, size1, start2, size2; fifo.prepareToWrite(1, start1, size1, start2, size2);
            if (size1 > 0) buffer[(size_t)start1] = s; // overwrite
            fifo.write(juce::jmin(1, size1 + size2));
        }
        bool pop(juce::String& out) noexcept
        {
            int start1, size1, start2, size2; fifo.prepareToRead(1, start1, size1, start2, size2);
            if (size1 <= 0) return false;
            out = buffer[(size_t)start1];
            fifo.read(1);
            return true;
        }
    };
    std::unique_ptr<LogRing> logRing;

    class LogFlushThread : public juce::Thread
    {
    public:
        LogFlushThread(LogRing& r, std::unique_ptr<juce::FileLogger>& lg, std::atomic<bool>& en)
        : juce::Thread("KimuVerbLogFlush"), ring(r), loggerRef(lg.get()), enabled(en) {}
        void run() override
        {
            juce::String msg;
            while (! threadShouldExit())
            {
                if (enabled.load())
                {
                    int drained = 0;
                    while (ring.pop(msg))
                    {
                        if (loggerRef != nullptr) loggerRef->writeToLog(msg);
                        if (++drained > 256) break; // avoid long stalls
                    }
                }
                wait(50);
            }
        }
    private:
        LogRing& ring;
        juce::FileLogger* loggerRef;
        std::atomic<bool>& enabled;
    };
    std::unique_ptr<LogFlushThread> logThread;
    std::atomic<bool> loggingEnabled { true };

    struct ParamChangeLogger : public juce::AudioProcessorValueTreeState::Listener
    {
        KimuVerbAudioProcessor* owner = nullptr;
        void parameterChanged(const juce::String& parameterID, float newValue) override
        {
            if (owner) owner->rtLog(juce::String("param ") + parameterID + "=" + juce::String(newValue));
        }
    };
    std::unique_ptr<ParamChangeLogger> paramChangeLogger;

    void updateReverbParams();
    void updateFilters();
    void updateSmoothers();

    float computeDuckingGain(float dryEnv, float duckAmt, float thresholdDb, float releaseMs, double sr) const;

    // Cached raw parameter pointers for RT-safe access
    std::atomic<float>* paramMix = nullptr;
    std::atomic<float>* paramSize = nullptr;
    std::atomic<float>* paramDamping = nullptr;
    std::atomic<float>* paramWidth = nullptr;
    std::atomic<float>* paramPreDelay = nullptr;
    std::atomic<float>* paramLowCut = nullptr;
    std::atomic<float>* paramHighCut = nullptr;
    std::atomic<float>* paramDuck = nullptr;
    std::atomic<float>* paramDuckThresh = nullptr;
    std::atomic<float>* paramDuckRel = nullptr;
    std::atomic<float>* paramOutGain = nullptr;
    std::atomic<float>* paramLogging = nullptr;

    static float readParam(std::atomic<float>* p, float fallback) noexcept
    {
        return (p != nullptr) ? p->load(std::memory_order_relaxed) : fallback;
    }

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(KimuVerbAudioProcessor)
};

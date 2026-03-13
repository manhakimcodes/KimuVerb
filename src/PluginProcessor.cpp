#include "PluginProcessor.h"
#include "PluginEditor.h"
#include <cmath>

KimuVerbAudioProcessor::KimuVerbAudioProcessor()
    : juce::AudioProcessor(BusesProperties()
        .withInput ("Input", juce::AudioChannelSet::stereo(), true)
        .withOutput("Output", juce::AudioChannelSet::stereo(), true))
    , parameters(*this, nullptr, juce::Identifier("KimuVerbParams"), createParameterLayout())
{
    auto dir = juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory)
        .getChildFile("KimuAudio").getChildFile("KimuVerb").getChildFile("logs");
    if (dir.createDirectory())
    {
        logger.reset(new juce::FileLogger(dir.getChildFile("session.log"), "KimuVerb", 1024 * 1024));
        if (logger != nullptr)
            juce::Logger::setCurrentLogger(logger.get());
        else
            loggingEnabled.store(false);
    }
    else
    {
        loggingEnabled.store(false);
    }

    // Init RT-safe log ring and background flusher
    logRing = std::make_unique<LogRing>(2048);
    logThread = std::make_unique<LogFlushThread>(*logRing, logger, loggingEnabled);
    logThread->startThread(juce::Thread::Priority::low);

    // Parameter listeners for logging (RT-safe via rtLog)
    paramChangeLogger = std::make_unique<ParamChangeLogger>();
    paramChangeLogger->owner = this;
    const char* ids[] = { "size","damping","width","mix","predelay","lowcut","highcut","duck","duckthresh","duckrel","outgain","logging" };
    for (auto* id : ids) parameters.addParameterListener(id, paramChangeLogger.get());

    // Honour initial logging state
    paramMix = parameters.getRawParameterValue("mix");
    paramSize = parameters.getRawParameterValue("size");
    paramDamping = parameters.getRawParameterValue("damping");
    paramWidth = parameters.getRawParameterValue("width");
    paramPreDelay = parameters.getRawParameterValue("predelay");
    paramLowCut = parameters.getRawParameterValue("lowcut");
    paramHighCut = parameters.getRawParameterValue("highcut");
    paramDuck = parameters.getRawParameterValue("duck");
    paramDuckThresh = parameters.getRawParameterValue("duckthresh");
    paramDuckRel = parameters.getRawParameterValue("duckrel");
    paramOutGain = parameters.getRawParameterValue("outgain");
    paramLogging = parameters.getRawParameterValue("logging");

    if (paramLogging != nullptr)
        loggingEnabled.store(paramLogging->load(std::memory_order_relaxed) >= 0.5f);

    rtLog("processor constructed");
}

KimuVerbAudioProcessor::~KimuVerbAudioProcessor()
{
    rtLog("processor destructing");
    const char* ids[] = { "size","damping","width","mix","predelay","lowcut","highcut","duck","duckthresh","duckrel","outgain","logging" };
    if (paramChangeLogger != nullptr)
        for (auto* id : ids) parameters.removeParameterListener(id, paramChangeLogger.get());
    if (logThread) { logThread->stopThread(500); logThread.reset(); }
    paramChangeLogger.reset();
    logRing.reset();
    juce::Logger::setCurrentLogger(nullptr);
}

const juce::String KimuVerbAudioProcessor::getName() const { return JucePlugin_Name; }

void KimuVerbAudioProcessor::rtLog(const juce::String& msg) noexcept
{
    if (! logRing) return;
    if (! loggingEnabled.load(std::memory_order_relaxed)) return;
    logRing->push(juce::Time::getCurrentTime().toString(true, true, true, true) + " | " + msg);
}

bool KimuVerbAudioProcessor::acceptsMidi() const { return false; }
bool KimuVerbAudioProcessor::producesMidi() const { return false; }
bool KimuVerbAudioProcessor::isMidiEffect() const { return false; }

double KimuVerbAudioProcessor::getTailLengthSeconds() const { return 10.0; }

int KimuVerbAudioProcessor::getNumPrograms() { return 1; }
int KimuVerbAudioProcessor::getCurrentProgram() { return 0; }
void KimuVerbAudioProcessor::setCurrentProgram(int) {}
const juce::String KimuVerbAudioProcessor::getProgramName(int) { return {}; }
void KimuVerbAudioProcessor::changeProgramName(int, const juce::String&) {}

void KimuVerbAudioProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    rtLog("prepareToPlay sr=" + juce::String(sampleRate) + ", block=" + juce::String(samplesPerBlock));

    spec.sampleRate = sampleRate;
    spec.maximumBlockSize = static_cast<juce::uint32>(samplesPerBlock);
    spec.numChannels = static_cast<juce::uint32>(juce::jmax(1, getTotalNumOutputChannels()));

    reverb.prepare(spec);
    highPass.reset();
    lowPass.reset();
    limiter.reset();
    highPass.prepare(spec);
    lowPass.prepare(spec);
    limiter.prepare(spec);

    updateReverbParams();
    updateFilters();

    mixSmoothed.reset(sampleRate, 0.05);
    preDelayMsSmoothed.reset(sampleRate, 0.05);

    int maxPreDelayMs = 400;
    size_t maxSamples = static_cast<size_t>(sampleRate * maxPreDelayMs / 1000.0) + samplesPerBlock + 1;
    preDelayBufferL.assign(maxSamples, 0.0f);
    preDelayBufferR.assign(maxSamples, 0.0f);
    preDelayWriteL = 0;
    preDelayWriteR = 0;

    dryBuffer.setSize(juce::jmax(1, getTotalNumOutputChannels()), samplesPerBlock);
}

void KimuVerbAudioProcessor::releaseResources()
{
    rtLog("releaseResources");
    reverb.reset();
    highPass.reset();
    lowPass.reset();
    limiter.reset();
    preDelayBufferL.clear();
    preDelayBufferR.clear();
    dryBuffer.setSize(0, 0);
}

#ifndef JucePlugin_PreferredChannelConfigurations
bool KimuVerbAudioProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
{
    if (layouts.getMainOutputChannelSet() == juce::AudioChannelSet::mono()
        || layouts.getMainOutputChannelSet() == juce::AudioChannelSet::stereo())
        return true;
    return false;
}
#endif

void KimuVerbAudioProcessor::updateReverbParams()
{
    juce::dsp::Reverb::Parameters p;
    p.roomSize = readParam(paramSize, 0.8f);
    p.damping = readParam(paramDamping, 0.5f);
    p.width = readParam(paramWidth, 1.0f);
    p.freezeMode = 0.0f;
    reverb.setParameters(p);
}

void KimuVerbAudioProcessor::updateFilters()
{
    auto hp = readParam(paramLowCut, 40.0f);
    auto lp = readParam(paramHighCut, 18000.0f);
    highPass.setType(juce::dsp::LinkwitzRileyFilterType::highpass);
    lowPass.setType(juce::dsp::LinkwitzRileyFilterType::lowpass);
    highPass.setCutoffFrequency(hp);
    lowPass.setCutoffFrequency(lp);
}

void KimuVerbAudioProcessor::updateSmoothers()
{
    mixSmoothed.setTargetValue(readParam(paramMix, 0.4f));
    preDelayMsSmoothed.setTargetValue(readParam(paramPreDelay, 20.0f));
}

float KimuVerbAudioProcessor::computeDuckingGain(float dryEnv, float duckAmt, float thresholdDb, float releaseMs, double sr) const
{
    float db = juce::Decibels::gainToDecibels(juce::jlimit(1.0e-6f, 10.0f, dryEnv));
    float over = db - thresholdDb;
    float comp = juce::jlimit(0.0f, 1.0f, over / 24.0f);
    float target = 1.0f - duckAmt * comp;
    return target;
}

void KimuVerbAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    try
    {
        juce::ScopedNoDenormals noDenormals;
        auto numChannels = buffer.getNumChannels();
        auto numSamples = buffer.getNumSamples();

        // Read logging toggle atomically once per block
        if (paramLogging != nullptr)
            loggingEnabled.store(paramLogging->load(std::memory_order_relaxed) >= 0.5f, std::memory_order_relaxed);

        if (dryBuffer.getNumSamples() < numSamples)
            dryBuffer.setSize(juce::jmax(1, getTotalNumOutputChannels()), numSamples, true, true, true);
        dryBuffer.makeCopyOf(buffer);

        updateReverbParams();
        updateFilters();
        updateSmoothers();

        auto duckAmt = readParam(paramDuck, 0.0f);
        auto duckThresh = readParam(paramDuckThresh, -18.0f);
        auto duckRelease = readParam(paramDuckRel, 120.0f);
        auto outGainDb = readParam(paramOutGain, 0.0f);

        juce::HeapBlock<size_t> preDelaySamples;
        preDelaySamples.malloc((size_t) numSamples);
        juce::HeapBlock<float> mixValues;
        mixValues.malloc((size_t) numSamples);
        for (int n = 0; n < numSamples; ++n)
        {
            const float preMs = preDelayMsSmoothed.getNextValue();
            size_t preSamp = static_cast<size_t>(getSampleRate() * preMs / 1000.0);
            preDelaySamples[n] = preSamp;
            mixValues[n] = mixSmoothed.getNextValue();
        }

        for (int ch = 0; ch < numChannels; ++ch)
        {
            float* data = buffer.getWritePointer(ch);
            auto& ring = (ch == 0 ? preDelayBufferL : preDelayBufferR);
            auto& writeIdx = (ch == 0 ? preDelayWriteL : preDelayWriteR);
            const size_t ringSize = ring.size();
            if (ringSize == 0)
                continue;

            for (int n = 0; n < numSamples; ++n)
            {
                size_t preSamp = preDelaySamples[n];
                if (preSamp >= ringSize)
                    preSamp = ringSize - 1;
                size_t readIdx = (writeIdx + ringSize - preSamp) % ringSize;
                float in = data[n];
                float delayed = ring[readIdx];
                ring[writeIdx] = in;
                data[n] = (preSamp > 0 ? delayed : in);
                writeIdx = (writeIdx + 1) % ringSize;
            }
        }

        juce::dsp::AudioBlock<float> block(buffer);
        juce::dsp::ProcessContextReplacing<float> ctx(block);
        reverb.process(ctx);

        float alpha = std::exp(-1.0f / (0.001f * juce::jmax(1.0f, duckRelease) * (float)getSampleRate()));
        for (int n = 0; n < numSamples; ++n)
        {
            float dl = dryBuffer.getSample(0, n);
            float dr = dryBuffer.getNumChannels() > 1 ? dryBuffer.getSample(1, n) : dl;
            dl = std::abs(dl);
            dr = std::abs(dr);
            envL = juce::jmax(dl, envL * alpha + (1.0f - alpha) * dl);
            envR = juce::jmax(dr, envR * alpha + (1.0f - alpha) * dr);
        }
        float dryEnv = 0.5f * (envL + envR);
        float duckGain = computeDuckingGain(dryEnv, duckAmt, duckThresh, duckRelease, getSampleRate());

        for (int ch = 0; ch < numChannels; ++ch)
        {
            float* wetPtr = buffer.getWritePointer(ch);
            const float* dryPtr = dryBuffer.getReadPointer(juce::jmin(ch, dryBuffer.getNumChannels() - 1));
            for (int n = 0; n < numSamples; ++n)
            {
                float wet = wetPtr[n] * duckGain;
                float dry = dryPtr[n];
                float mix = mixValues[n];
                wetPtr[n] = dry + (wet - dry) * mix;
            }
        }

        // No limiter in reverb path to avoid coloration/distortion.

        float outG = juce::Decibels::decibelsToGain(outGainDb);
        for (int ch = 0; ch < numChannels; ++ch)
        {
            float* data = buffer.getWritePointer(ch);
            for (int n = 0; n < numSamples; ++n)
            {
                float v = data[n] * outG * 0.8f;
                // Gentle soft clip to prevent hard clipping without a limiter.
                data[n] = std::tanh(v);
            }
        }
    }
    catch (const std::bad_alloc& e)
    {
        rtLog(juce::String("processBlock bad_alloc: ") + e.what());
        buffer.clear();
    }
    catch (const std::exception& e)
    {
        rtLog(juce::String("processBlock exception: ") + e.what());
        buffer.clear();
    }
    catch (...)
    {
        rtLog("processBlock unknown exception");
        buffer.clear();
    }
}

bool KimuVerbAudioProcessor::hasEditor() const { return true; }
juce::AudioProcessorEditor* KimuVerbAudioProcessor::createEditor() { return new KimuVerbAudioProcessorEditor(*this); }

void KimuVerbAudioProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    rtLog("getStateInformation");
    auto state = parameters.copyState();
    juce::MemoryOutputStream mos(destData, true);
    state.writeToStream(mos);
}

void KimuVerbAudioProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    rtLog("setStateInformation size=" + juce::String(sizeInBytes));
    auto tree = juce::ValueTree::readFromData(data, static_cast<size_t>(sizeInBytes));
    if (tree.isValid())
        parameters.replaceState(tree);
}

KimuVerbAudioProcessor::APVTS::ParameterLayout KimuVerbAudioProcessor::createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    // Main parameters (orca body mapping)
    params.push_back(std::make_unique<juce::AudioParameterFloat>("mix", "Mix", juce::NormalisableRange<float>(0.0f, 1.0f), 0.4f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("size", "Size", juce::NormalisableRange<float>(0.0f, 1.0f), 0.8f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("decay", "Decay", juce::NormalisableRange<float>(0.1f, 20.0f), 6.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("predelay", "PreDelay", juce::NormalisableRange<float>(0.0f, 200.0f), 20.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("depth", "Depth", juce::NormalisableRange<float>(0.0f, 1.0f), 0.5f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("damping", "Damping", juce::NormalisableRange<float>(0.0f, 1.0f), 0.5f));
    
    // Advanced parameters
    params.push_back(std::make_unique<juce::AudioParameterFloat>("tone", "Tone", juce::NormalisableRange<float>(-1.0f, 1.0f), 0.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("width", "Width", juce::NormalisableRange<float>(0.0f, 2.0f), 1.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("lowcut", "LowCut", juce::NormalisableRange<float>(20.0f, 500.0f, 0.0f, 0.5f), 40.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("highcut", "HighCut", juce::NormalisableRange<float>(2000.0f, 20000.0f, 0.0f, 0.5f), 18000.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("motion", "Motion", juce::NormalisableRange<float>(0.0f, 1.0f), 0.3f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("diffusion", "Diffusion", juce::NormalisableRange<float>(0.0f, 1.0f), 0.7f));
    
    // Oceanic controls
    params.push_back(std::make_unique<juce::AudioParameterFloat>("current", "Current", juce::NormalisableRange<float>(0.0f, 1.0f), 0.2f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("pressure", "Pressure", juce::NormalisableRange<float>(0.0f, 1.0f), 0.5f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("depthshift", "DepthShift", juce::NormalisableRange<float>(0.0f, 1.0f), 0.3f));

    params.push_back(std::make_unique<juce::AudioParameterBool>("logging", "Logging", true));

    return { params.begin(), params.end() };
}
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new KimuVerbAudioProcessor();
}

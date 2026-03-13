#include "PluginEditor.h"
#include <BinaryData.h>

struct KimuVerbAudioProcessorEditor::SampleData
{
    juce::AudioBuffer<float> buffer;
    double sampleRate = 44100.0;
};

struct KimuVerbAudioProcessorEditor::UiSampleSource : public juce::AudioSource
{
    void prepareToPlay(int, double sampleRate) override { sr = sampleRate > 0.0 ? sampleRate : 44100.0; }
    void releaseResources() override {}

    void setSample(std::shared_ptr<SampleData> data)
    {
        sampleData = std::move(data);
    }

    void trigger(double durationSeconds, float gain, double startOffsetSeconds = 0.0)
    {
        if (! sampleData || sampleData->buffer.getNumSamples() == 0)
            return;

        const auto srcRate = sampleData->sampleRate > 0.0 ? sampleData->sampleRate : 44100.0;
        const auto srcLen = sampleData->buffer.getNumSamples();

        const auto startSample = (int) std::round(startOffsetSeconds * srcRate);
        const auto maxPlayable = juce::jlimit(0, srcLen - 1, srcLen - startSample);
        const auto durationSamples = (int) std::round(durationSeconds * srcRate);

        srcStart = juce::jlimit(0, srcLen - 1, startSample);
        srcLength = juce::jmax(1, juce::jmin(maxPlayable, durationSamples));
        srcPos = 0.0;
        srcStep = srcRate / sr;
        level = juce::jlimit(0.0f, 1.0f, gain);
        active = true;
    }

    void getNextAudioBlock(const juce::AudioSourceChannelInfo& bufferToFill) override
    {
        bufferToFill.clearActiveBufferRegion();
        if (! active || sr <= 0.0 || ! sampleData)
            return;

        auto* out = bufferToFill.buffer;
        auto& in = sampleData->buffer;
        const int outCh = out->getNumChannels();
        const int inCh = in.getNumChannels();
        const int numSamples = bufferToFill.numSamples;
        const int start = bufferToFill.startSample;

        for (int i = 0; i < numSamples; ++i)
        {
            if ((int) std::floor(srcPos) >= srcLength)
            {
                active = false;
                break;
            }

            const int idx = srcStart + (int) std::floor(srcPos);
            float sample = 0.0f;
            if (idx >= 0 && idx < in.getNumSamples())
            {
                const float s0 = in.getSample(0, idx);
                const float s1 = (inCh > 1) ? in.getSample(1, idx) : s0;
                sample = (s0 + s1) * 0.5f;
            }

            const float t = (float) srcPos / (float) juce::jmax(1, srcLength);
            const float fadeIn = juce::jmin(1.0f, t / 0.03f);
            const float fadeOut = juce::jmin(1.0f, (1.0f - t) / 0.08f);
            const float env = fadeIn * fadeOut;
            sample *= level * env;

            for (int ch = 0; ch < outCh; ++ch)
                out->addSample(ch, start + i, sample);

            srcPos += srcStep;
        }
    }

    double sr = 44100.0;
    float level = 0.1f;
    double srcPos = 0.0;
    double srcStep = 1.0;
    int srcStart = 0;
    int srcLength = 0;
    bool active = false;
    std::shared_ptr<SampleData> sampleData;
};

struct KimuVerbAudioProcessorEditor::OrcaTooltip : public juce::Component
{
    OrcaTooltip()
    {
        setInterceptsMouseClicks(false, false);
    }

    void setText(const juce::String& newTitle, const juce::String& newContent)
    {
        title = newTitle;
        content = newContent;
        repaint();
    }

    void paint(juce::Graphics& g) override
    {
        auto bounds = getLocalBounds().toFloat();

        g.setColour(juce::Colour(10, 20, 35).withAlpha(0.92f));
        g.fillRoundedRectangle(bounds, 8.0f);

        g.setColour(juce::Colour(180, 220, 255).withAlpha(0.9f));
        g.drawRoundedRectangle(bounds, 8.0f, 1.2f);

        g.setColour(juce::Colour(255, 255, 255));
        g.setFont(juce::Font(13.0f, juce::Font::bold));
        g.drawFittedText(title, bounds.reduced(10.0f, 8.0f).toNearestInt(), juce::Justification::centredTop, 1);

        g.setColour(juce::Colour(200, 220, 240));
        g.setFont(juce::Font(12.0f));
        auto contentBounds = bounds.reduced(10.0f, 8.0f);
        contentBounds.removeFromTop(18.0f);
        g.drawFittedText(content, contentBounds.toNearestInt(), juce::Justification::centred, 2);
    }

private:
    juce::String title;
    juce::String content;
};


//==============================================================================
// OceanLookAndFeel Implementation
//==============================================================================

void OceanLookAndFeel::drawRotarySlider(juce::Graphics& g, int x, int y, int width, int height, float sliderPosProportional, float rotaryStartAngle, float rotaryEndAngle, juce::Slider& slider)
{
    auto bounds = juce::Rectangle<float>(x, y, width, height).reduced(8.0f);
    auto radius = juce::jmin(bounds.getWidth(), bounds.getHeight()) * 0.5f;
    auto centre = bounds.getCentre();
    auto angle = rotaryStartAngle + sliderPosProportional * (rotaryEndAngle - rotaryStartAngle);
    const bool isHovered = slider.isMouseOverOrDragging();

    if (isHovered)
    {
        juce::ColourGradient glowGrad(juce::Colour(100, 200, 255).withAlpha(0.28f), centre.x - radius * 1.2f, centre.y - radius * 1.2f,
                                      juce::Colour(150, 220, 255).withAlpha(0.10f), centre.x + radius * 1.2f, centre.y + radius * 1.2f, false);
        g.setGradientFill(glowGrad);
        g.fillEllipse(centre.x - radius * 1.2f, centre.y - radius * 1.2f, radius * 2.4f, radius * 2.4f);
    }

    juce::ColourGradient outerRing(juce::Colour(15, 35, 55), centre.x - radius, centre.y - radius,
                                   juce::Colour(25, 45, 65), centre.x + radius, centre.y + radius, false);
    g.setGradientFill(outerRing);
    g.fillEllipse(centre.x - radius, centre.y - radius, radius * 2.0f, radius * 2.0f);

    g.setColour(juce::Colour(40, 80, 120).withAlpha(0.8f));
    g.fillEllipse(centre.x - radius * 0.85f, centre.y - radius * 0.85f, radius * 1.7f, radius * 1.7f);

    juce::ColourGradient centerGrad(juce::Colour(60, 100, 140), centre.x - radius * 0.3f, centre.y - radius * 0.3f,
                                    juce::Colour(30, 60, 90), centre.x + radius * 0.3f, centre.y + radius * 0.3f, false);
    g.setGradientFill(centerGrad);
    g.fillEllipse(centre.x - radius * 0.6f, centre.y - radius * 0.6f, radius * 1.2f, radius * 1.2f);

    g.setColour(juce::Colour(100, 200, 255).withAlpha(0.9f));
    juce::Path arc;
    arc.addArc(centre.x - radius * 0.75f, centre.y - radius * 0.75f, radius * 1.5f, radius * 1.5f,
               rotaryStartAngle, angle, true);
    g.strokePath(arc, juce::PathStrokeType(isHovered ? 4.0f : 3.0f));

    juce::Path pointer;
    auto pointerLength = radius * 0.7f;
    auto pointerThickness = isHovered ? 4.0f : 3.0f;
    pointer.addLineSegment(juce::Line<float>(centre.x, centre.y,
                                             centre.x + std::sin(angle) * pointerLength,
                                             centre.y - std::cos(angle) * pointerLength), pointerThickness);
    g.setColour(isHovered ? juce::Colour(235, 245, 255) : juce::Colour(150, 220, 255));
    g.strokePath(pointer, juce::PathStrokeType(pointerThickness, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    g.setColour(juce::Colour(200, 230, 255));
    g.fillEllipse(centre.x - 3.0f, centre.y - 3.0f, 6.0f, 6.0f);
}

void OceanLookAndFeel::drawLinearSlider(juce::Graphics& g, int x, int y, int width, int height, float sliderPos, float minSliderPos, float maxSliderPos, const juce::Slider::SliderStyle style, juce::Slider& slider)
{
    // Ocean-themed linear slider
    auto bounds = slider.getLocalBounds().toFloat().reduced(2.0f);
    
    juce::ColourGradient trackGradient(juce::Colour(30, 60, 80), 0, 0,
                                      juce::Colour(15, 40, 60), 0, bounds.getHeight(), false);
    g.setGradientFill(trackGradient);
    g.fillRoundedRectangle(bounds, 4.0f);
    
    // Thumb
    auto thumbBounds = bounds.removeFromTop(bounds.getHeight() * 0.1f);
    thumbBounds = thumbBounds.withCentre(juce::Point<float>(bounds.getCentreX(), sliderPos));
    
    g.setColour(juce::Colour(100, 200, 255));
    g.fillRoundedRectangle(thumbBounds, 3.0f);
}

void OceanLookAndFeel::drawToggleButton(juce::Graphics& g, juce::ToggleButton& button, bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown)
{
    auto bounds = button.getLocalBounds().toFloat().reduced(2.0f);
    
    if (button.getToggleState())
    {
        juce::ColourGradient activeGradient(juce::Colour(50, 150, 200), 0, 0,
                                           juce::Colour(30, 100, 150), 0, bounds.getHeight(), false);
        g.setGradientFill(activeGradient);
    }
    else
    {
        juce::ColourGradient inactiveGradient(juce::Colour(40, 60, 80), 0, 0,
                                             juce::Colour(20, 40, 60), 0, bounds.getHeight(), false);
        g.setGradientFill(inactiveGradient);
    }
    
    g.fillRoundedRectangle(bounds, 4.0f);
    
    g.setColour(juce::Colours::white.withAlpha(0.8f));
    g.drawFittedText(button.getButtonText(), bounds.toNearestInt(), juce::Justification::centred, 1);
}

void OceanLookAndFeel::drawLabel(juce::Graphics& g, juce::Label& label)
{
    auto bounds = label.getLocalBounds().toFloat().reduced(3.0f);
    const bool isHovered = label.isMouseOverOrDragging();
    const bool isOrcaLabel = (label.getComponentID() == "orcaLabel");
    if (isOrcaLabel)
    {
        bounds = bounds.translated(0.0f, -5.0f);
        bounds = bounds.reduced(2.0f, 1.0f);
    }

    if (isOrcaLabel)
    {
        const auto labelText = label.getText();
        const bool isWhiteLabel = (labelText == "SIZE" || labelText == "MIX" || labelText == "DEPTH" || labelText == "DECAY");
        if (isHovered)
        {
            g.setColour(isWhiteLabel ? juce::Colour(255, 255, 255).withAlpha(0.3f)
                                     : juce::Colour(50, 150, 220).withAlpha(0.4f));
            g.setFont(juce::Font(16.0f, juce::Font::bold));
            g.drawFittedText(label.getText(), bounds.toNearestInt(), juce::Justification::centred, 1);

            g.setColour(isWhiteLabel ? juce::Colour(255, 255, 255)
                                     : juce::Colour(30, 100, 180));
            g.setFont(juce::Font(14.0f, juce::Font::bold));
            g.drawFittedText(label.getText(), bounds.toNearestInt(), juce::Justification::centred, 1);
        }
        else
        {
            g.setColour(isWhiteLabel ? juce::Colour(255, 255, 255)
                                     : juce::Colour(40, 120, 200));
            g.setFont(juce::Font(13.0f, juce::Font::bold));
            g.drawFittedText(label.getText(), bounds.toNearestInt(), juce::Justification::centred, 1);
        }
    }
    else
    {
        if (isHovered)
        {
            juce::ColourGradient hoverBg(juce::Colour(30, 60, 120), bounds.getX(), bounds.getY(),
                                         juce::Colour(50, 100, 180), bounds.getRight(), bounds.getBottom(), false);
            g.setGradientFill(hoverBg);
            g.fillRoundedRectangle(bounds, 3.0f);

            g.setColour(juce::Colour(100, 150, 255));
            g.drawRoundedRectangle(bounds, 3.0f, 1.0f);
        }
        else
        {
            g.setColour(juce::Colour(15, 30, 50).withAlpha(0.3f));
            g.fillRoundedRectangle(bounds, 3.0f);

            g.setColour(juce::Colour(20, 40, 70));
            g.drawRoundedRectangle(bounds, 3.0f, 0.8f);
        }
    }

    if (! isOrcaLabel)
    {
        g.setColour(juce::Colour(200, 220, 240));
        g.setFont(juce::Font(12.0f, juce::Font::bold));
        g.drawFittedText(label.getText(), bounds.toNearestInt(), juce::Justification::centred, 1);
    }
}

void OceanLookAndFeel::setKnobImage(const juce::Image& img)
{
    knobImage = img;
}

void OceanLookAndFeel::setKnobStripImage(const juce::Image& img)
{
    knobStripImage = img;
    if (knobStripImage.isValid())
    {
        const int h = knobStripImage.getHeight();
        knobStripFrames = (h > 0) ? (knobStripImage.getWidth() / h) : 0;
    }
    else
    {
        knobStripFrames = 0;
    }
}

//==============================================================================
// WaveformVisualizer Implementation
//==============================================================================

WaveformVisualizer::WaveformVisualizer()
{
    waveformBuffer.setSize(1, bufferSize);
    waveformBuffer.clear();
    
    startTimerHz(30); // 30 FPS refresh rate
}

void WaveformVisualizer::paint(juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat();
    
    // Background
    juce::ColourGradient bgGradient(juce::Colour(10, 30, 50), 0, 0,
                                   juce::Colour(5, 20, 35), bounds.getWidth(), bounds.getHeight(), false);
    g.setGradientFill(bgGradient);
    g.fillRoundedRectangle(bounds, 8.0f);
    
    // Draw waveform
    if (waveformPath.getLength() > 0)
    {
        g.setColour(juce::Colour(100, 200, 255).withAlpha(0.8f));
        g.strokePath(waveformPath, juce::PathStrokeType(2.0f));
        
        // Glow effect
        g.setColour(juce::Colour(150, 220, 255).withAlpha(0.3f));
        g.strokePath(waveformPath, juce::PathStrokeType(4.0f));
    }
}

void WaveformVisualizer::resized()
{
    // Will be called when component is resized
}

void WaveformVisualizer::pushSample(float sample)
{
    waveformBuffer.setSample(0, writePosition, sample);
    writePosition = (writePosition + 1) % bufferSize;
}

void WaveformVisualizer::timerCallback()
{
    // Update waveform path
    waveformPath.clear();
    
    auto bounds = getLocalBounds().toFloat();
    auto width = bounds.getWidth();
    auto height = bounds.getHeight();
    auto centerY = height / 2.0f;
    
    waveformPath.startNewSubPath(0.0f, centerY);
    
    for (int i = 0; i < bufferSize; ++i)
    {
        auto sample = waveformBuffer.getSample(0, (writePosition + i) % bufferSize);
        auto x = juce::jmap(float(i), 0.0f, float(bufferSize), 0.0f, width);
        auto y = juce::jmap(sample, -1.0f, 1.0f, centerY - height * 0.4f, centerY + height * 0.4f);
        
        if (i == 0)
            waveformPath.startNewSubPath(x, y);
        else
            waveformPath.lineTo(x, y);
    }
    
    repaint();
}

//==============================================================================
// PresetManager Implementation
//==============================================================================

void PresetManager::initializeFactoryPresets()
{
    factoryPresets = {
        {"Deep Ocean Dream", {{"mix", 0.45f}, {"size", 0.9f}, {"decay", 14.0f}, {"predelay", 35.0f}, {"depth", 0.7f}, {"damping", 0.6f}, {"width", 1.2f}, {"diffusion", 0.85f}, {"tone", -0.3f}}},
        {"Whale Cathedral", {{"mix", 0.6f}, {"size", 1.0f}, {"decay", 18.0f}, {"predelay", 80.0f}, {"depth", 0.4f}, {"damping", 0.4f}, {"width", 1.4f}, {"diffusion", 0.9f}, {"tone", 0.3f}}},
        {"Gentle Tide", {{"mix", 0.25f}, {"size", 0.4f}, {"decay", 3.0f}, {"predelay", 12.0f}, {"depth", 0.2f}, {"damping", 0.5f}, {"width", 1.0f}, {"diffusion", 0.6f}, {"tone", 0.0f}}},
        {"Midnight Current", {{"mix", 0.5f}, {"size", 0.7f}, {"decay", 8.0f}, {"predelay", 45.0f}, {"depth", 0.6f}, {"damping", 0.7f}, {"width", 1.1f}, {"diffusion", 0.75f}, {"tone", -0.2f}}},
        {"Arctic Drift", {{"mix", 0.35f}, {"size", 0.6f}, {"decay", 5.0f}, {"predelay", 25.0f}, {"depth", 0.3f}, {"damping", 0.8f}, {"width", 0.9f}, {"diffusion", 0.65f}, {"tone", -0.5f}}},
        {"Infinite Blue", {{"mix", 0.55f}, {"size", 0.85f}, {"decay", 12.0f}, {"predelay", 60.0f}, {"depth", 0.5f}, {"damping", 0.45f}, {"width", 1.3f}, {"diffusion", 0.8f}, {"tone", 0.1f}}},
        {"Submarine Hangar", {{"mix", 0.4f}, {"size", 0.95f}, {"decay", 10.0f}, {"predelay", 40.0f}, {"depth", 0.45f}, {"damping", 0.55f}, {"width", 1.15f}, {"diffusion", 0.7f}, {"tone", -0.1f}}},
        {"Abyss Chamber", {{"mix", 0.65f}, {"size", 1.0f}, {"decay", 20.0f}, {"predelay", 100.0f}, {"depth", 0.8f}, {"damping", 0.35f}, {"width", 1.5f}, {"diffusion", 0.95f}, {"tone", 0.4f}}},
        {"Leviathan Hall", {{"mix", 0.7f}, {"size", 0.9f}, {"decay", 16.0f}, {"predelay", 90.0f}, {"depth", 0.6f}, {"damping", 0.4f}, {"width", 1.4f}, {"diffusion", 0.85f}, {"tone", 0.2f}}},
        {"Sunken Temple", {{"mix", 0.48f}, {"size", 0.75f}, {"decay", 11.0f}, {"predelay", 55.0f}, {"depth", 0.55f}, {"damping", 0.5f}, {"width", 1.25f}, {"diffusion", 0.78f}, {"tone", -0.15f}}},
        {"Glass Water", {{"mix", 0.3f}, {"size", 0.35f}, {"decay", 2.5f}, {"predelay", 8.0f}, {"depth", 0.15f}, {"damping", 0.6f}, {"width", 0.85f}, {"diffusion", 0.55f}, {"tone", 0.05f}}},
        {"Studio Ocean", {{"mix", 0.42f}, {"size", 0.55f}, {"decay", 4.0f}, {"predelay", 18.0f}, {"depth", 0.25f}, {"damping", 0.52f}, {"width", 1.05f}, {"diffusion", 0.62f}, {"tone", -0.08f}}},
        {"Soft Lagoon", {{"mix", 0.38f}, {"size", 0.45f}, {"decay", 3.5f}, {"predelay", 15.0f}, {"depth", 0.22f}, {"damping", 0.58f}, {"width", 0.95f}, {"diffusion", 0.58f}, {"tone", -0.12f}}},
        {"Whale Call", {{"mix", 0.52f}, {"size", 0.8f}, {"decay", 9.0f}, {"predelay", 50.0f}, {"depth", 0.65f}, {"damping", 0.42f}, {"width", 1.2f}, {"diffusion", 0.82f}, {"tone", 0.15f}}},
        {"Sonic Current", {{"mix", 0.46f}, {"size", 0.68f}, {"decay", 7.0f}, {"predelay", 38.0f}, {"depth", 0.48f}, {"damping", 0.53f}, {"width", 1.08f}, {"diffusion", 0.72f}, {"tone", -0.18f}}},
        {"Hydro Echo", {{"mix", 0.58f}, {"size", 0.88f}, {"decay", 13.0f}, {"predelay", 70.0f}, {"depth", 0.58f}, {"damping", 0.38f}, {"width", 1.35f}, {"diffusion", 0.88f}, {"tone", 0.25f}}},
        {"Liquid Sky", {{"mix", 0.44f}, {"size", 0.72f}, {"decay", 8.5f}, {"predelay", 42.0f}, {"depth", 0.42f}, {"damping", 0.48f}, {"width", 1.18f}, {"diffusion", 0.76f}, {"tone", -0.05f}}},
        {"Atlantis Hall", {{"mix", 0.62f}, {"size", 0.92f}, {"decay", 15.0f}, {"predelay", 85.0f}, {"depth", 0.52f}, {"damping", 0.36f}, {"width", 1.45f}, {"diffusion", 0.92f}, {"tone", 0.35f}}},
        {"Underwater Voice", {{"mix", 0.54f}, {"size", 0.78f}, {"decay", 6.5f}, {"predelay", 32.0f}, {"depth", 0.38f}, {"damping", 0.62f}, {"width", 1.12f}, {"diffusion", 0.68f}, {"tone", -0.25f}}},
        {"Distant Horizon", {{"mix", 0.41f}, {"size", 0.65f}, {"decay", 5.8f}, {"predelay", 28.0f}, {"depth", 0.35f}, {"damping", 0.56f}, {"width", 1.22f}, {"diffusion", 0.74f}, {"tone", -0.22f}}},
        {"Ocean Bloom", {{"mix", 0.49f}, {"size", 0.82f}, {"decay", 9.5f}, {"predelay", 48.0f}, {"depth", 0.46f}, {"damping", 0.44f}, {"width", 1.28f}, {"diffusion", 0.8f}, {"tone", 0.08f}}}
    };
}

void PresetManager::loadPreset(int index, juce::AudioProcessorValueTreeState& params)
{
    if (index >= 0 && index < factoryPresets.size())
    {
        currentPresetIndex = index;
        auto& preset = factoryPresets[index];
        
        for (const auto& param : preset.parameters)
        {
            if (auto* p = params.getParameter(param.first))
            {
                p->setValueNotifyingHost(p->convertFrom0to1(param.second));
            }
        }
    }
}

juce::String PresetManager::getCurrentPresetName() const
{
    if (currentPresetIndex >= 0 && currentPresetIndex < factoryPresets.size())
        return factoryPresets[currentPresetIndex].name;
    return "No Preset";
}

//==============================================================================
// OceanBackground Implementation
//==============================================================================

OceanBackground::OceanBackground()
{
    // Initialize particles
    for (int i = 0; i < 20; ++i)
    {
        particles.push_back(juce::Point<float>(juce::Random::getSystemRandom().nextFloat() * 800.0f,
                                              juce::Random::getSystemRandom().nextFloat() * 600.0f));
    }
    
    // Initialize light rays
    for (int i = 0; i < 5; ++i)
    {
        auto startX = juce::Random::getSystemRandom().nextFloat() * 800.0f;
        lightRays.push_back(juce::Line<float>(startX, 0.0f, startX + 50.0f, 600.0f));
    }
    
    startTimerHz(20); // 20 FPS for smooth animation
}

void OceanBackground::paint(juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat();
    
    // Ocean gradient background
    juce::ColourGradient oceanGradient(juce::Colour(5, 25, 45), 0, 0,
                                      juce::Colour(15, 45, 75), bounds.getWidth(), bounds.getHeight(), false);
    g.setGradientFill(oceanGradient);
    g.fillAll();
    
    // Animated light rays
    g.setColour(juce::Colour(100, 150, 200).withAlpha(0.1f));
    for (auto& ray : lightRays)
    {
        auto offsetRay = juce::Line<float>(ray.getStart().x + animationOffset * 10.0f, ray.getStart().y,
                                         ray.getEnd().x + animationOffset * 15.0f, ray.getEnd().y);
        g.drawLine(offsetRay, 2.0f);
    }
    
    // Floating particles
    g.setColour(juce::Colour(150, 200, 255).withAlpha(0.3f));
    for (auto& particle : particles)
    {
        auto offsetParticle = particle + juce::Point<float>(std::sin(animationOffset + particle.x * 0.01f) * 5.0f,
                                                          std::cos(animationOffset + particle.y * 0.01f) * 3.0f);
        g.fillEllipse(offsetParticle.x, offsetParticle.y, 3.0f, 3.0f);
    }
}

void OceanBackground::resized()
{
    // Handle resize
}

void OceanBackground::timerCallback()
{
    animationOffset += 0.02f; // Very slow movement as specified
    if (animationOffset > juce::MathConstants<float>::twoPi)
        animationOffset -= juce::MathConstants<float>::twoPi;
    
    repaint();
}

//==============================================================================
// KimuVerbAudioProcessorEditor Implementation
//==============================================================================

juce::Image KimuVerbAudioProcessorEditor::loadImageFromBinary(const char* name)
{
    int size = 0;
    if (const void* data = BinaryData::getNamedResource(name, size))
        return juce::ImageCache::getFromMemory(data, size);
    return {};
}

juce::Image KimuVerbAudioProcessorEditor::loadImageFromFile(const juce::String& relativePath)
{
    auto base = juce::File::getCurrentWorkingDirectory();
    auto file = base.getChildFile(relativePath);
    if (file.existsAsFile())
        return juce::ImageCache::getFromFile(file);
    return {};
}

void KimuVerbAudioProcessorEditor::loadImages()
{
    backgroundImage = juce::ImageCache::getFromMemory(BinaryData::background_png, BinaryData::background_pngSize);
    orcaImage = juce::ImageCache::getFromMemory(BinaryData::orca_png, BinaryData::orca_pngSize);
    logoImage = juce::ImageCache::getFromMemory(BinaryData::kimuverb_logo_png, BinaryData::kimuverb_logo_pngSize);
    waveImage = juce::ImageCache::getFromMemory(BinaryData::wavevisualizer_png, BinaryData::wavevisualizer_pngSize);
    knobStripImage = juce::ImageCache::getFromMemory(BinaryData::knobstrip_example_png, BinaryData::knobstrip_example_pngSize);
    knobImage = juce::ImageCache::getFromMemory(BinaryData::knob_2_png, BinaryData::knob_2_pngSize);
    labelMixImage = juce::ImageCache::getFromMemory(BinaryData::mix_textlabel_png, BinaryData::mix_textlabel_pngSize);
    labelSizeImage = juce::ImageCache::getFromMemory(BinaryData::size_textlabel_png, BinaryData::size_textlabel_pngSize);
    labelPreDelayImage = juce::ImageCache::getFromMemory(BinaryData::predelay_text_label_png, BinaryData::predelay_text_label_pngSize);

    if (! backgroundImage.isValid())
        backgroundImage = loadImageFromFile("assets/ui/background/background.png");
    if (! orcaImage.isValid())
        orcaImage = loadImageFromFile("assets/ui/orca/orca.png");
    if (! logoImage.isValid())
        logoImage = loadImageFromFile("assets/ui/kimuverb logo/kimuverb logo.png");
    if (! waveImage.isValid())
        waveImage = loadImageFromFile("assets/ui/wave visualizer/wavevisualizer.png");
    if (! knobStripImage.isValid())
        knobStripImage = loadImageFromFile("assets/ui/knobstrip/knobstrip example.png");
    if (! knobImage.isValid())
        knobImage = loadImageFromFile("assets/ui/knobstrip/knob 2.png");
    if (! labelMixImage.isValid())
        labelMixImage = loadImageFromFile("assets/ui/Text Labels/mix textlabel.png");
    if (! labelSizeImage.isValid())
        labelSizeImage = loadImageFromFile("assets/ui/Text Labels/size textlabel.png");
    if (! labelPreDelayImage.isValid())
        labelPreDelayImage = loadImageFromFile("assets/ui/Text Labels/predelay text label.png");

    juce::Logger::writeToLog("BinaryData resources: " + juce::String(BinaryData::namedResourceListSize));
    juce::Logger::writeToLog("Background loaded: " + juce::String(backgroundImage.isValid() ? "YES" : "NO"));
    juce::Logger::writeToLog("Orca loaded: " + juce::String(orcaImage.isValid() ? "YES" : "NO"));
    juce::Logger::writeToLog("Logo loaded: " + juce::String(logoImage.isValid() ? "YES" : "NO"));
}

bool KimuVerbAudioProcessorEditor::loadAudioFile(const juce::File& file, SampleData& outData, const juce::String& label)
{
    if (! file.existsAsFile())
    {
        juce::Logger::writeToLog(label + " audio missing: " + file.getFullPathName());
        return false;
    }

    std::unique_ptr<juce::AudioFormatReader> reader(uiFormatManager.createReaderFor(file));
    if (reader == nullptr)
    {
        juce::Logger::writeToLog(label + " audio failed to load: " + file.getFullPathName());
        return false;
    }

    outData.sampleRate = reader->sampleRate;
    outData.buffer.setSize((int) reader->numChannels, (int) reader->lengthInSamples);
    reader->read(&outData.buffer, 0, (int) reader->lengthInSamples, 0, true, true);
    juce::Logger::writeToLog(label + " audio loaded: " + file.getFileName());
    return true;
}

void KimuVerbAudioProcessorEditor::loadAudioFiles()
{
    uiFormatManager.registerBasicFormats();
    whaleData = std::make_shared<SampleData>();
    betterOceanData = std::make_shared<SampleData>();

    loadAudioFile(juce::File("c:/Users/Owner/Desktop/assets/audio/kimuverb sounds/whale sound/killer whale sound.mp3"),
                  *whaleData, "Whale");
    loadAudioFile(juce::File("c:/Users/Owner/Desktop/assets/audio/kimuverb sounds/ocean sound/better ocean sound/better ocean sound.mp3"),
                  *betterOceanData, "Better Ocean");
}

KimuVerbAudioProcessorEditor::KimuVerbAudioProcessorEditor(KimuVerbAudioProcessor& p)
: juce::AudioProcessorEditor(&p), processor(p)
{
    setLookAndFeel(&lnf);
    loadImages();
    loadAudioFiles();
    lnf.setKnobStripImage(knobStripImage);
    lnf.setKnobImage(knobImage);
    
    // Initialize UI components with safe fallbacks
    visualizer = std::make_unique<WaveformVisualizer>();
    oceanBg = std::make_unique<OceanBackground>();
    presetManager = std::make_unique<PresetManager>();
    
    presetManager->initializeFactoryPresets();
    
    setupParameters();
    setupPresetDropdown();
    setupPresetNavigation();
    setupUiAudio();
    
    setSize(900, 600);
    createOrcaBodyMapping();
    layoutOrcaControls();
    layoutUIComponents();

    startTimerHz(30);
}

KimuVerbAudioProcessorEditor::~KimuVerbAudioProcessorEditor()
{
    shutdownUiAudio();
    setLookAndFeel(nullptr);
}

void KimuVerbAudioProcessorEditor::paint(juce::Graphics& g)
{
    // Ocean gradient background (programmatic instead of image loading)
    auto bounds = getLocalBounds().toFloat();
    if (backgroundImage.isValid())
    {
        g.drawImageWithin(backgroundImage,
                          (int) bounds.getX(), (int) bounds.getY(),
                          (int) bounds.getWidth(), (int) bounds.getHeight(),
                          juce::RectanglePlacement::stretchToFit);
    }
    draw3DMarineEffects(g, bounds);

    
    // Draw orca image (fallback to path if missing)
    auto contentArea = bounds;
    contentArea.removeFromRight(220.0f);
    contentArea.removeFromLeft(160.0f);
    contentArea.removeFromBottom(110.0f);
    auto orcaArea = contentArea.reduced(10.0f);
    if (orcaImage.isValid())
    {
        g.drawImageWithin(orcaImage,
                          (int) orcaArea.getX(), (int) orcaArea.getY(),
                          (int) orcaArea.getWidth(), (int) orcaArea.getHeight(),
                          juce::RectanglePlacement::centred);
    }
    else
    {
        g.setColour(juce::Colour(20, 40, 60).withAlpha(0.9f));
        g.fillPath(orcaPath);
        g.setColour(juce::Colour(100, 200, 255).withAlpha(0.4f));
        g.strokePath(orcaPath, juce::PathStrokeType(3.0f));
    }

    drawWaveOverlay(g, contentArea);
    drawRipples(g, contentArea);

    // Logo (top-left)
    if (logoImage.isValid())
    {
        auto logoArea = bounds.removeFromTop(70.0f).removeFromLeft(320.0f).reduced(10.0f);
        juce::ColourGradient logoBg(juce::Colour(10, 30, 60), logoArea.getX(), logoArea.getY(),
                                    juce::Colour(20, 50, 90), logoArea.getRight(), logoArea.getBottom(), false);
        g.setGradientFill(logoBg);
        g.fillRoundedRectangle(logoArea.toFloat(), 5.0f);
        g.setColour(juce::Colours::black);
        g.drawRoundedRectangle(logoArea.toFloat(), 5.0f, 2.0f);
        g.drawImageWithin(logoImage,
                          (int) logoArea.getX(), (int) logoArea.getY(),
                          (int) logoArea.getWidth(), (int) logoArea.getHeight(),
                          juce::RectanglePlacement::centred);
    }

    // PNG labels removed - using consistent hover headers instead
    
    // Hover indicator
    if (hoveredBodyPart.isNotEmpty())
    {
        g.setColour(juce::Colour(150, 220, 255).withAlpha(0.5f));
        g.setFont(juce::Font(14.0f));
        g.drawFittedText(hoveredBodyPart, getLocalBounds().removeFromBottom(30), juce::Justification::centred, 1);
    }
}

void KimuVerbAudioProcessorEditor::resized()
{
    createOrcaBodyMapping();
    layoutOrcaControls();
    layoutUIComponents();
    lastMoveMs = juce::Time::getMillisecondCounter();
}

void KimuVerbAudioProcessorEditor::moved()
{
    lastMoveMs = juce::Time::getMillisecondCounter();
}

void KimuVerbAudioProcessorEditor::createOrcaBodyMapping()
{
    auto bounds = getLocalBounds().toFloat();
    auto contentArea = bounds;
    contentArea.removeFromRight(220.0f);
    contentArea.removeFromLeft(160.0f);
    contentArea.removeFromBottom(110.0f);
    auto orcaArea = contentArea.reduced(10.0f);
    
    // Create detailed orca path
    orcaPath.clear();
    orcaPath.startNewSubPath(orcaArea.getX(), orcaArea.getCentreY());
    orcaPath.quadraticTo(orcaArea.getX() + orcaArea.getWidth() * 0.15f, orcaArea.getY(),
                         orcaArea.getX() + orcaArea.getWidth() * 0.45f, orcaArea.getY() + orcaArea.getHeight() * 0.15f);
    orcaPath.quadraticTo(orcaArea.getX() + orcaArea.getWidth() * 0.65f, orcaArea.getY() + orcaArea.getHeight() * 0.25f,
                         orcaArea.getX() + orcaArea.getWidth() * 0.80f, orcaArea.getY() + orcaArea.getHeight() * 0.30f);
    orcaPath.quadraticTo(orcaArea.getRight(), orcaArea.getY() + orcaArea.getHeight() * 0.55f,
                         orcaArea.getX() + orcaArea.getWidth() * 0.75f, orcaArea.getY() + orcaArea.getHeight() * 0.80f);
    orcaPath.quadraticTo(orcaArea.getX() + orcaArea.getWidth() * 0.55f, orcaArea.getBottom(),
                         orcaArea.getX() + orcaArea.getWidth() * 0.20f, orcaArea.getY() + orcaArea.getHeight() * 0.80f);
    orcaPath.closeSubPath();
    
    // Map body parts to match the new orca drawing
    const float knobSize = 58.0f;
    auto place = [&](const juce::String& key, float nx, float ny)
    {
        auto x = orcaArea.getX() + nx * orcaArea.getWidth();
        auto y = orcaArea.getY() + ny * orcaArea.getHeight();
        orcaBodyParts[key] = juce::Rectangle<float>(x - knobSize * 0.5f, y - knobSize * 0.5f, knobSize, knobSize);
    };
    place("Head", 0.25f, 0.35f);       // Pre-delay (original)
    place("DorsalFin", 0.75f, 0.20f);  // Size (original)
    place("UpperBody", 0.70f, 0.45f);  // Decay (original)
    place("Belly", 0.10f, 0.80f);      // Mix (original position)
    place("Tail", 0.30f, 0.60f);       // Depth (original)
    place("LowerFin", 0.50f, 0.70f);   // Damping (original)
}

void KimuVerbAudioProcessorEditor::setupParameters()
{
    auto setupSlider = [&](juce::Slider& s) {
        s.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
        s.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
        s.setLookAndFeel(&lnf);
        s.setEnabled(true);
        s.setInterceptsMouseClicks(true, true);
        s.setMouseCursor(juce::MouseCursor::PointingHandCursor);
        addAndMakeVisible(s);
    };
    
    // Main orca body parameters
    setupSlider(preDelaySlider);    // Head
    setupSlider(sizeSlider);        // Dorsal Fin
    setupSlider(decaySlider);      // Upper Body
    setupSlider(mixSlider);        // Belly
    setupSlider(depthSlider);      // Tail
    setupSlider(dampingSlider);    // Lower Fin
    
    // Additional parameters
    setupSlider(toneSlider);
    setupSlider(widthSlider);
    setupSlider(lowCutSlider);
    setupSlider(highCutSlider);
    setupSlider(motionSlider);
    setupSlider(diffusionSlider);
    
    // Oceanic controls
    setupSlider(currentSlider);
    setupSlider(pressureSlider);
    setupSlider(depthShiftSlider);

    
    addAndMakeVisible(loggingToggle);

    auto setupLabel = [&](juce::Label& l, const juce::String& text) {
        l.setText(text, juce::dontSendNotification);
        l.setColour(juce::Label::textColourId, juce::Colour(200, 220, 240));
        l.setColour(juce::Label::backgroundColourId, juce::Colour(20, 40, 70).withAlpha(0.8f));
        l.setFont(juce::Font(12.0f, juce::Font::bold));
        l.setJustificationType(juce::Justification::centred);
        l.setInterceptsMouseClicks(true, false);
        l.setRepaintsOnMouseActivity(true);
        addAndMakeVisible(l);
    };
    auto markOrcaLabel = [&](juce::Label& l) {
        l.setComponentID("orcaLabel");
    };
    setupLabel(toneLabel, "TONE");
    setupLabel(widthLabel, "WIDTH");
    setupLabel(lowCutLabel, "LOW CUT");
    setupLabel(highCutLabel, "HIGH CUT");
    setupLabel(motionLabel, "MOTION");
    setupLabel(diffusionLabel, "DIFFUSION");
    setupLabel(currentLabel, "CURRENT");
    setupLabel(pressureLabel, "PRESSURE");
    setupLabel(depthShiftLabel, "DEPTH SHIFT");
    setupLabel(preDelayLabel, "PREDELAY");
    setupLabel(sizeLabel, "SIZE");
    setupLabel(decayLabel, "DECAY");
    setupLabel(dampingLabel, "DAMPING");
    setupLabel(depthLabel, "DEPTH");
    setupLabel(mixLabel, "MIX");
    markOrcaLabel(preDelayLabel);
    markOrcaLabel(sizeLabel);
    markOrcaLabel(decayLabel);
    markOrcaLabel(mixLabel);
    markOrcaLabel(depthLabel);
    markOrcaLabel(dampingLabel);
    
    // UI Components
    addAndMakeVisible(*oceanBg);
    oceanBg->setInterceptsMouseClicks(false, false);
    oceanBg->setVisible(false);
    addAndMakeVisible(*visualizer);
    addAndMakeVisible(prevPresetButton);
    addAndMakeVisible(nextPresetButton);
    addAndMakeVisible(presetNameLabel);
    addAndMakeVisible(presetComboBox);
    presetNameLabel.setVisible(false);
    
    // Parameter attachments
    auto& vts = processor.getValueTreeState();
    preDelayAttachment = std::make_unique<SliderAttachment>(vts, "predelay", preDelaySlider);
    sizeAttachment = std::make_unique<SliderAttachment>(vts, "size", sizeSlider);
    decayAttachment = std::make_unique<SliderAttachment>(vts, "decay", decaySlider);
    mixAttachment = std::make_unique<SliderAttachment>(vts, "mix", mixSlider);
    depthAttachment = std::make_unique<SliderAttachment>(vts, "depth", depthSlider);
    dampingAttachment = std::make_unique<SliderAttachment>(vts, "damping", dampingSlider);
    
    toneAttachment = std::make_unique<SliderAttachment>(vts, "tone", toneSlider);
    widthAttachment = std::make_unique<SliderAttachment>(vts, "width", widthSlider);
    lowCutAttachment = std::make_unique<SliderAttachment>(vts, "lowcut", lowCutSlider);
    highCutAttachment = std::make_unique<SliderAttachment>(vts, "highcut", highCutSlider);
    motionAttachment = std::make_unique<SliderAttachment>(vts, "motion", motionSlider);
    diffusionAttachment = std::make_unique<SliderAttachment>(vts, "diffusion", diffusionSlider);
    
    currentAttachment = std::make_unique<SliderAttachment>(vts, "current", currentSlider);
    pressureAttachment = std::make_unique<SliderAttachment>(vts, "pressure", pressureSlider);
    depthShiftAttachment = std::make_unique<SliderAttachment>(vts, "depthshift", depthShiftSlider);
    
    loggingAttachment = std::make_unique<ButtonAttachment>(vts, "logging", loggingToggle);
}

void KimuVerbAudioProcessorEditor::layoutOrcaControls()
{
    // Position knobs on orca body parts
    preDelaySlider.setBounds(orcaBodyParts["Head"].toNearestInt());
    sizeSlider.setBounds(orcaBodyParts["DorsalFin"].toNearestInt());
    decaySlider.setBounds(orcaBodyParts["UpperBody"].toNearestInt());
    mixSlider.setBounds(orcaBodyParts["Belly"].toNearestInt());
    depthSlider.setBounds(orcaBodyParts["Tail"].toNearestInt());
    dampingSlider.setBounds(orcaBodyParts["LowerFin"].toNearestInt());
    preDelaySlider.toFront(false);
    sizeSlider.toFront(false);
    decaySlider.toFront(false);
    mixSlider.toFront(false);
    depthSlider.toFront(false);
    dampingSlider.toFront(false);

    auto placeLabel = [&](juce::Label& l, const juce::Slider& s)
    {
        auto r = s.getBounds().toFloat().translated(0.0f, -22.0f).expanded(6.0f, 2.0f);
        auto bounds = getLocalBounds().toFloat().reduced(2.0f);
        r.setY(juce::jmax(bounds.getY(), r.getY()));
        if (r.getBottom() > bounds.getBottom())
            r.setY(bounds.getBottom() - r.getHeight());
        l.setBounds(r.toNearestInt());
        l.setVisible(true);
        l.toFront(true);
    };
    placeLabel(preDelayLabel, preDelaySlider);
    placeLabel(sizeLabel, sizeSlider);
    placeLabel(decayLabel, decaySlider);
    placeLabel(mixLabel, mixSlider);
    placeLabel(dampingLabel, dampingSlider);
    placeLabel(depthLabel, depthSlider);
}

void KimuVerbAudioProcessorEditor::layoutUIComponents()
{
    auto bounds = getLocalBounds();
    
    // Ocean background (full screen)
    oceanBg->setBounds(bounds);
    
    // Visualizer on the right side
    visualizer->setBounds(bounds.removeFromRight(200).reduced(10));
    
    // Advanced parameters on the left side - LARGER and LOWER
    auto leftPanel = bounds.removeFromLeft(180).reduced(10);
    leftPanel = leftPanel.withTop(leftPanel.getY() + 100); // Move down 100px for better alignment
    
    const int labelH = 14;
    const int knobH = 58;
    const int vGap = 8;
    auto placeLeft = [&](juce::Label& l, juce::Slider& s)
    {
        l.setBounds(leftPanel.removeFromTop(labelH));
        s.setBounds(leftPanel.removeFromTop(knobH));
        leftPanel.removeFromTop(vGap);
    };

    placeLeft(widthLabel, widthSlider);
    placeLeft(lowCutLabel, lowCutSlider);
    placeLeft(highCutLabel, highCutSlider);
    placeLeft(motionLabel, motionSlider);
    placeLeft(diffusionLabel, diffusionSlider);
    
    // Oceanic controls at bottom (includes Tone)
    auto oceanicPanel = bounds.removeFromBottom(120).reduced(10);
    auto oceanicPanelBounds = oceanicPanel;
    const int bottomCols = 4;
    const int bottomGap = 8;
    const int colW = (oceanicPanel.getWidth() - (bottomCols - 1) * bottomGap) / bottomCols;
    auto placeBottom = [&](int colIndex, juce::Label& l, juce::Slider& s)
    {
        auto col = oceanicPanelBounds.removeFromLeft(colW);
        if (colIndex < bottomCols - 1)
            oceanicPanelBounds.removeFromLeft(bottomGap);
        l.setBounds(col.removeFromTop(14));
        s.setBounds(col.removeFromTop(58));
    };
    placeBottom(0, currentLabel, currentSlider);
    placeBottom(1, pressureLabel, pressureSlider);
    placeBottom(2, depthShiftLabel, depthShiftSlider);
    placeBottom(3, toneLabel, toneSlider);
    
    // Preset navigation at bottom center
    auto presetPanel = bounds.removeFromBottom(60).reduced(10);
    prevPresetButton.setBounds(presetPanel.removeFromLeft(40));
    nextPresetButton.setBounds(presetPanel.removeFromRight(40));
    presetNameLabel.setBounds(presetPanel);
    presetComboBox.setBounds(presetPanel.reduced(10));
    
    // Logging toggle
    loggingToggle.setBounds(bounds.removeFromTop(30).removeFromRight(100));

    orcaHit = computeOrcaArea();
    visualizerHit = visualizer->getBounds();
    bottomHit = oceanicPanelBounds.toNearestInt();
    
    // Update preset display
    updatePresetDisplay();
}

void KimuVerbAudioProcessorEditor::setupPresetDropdown()
{
    presetComboBox.clear();
    presetNames.clear();
    for (const auto& p : presetManager->factoryPresets)
        presetNames.add(p.name);

    presetComboBox.addItemList(presetNames, 1);
    presetComboBox.setSelectedId(presetManager->currentPresetIndex + 1, juce::dontSendNotification);
    presetComboBox.setColour(juce::ComboBox::backgroundColourId, juce::Colour(20, 50, 80));
    presetComboBox.setColour(juce::ComboBox::textColourId, juce::Colour(255, 255, 255));
    presetComboBox.setColour(juce::ComboBox::outlineColourId, juce::Colour(100, 200, 255));
    presetComboBox.setColour(juce::ComboBox::arrowColourId, juce::Colour(150, 220, 255));
    presetComboBox.setColour(juce::ComboBox::buttonColourId, juce::Colour(40, 80, 120));
    presetComboBox.setColour(juce::ComboBox::focusedOutlineColourId, juce::Colour(120, 200, 255));
    presetComboBox.setJustificationType(juce::Justification::centred);
    presetComboBox.setTooltip("Select preset");
    presetComboBox.setTextWhenNothingSelected("Choose Preset...");
    presetComboBox.setScrollWheelEnabled(true);

    presetComboBox.onChange = [this]()
    {
        int presetIndex = presetComboBox.getSelectedId() - 1;
        if (presetIndex >= 0 && presetIndex < presetManager->factoryPresets.size())
        {
            auto bounds = presetComboBox.getBounds();
            addRipple(juce::Point<float>(bounds.getCentreX(), bounds.getCentreY()));
            presetManager->loadPreset(presetIndex, processor.getValueTreeState());
            updatePresetDisplay();
        }
    };
}

void KimuVerbAudioProcessorEditor::setupPresetNavigation()
{
    prevPresetButton.setButtonText("<");
    nextPresetButton.setButtonText(">");
    prevPresetButton.setColour(juce::TextButton::buttonColourId, juce::Colour(30, 60, 100));
    prevPresetButton.setColour(juce::TextButton::textColourOnId, juce::Colour(255, 255, 255));
    prevPresetButton.setColour(juce::TextButton::textColourOffId, juce::Colour(200, 240, 255));
    prevPresetButton.setColour(juce::TextButton::buttonOnColourId, juce::Colour(60, 100, 150));
    nextPresetButton.setColour(juce::TextButton::buttonColourId, juce::Colour(30, 60, 100));
    nextPresetButton.setColour(juce::TextButton::textColourOnId, juce::Colour(255, 255, 255));
    nextPresetButton.setColour(juce::TextButton::textColourOffId, juce::Colour(200, 240, 255));
    nextPresetButton.setColour(juce::TextButton::buttonOnColourId, juce::Colour(60, 100, 150));
    prevPresetButton.setTooltip("Previous preset");
    nextPresetButton.setTooltip("Next preset");
    prevPresetButton.onClick = [this]() {
        if (presetManager->currentPresetIndex > 0)
        {
            auto bounds = prevPresetButton.getBounds();
            addRipple(juce::Point<float>(bounds.getCentreX(), bounds.getCentreY()));
            presetManager->loadPreset(presetManager->currentPresetIndex - 1, processor.getValueTreeState());
            updatePresetDisplay();
        }
    };
    
    nextPresetButton.onClick = [this]() {
        if (presetManager->currentPresetIndex < presetManager->factoryPresets.size() - 1)
        {
            auto bounds = nextPresetButton.getBounds();
            addRipple(juce::Point<float>(bounds.getCentreX(), bounds.getCentreY()));
            presetManager->loadPreset(presetManager->currentPresetIndex + 1, processor.getValueTreeState());
            updatePresetDisplay();
        }
    };
    
    presetNameLabel.setColour(juce::Label::textColourId, juce::Colour(200, 220, 240));
    presetNameLabel.setFont(juce::Font(16.0f));
    presetNameLabel.setJustificationType(juce::Justification::centred);
}

void KimuVerbAudioProcessorEditor::updatePresetDisplay()
{
    presetNameLabel.setText(presetManager->getCurrentPresetName(), juce::dontSendNotification);
    presetComboBox.setSelectedId(presetManager->currentPresetIndex + 1, juce::dontSendNotification);

    const double duration = getPresetOceanDuration(presetManager->currentPresetIndex);
    double startOffset = 0.0;
    if (betterOceanData)
    {
        const auto oceanLenSec = betterOceanData->buffer.getNumSamples() / juce::jmax(1.0, betterOceanData->sampleRate);
        const double randomFrac = uiRng.nextDouble();
        startOffset = oceanLenSec * randomFrac;
    }
    triggerBetterOceanSfx(duration, startOffset);
}

juce::Rectangle<int> KimuVerbAudioProcessorEditor::computeOrcaArea() const
{
    auto bounds = getLocalBounds().toFloat();
    auto contentArea = bounds;
    contentArea.removeFromRight(220.0f);
    contentArea.removeFromLeft(160.0f);
    contentArea.removeFromBottom(110.0f);
    auto orcaArea = contentArea.reduced(10.0f);
    return orcaArea.toNearestInt();
}

double KimuVerbAudioProcessorEditor::getPresetOceanDuration(int presetIndex) const
{
    switch (presetIndex)
    {
        case 0: return 4.5;
        case 1: return 6.0;
        case 2: return 4.0;
        case 3: return 5.0;
        case 4: return 4.0;
        case 5: return 5.5;
        case 6: return 4.8;
        case 7: return 6.5;
        case 8: return 6.0;
        case 9: return 5.2;
        case 10: return 4.0;
        case 11: return 4.0;
        case 12: return 4.0;
        case 13: return 5.0;
        case 14: return 5.0;
        case 15: return 5.8;
        case 16: return 5.2;
        case 17: return 6.2;
        case 18: return 4.6;
        case 19: return 4.4;
        case 20: return 5.4;
        default: return 4.5;
    }
}

void KimuVerbAudioProcessorEditor::setupUiAudio()
{
    if (processor.wrapperType != juce::AudioProcessor::wrapperType_Standalone)
        return;

    auto err = uiAudioDevice.initialise(0, 2, nullptr, true);
    if (! err.isEmpty())
        return;

    whaleSample = std::make_unique<UiSampleSource>();
    whaleSample->setSample(whaleData);
    betterOceanSample = std::make_unique<UiSampleSource>();
    betterOceanSample->setSample(betterOceanData);
    uiMixer.addInputSource(whaleSample.get(), false);
    uiMixer.addInputSource(betterOceanSample.get(), false);

    uiAudioPlayer.setSource(&uiMixer);
    uiAudioDevice.addAudioCallback(&uiAudioPlayer);
    uiAudioReady = true;
}

void KimuVerbAudioProcessorEditor::shutdownUiAudio()
{
    if (! uiAudioReady)
        return;

    uiAudioDevice.removeAudioCallback(&uiAudioPlayer);
    uiAudioPlayer.setSource(nullptr);
    uiMixer.removeAllInputs();
    whaleSample.reset();
    betterOceanSample.reset();
    uiAudioReady = false;
}

void KimuVerbAudioProcessorEditor::triggerWhaleSfx()
{
    if (! uiAudioReady || whaleSample == nullptr || ! whaleData)
        return;

    auto now = juce::Time::getMillisecondCounter();
    if (now - lastWhaleMs < 1200)
        return;
    lastWhaleMs = now;

    const auto whaleLenSec = whaleData->buffer.getNumSamples() / juce::jmax(1.0, whaleData->sampleRate);
    const double duration = juce::jmin(1.4, whaleLenSec);
    whaleSample->trigger(duration, 0.06f, 0.0);
}

void KimuVerbAudioProcessorEditor::triggerWaterSfx()
{
    if (! uiAudioReady || betterOceanSample == nullptr || ! betterOceanData)
        return;

    auto now = juce::Time::getMillisecondCounter();
    if (now - lastWaterMs < 350)
        return;
    lastWaterMs = now;

    const auto oceanLenSec = betterOceanData->buffer.getNumSamples() / juce::jmax(1.0, betterOceanData->sampleRate);
    const double maxStart = juce::jmax(0.0, oceanLenSec - 1.0);
    const double startOffset = uiRng.nextDouble() * maxStart;
    betterOceanSample->trigger(0.8, 0.30f, startOffset);
}

void KimuVerbAudioProcessorEditor::triggerBetterOceanSfx(double durationSeconds, double startOffsetSeconds)
{
    if (! uiAudioReady || betterOceanSample == nullptr || ! betterOceanData)
    {
        juce::Logger::writeToLog("triggerBetterOceanSfx: Failed - audio not ready or data missing");
        return;
    }

    auto now = juce::Time::getMillisecondCounter();
    if (now - lastWaterMs < 350)
        return;
    lastWaterMs = now;

    juce::Logger::writeToLog("triggerBetterOceanSfx: Triggering ocean sound - duration: " +
                             juce::String(durationSeconds) + "s, startOffset: " +
                             juce::String(startOffsetSeconds) + "s");
    juce::Logger::writeToLog("triggerBetterOceanSfx: Ocean buffer samples: " +
                             juce::String(betterOceanData->buffer.getNumSamples()) +
                             ", sample rate: " + juce::String(betterOceanData->sampleRate));

    betterOceanSample->trigger(durationSeconds, 0.30f, startOffsetSeconds);

    juce::Logger::writeToLog("triggerBetterOceanSfx: Ocean sound triggered successfully");
}


void KimuVerbAudioProcessorEditor::mouseDown(const juce::MouseEvent& event)
{
    const auto pos = event.position.toInt();

    // Click on orca image -> whale sound
    if (orcaHit.contains(pos))
    {
        triggerWhaleSfx();
        return;
    }

    // Double-click anywhere -> whale sound
    if (event.getNumberOfClicks() >= 2)
    {
        triggerWhaleSfx();
        return;
    }
}

void KimuVerbAudioProcessorEditor::mouseMove(const juce::MouseEvent& event)
{
    // Update hover state for orca body parts
    juce::String newHover;
    for (const auto& [partName, bounds] : orcaBodyParts)
    {
        if (bounds.contains(event.position))
        {
            newHover = partName;
            break;
        }
    }

    if (newHover != hoveredBodyPart)
    {
        hoveredBodyPart = newHover;
        repaint();
    }

    updateOrcaTooltip(newHover);

    const auto pos = event.position;
    if (orcaHit.contains(pos.toInt()))
    {
        const auto nowMs = juce::Time::getMillisecondCounter();
        const auto dist = pos.getDistanceFrom(lastRipplePos);
        if ((nowMs - lastRippleMs) > 40 && dist > 8.0f)
        {
            addRipple(pos);
            lastRippleMs = nowMs;
            lastRipplePos = pos;
        }
    }

    wasInOrca = orcaHit.contains(event.position.toInt());
    wasInVisualizer = visualizerHit.contains(event.position.toInt());
    wasInBottom = bottomHit.contains(event.position.toInt());
}

void KimuVerbAudioProcessorEditor::timerCallback()
{
    const auto nowMs = juce::Time::getMillisecondCounter();
    if (lastAnimMs == 0)
        lastAnimMs = nowMs;

    if (! isShowing())
        return;

    if (nowMs - lastMoveMs < 250)
        return;

    const float deltaTime = (nowMs - lastAnimMs) / 1000.0f;
    lastAnimMs = nowMs;

    if (! isVisible() || getWidth() == 0 || getHeight() == 0)
        return;

    updateRipples(deltaTime);
    waveEffect.update(deltaTime);

    repaint();
}

void KimuVerbAudioProcessorEditor::addRipple(juce::Point<float> pos)
{
    if (ripples.size() >= 16)
        ripples.erase(ripples.begin());

    WaterRipple ripple;
    ripple.position = pos;
    ripple.radius = 2.0f;
    ripple.maxRadius = 60.0f;
    ripple.alpha = 0.7f;
    ripple.active = true;

    ripples.push_back(ripple);
}

void KimuVerbAudioProcessorEditor::updateRipples(float deltaTime)
{
    for (auto& ripple : ripples)
    {
        if (! ripple.active)
            continue;
        ripple.radius += deltaTime * 120.0f;
        ripple.alpha -= deltaTime * 1.2f;
        if (ripple.radius >= ripple.maxRadius || ripple.alpha <= 0.0f)
            ripple.active = false;
    }

    ripples.erase(std::remove_if(ripples.begin(), ripples.end(),
                                 [](const WaterRipple& r) { return ! r.active; }),
                  ripples.end());
}

void KimuVerbAudioProcessorEditor::drawRipples(juce::Graphics& g, const juce::Rectangle<float>& area)
{
    if (ripples.empty())
        return;

    juce::Graphics::ScopedSaveState state(g);
    g.reduceClipRegion(area.toNearestInt());

    const auto rippleColor = juce::Colour(180, 240, 255);
    for (const auto& ripple : ripples)
    {
        if (! ripple.active || ripple.alpha <= 0.0f)
            continue;
        const auto r = ripple.radius;
        const auto rect = juce::Rectangle<float>(ripple.position.x - r, ripple.position.y - r, r * 2.0f, r * 2.0f);
        g.setColour(rippleColor.withAlpha(ripple.alpha * 0.6f));
        g.drawEllipse(rect, 1.5f);
    }
}

void KimuVerbAudioProcessorEditor::drawWaveOverlay(juce::Graphics& g, const juce::Rectangle<float>& area)
{
    juce::Graphics::ScopedSaveState state(g);
    g.reduceClipRegion(area.toNearestInt());

    juce::Path wave;
    const float left = area.getX();
    const float right = area.getRight();
    const float baseY = area.getY() + area.getHeight() * 0.85f;

    wave.startNewSubPath(left, baseY + waveEffect.getWaveHeight(left));
    for (float x = left; x <= right; x += 4.0f)
    {
        const float y = baseY + waveEffect.getWaveHeight(x);
        wave.lineTo(x, y);
    }

    g.setColour(juce::Colour(120, 200, 255).withAlpha(0.25f));
    g.strokePath(wave, juce::PathStrokeType(2.5f));
}

void KimuVerbAudioProcessorEditor::draw3DMarineEffects(juce::Graphics& g, const juce::Rectangle<float>& bounds)
{
    if (bounds.getWidth() < 100.0f || bounds.getHeight() < 100.0f)
        return;

    juce::ColourGradient deepOcean(juce::Colour(3, 15, 30).withAlpha(0.2f), 0, bounds.getBottom(),
                                   juce::Colour(8, 25, 45).withAlpha(0.2f), bounds.getWidth(), bounds.getY(), false);
    g.setGradientFill(deepOcean);
    g.fillAll();

    const float waveOffset = std::sin(waveEffect.waveOffset) * 15.0f;
    juce::ColourGradient midOcean(juce::Colour(10, 35, 60).withAlpha(0.15f), bounds.getX() + waveOffset, bounds.getY() * 0.3f,
                                  juce::Colour(20, 50, 80).withAlpha(0.15f), bounds.getRight() - waveOffset, bounds.getHeight() * 0.7f, false);
    g.setGradientFill(midOcean);
    g.fillAll();

    const auto nowMs = juce::Time::getMillisecondCounter();
    if (nowMs % 3 == 0)
        drawWaterCaustics(g, bounds);
    if (nowMs % 2 == 0)
        drawDepthParticles(g, bounds);
}

void KimuVerbAudioProcessorEditor::drawWaterCaustics(juce::Graphics& g, const juce::Rectangle<float>& bounds)
{
    juce::Graphics::ScopedSaveState state(g);

    for (int i = 0; i < 5; ++i)
    {
        const float x = bounds.getX() + (bounds.getWidth() * 0.2f * i) + std::sin(waveEffect.waveOffset + (float) i) * 30.0f;
        const float y = bounds.getY() + (bounds.getHeight() * 0.3f) + std::cos(waveEffect.waveOffset * 0.8f + (float) i) * 20.0f;
        const float size = 80.0f + std::sin(waveEffect.waveOffset * 1.2f + (float) i) * 20.0f;

        juce::ColourGradient caustic(juce::Colour(100, 200, 255).withAlpha(0.1f), x, y,
                                     juce::Colour(150, 220, 255).withAlpha(0.05f), x + size, y + size, true);
        g.setGradientFill(caustic);
        g.fillEllipse(x, y, size, size);
    }
}

void KimuVerbAudioProcessorEditor::drawDepthParticles(juce::Graphics& g, const juce::Rectangle<float>& bounds)
{
    juce::Graphics::ScopedSaveState state(g);

    for (int i = 0; i < 5; ++i)
    {
        const float particleX = bounds.getX() + (bounds.getWidth() * (0.15f + i * 0.15f))
                                + std::sin(waveEffect.waveOffset * 0.3f + i * 2.0f) * 30.0f;
        const float particleY = bounds.getY() + (bounds.getHeight() * (0.25f + i * 0.1f))
                                + waveEffect.waveOffset * 15.0f;
        const float particleSize = 1.5f + std::sin(waveEffect.waveOffset + (float) i) * 0.5f;
        const float wrappedY = particleY > bounds.getBottom() ? bounds.getY() : particleY;

        g.setColour(juce::Colour(200, 240, 255).withAlpha(0.2f));
        g.fillEllipse(particleX, wrappedY, particleSize, particleSize);
    }
}

void KimuVerbAudioProcessorEditor::showOrcaTooltip(const juce::String& title, const juce::String& content, const juce::Rectangle<int>& anchor)
{
    if (! currentTooltip)
    {
        currentTooltip = std::make_unique<OrcaTooltip>();
        addAndMakeVisible(*currentTooltip);
    }

    currentTooltip->setText(title, content);

    const int tooltipW = 160;
    const int tooltipH = 58;
    auto x = anchor.getCentreX() - tooltipW / 2;
    auto y = anchor.getY() - tooltipH - 6;

    auto bounds = getLocalBounds().reduced(4);
    x = juce::jlimit(bounds.getX(), bounds.getRight() - tooltipW, x);
    y = juce::jlimit(bounds.getY(), bounds.getBottom() - tooltipH, y);

    auto target = juce::Rectangle<int>(x, y, tooltipW, tooltipH);
    currentTooltip->setBounds(target);
    currentTooltip->toFront(true);

    currentTooltip->setAlpha(0.0f);
    animator.animateComponent(currentTooltip.get(), target, 1.0f, 140, false, 1.0, 1.0);
    tooltipVisible = true;
}

void KimuVerbAudioProcessorEditor::hideOrcaTooltip()
{
    if (! currentTooltip)
        return;

    animator.animateComponent(currentTooltip.get(), currentTooltip->getBounds(), 0.0f, 120, false, 1.0, 1.0);
    tooltipVisible = false;
}

void KimuVerbAudioProcessorEditor::updateOrcaTooltip(const juce::String& newHover)
{
    if (newHover.isEmpty())
    {
        if (tooltipVisible)
            hideOrcaTooltip();
        currentHoveredPart.clear();
        return;
    }

    if (newHover == currentHoveredPart)
        return;

    juce::Slider* slider = nullptr;
    juce::String title;
    if (newHover == "Head")
    {
        slider = &preDelaySlider;
        title = "PREDELAY";
    }
    else if (newHover == "DorsalFin")
    {
        slider = &sizeSlider;
        title = "SIZE";
    }
    else if (newHover == "UpperBody")
    {
        slider = &decaySlider;
        title = "DECAY";
    }
    else if (newHover == "Belly")
    {
        slider = &mixSlider;
        title = "MIX";
    }
    else if (newHover == "Tail")
    {
        slider = &depthSlider;
        title = "DEPTH";
    }
    else if (newHover == "LowerFin")
    {
        slider = &dampingSlider;
        title = "DAMPING";
    }

    if (slider == nullptr)
        return;

    const auto valueText = "VALUE: " + juce::String(slider->getValue(), 2);
    showOrcaTooltip(title, valueText, slider->getBounds());
    currentHoveredPart = newHover;
}

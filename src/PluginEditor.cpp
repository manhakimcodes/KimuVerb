#include "PluginEditor.h"
#include <BinaryData.h>

//==============================================================================
// OceanLookAndFeel Implementation
//==============================================================================

void OceanLookAndFeel::drawRotarySlider(juce::Graphics& g, int x, int y, int width, int height, float sliderPosProportional, float rotaryStartAngle, float rotaryEndAngle, juce::Slider& slider)
{
    auto bounds = juce::Rectangle<float>(x, y, width, height).reduced(8.0f);
    auto radius = juce::jmin(bounds.getWidth(), bounds.getHeight()) * 0.5f;
    auto centre = bounds.getCentre();
    auto angle = rotaryStartAngle + sliderPosProportional * (rotaryEndAngle - rotaryStartAngle);

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
g.strokePath(arc, juce::PathStrokeType(3.0f));

    juce::Path pointer;
    auto pointerLength = radius * 0.7f;
    auto pointerThickness = 3.0f;
    pointer.addLineSegment(juce::Line<float>(centre.x, centre.y,
                                             centre.x + std::sin(angle) * pointerLength,
                                             centre.y - std::cos(angle) * pointerLength), pointerThickness);
    g.setColour(juce::Colour(150, 220, 255));
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
    g.setColour(juce::Colour(200, 220, 240));
    g.setFont(juce::Font(12.0f));
    g.drawFittedText(label.getText(), label.getLocalBounds(), juce::Justification::centred, 1);
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

KimuVerbAudioProcessorEditor::KimuVerbAudioProcessorEditor(KimuVerbAudioProcessor& p)
: juce::AudioProcessorEditor(&p), processor(p)
{
    setLookAndFeel(&lnf);
    loadImages();
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
    
    setSize(900, 600);
    createOrcaBodyMapping();
    layoutOrcaControls();
    layoutUIComponents();
}

KimuVerbAudioProcessorEditor::~KimuVerbAudioProcessorEditor()
{
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
    else
    {
        juce::ColourGradient oceanGradient(juce::Colour(5, 25, 45), 0, 0,
                                       juce::Colour(15, 45, 75), bounds.getWidth(), bounds.getHeight(), false);
        g.setGradientFill(oceanGradient);
        g.fillAll();
    }
    
    // Draw orca image (fallback to path if missing)
    auto contentArea = bounds;
    contentArea.removeFromRight(220.0f);
    contentArea.removeFromLeft(160.0f);
    contentArea.removeFromBottom(110.0f);
    auto orcaArea = contentArea.reduced(10.0f);
    if (orcaImage.isValid())
        g.drawImageWithin(orcaImage,
                          (int) orcaArea.getX(), (int) orcaArea.getY(),
                          (int) orcaArea.getWidth(), (int) orcaArea.getHeight(),
                          juce::RectanglePlacement::centred);
    else
    {
        g.setColour(juce::Colour(20, 40, 60).withAlpha(0.9f));
        g.fillPath(orcaPath);
        g.setColour(juce::Colour(100, 200, 255).withAlpha(0.4f));
        g.strokePath(orcaPath, juce::PathStrokeType(3.0f));
    }

    // Logo (top-left)
    if (logoImage.isValid())
    {
        auto logoArea = bounds.removeFromTop(70.0f).removeFromLeft(320.0f).reduced(10.0f);
        g.drawImageWithin(logoImage,
                          (int) logoArea.getX(), (int) logoArea.getY(),
                          (int) logoArea.getWidth(), (int) logoArea.getHeight(),
                          juce::RectanglePlacement::centred);
    }

    auto drawLabel = [&](const juce::Image& img, const juce::Slider& s, float yOffset)
    {
        if (! img.isValid())
            return;
        auto r = s.getBounds().toFloat().translated(0.0f, yOffset).expanded(20.0f, 10.0f);
        g.drawImageWithin(img,
                          (int) r.getX(), (int) r.getY(),
                          (int) r.getWidth(), (int) r.getHeight(),
                          juce::RectanglePlacement::centred);
    };
    drawLabel(labelPreDelayImage, preDelaySlider, -30.0f);
    drawLabel(labelSizeImage, sizeSlider, -30.0f);
    drawLabel(labelMixImage, mixSlider, -30.0f);
    
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
    const float knobSize = 80.0f;
    auto place = [&](const juce::String& key, float nx, float ny)
    {
        auto x = orcaArea.getX() + nx * orcaArea.getWidth();
        auto y = orcaArea.getY() + ny * orcaArea.getHeight();
        orcaBodyParts[key] = juce::Rectangle<float>(x - knobSize * 0.5f, y - knobSize * 0.5f, knobSize, knobSize);
    };
    place("Head", 0.25f, 0.35f);       // Pre-delay
    place("DorsalFin", 0.75f, 0.20f);  // Size
    place("UpperBody", 0.80f, 0.50f);  // Decay
    place("Belly", 0.30f, 0.75f);      // Mix
    place("Tail", 0.85f, 0.80f);       // Depth
    place("LowerFin", 0.50f, 0.85f);   // Damping
}

void KimuVerbAudioProcessorEditor::setupParameters()
{
    auto setupSlider = [&](juce::Slider& s) {
        s.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
        s.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
        s.setLookAndFeel(&lnf);
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
        l.setFont(juce::Font(12.0f, juce::Font::bold));
        l.setJustificationType(juce::Justification::centred);
        addAndMakeVisible(l);
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
    setupLabel(decayLabel, "DECAY");
    setupLabel(dampingLabel, "DAMPING");
    setupLabel(depthLabel, "DEPTH");
    
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

    auto placeLabel = [&](juce::Label& l, const juce::Slider& s)
    {
        auto r = s.getBounds().toFloat().translated(0.0f, -22.0f).expanded(8.0f, 4.0f);
        l.setBounds(r.toNearestInt());
    };
    placeLabel(decayLabel, decaySlider);
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
    
    // Advanced parameters on the left side
    // Replace lines 627-634 with this:
    // Advanced parameters on the left side - LARGER and LOWER
    auto leftPanel = bounds.removeFromLeft(180).reduced(10);
    leftPanel = leftPanel.withTop(leftPanel.getY() + 40); // Move down 40px
    
    auto placeLeft = [&](juce::Label& l, juce::Slider& s)
    {
        l.setBounds(leftPanel.removeFromTop(16)); // Larger labels (14->16)
        s.setBounds(leftPanel.removeFromTop(60)); // Larger knobs (48->60)
        leftPanel.removeFromTop(8); // More spacing (6->8)
    };
    
    placeLeft(toneLabel, toneSlider);
    placeLeft(widthLabel, widthSlider);
    placeLeft(lowCutLabel, lowCutSlider);
    placeLeft(highCutLabel, highCutSlider);
    placeLeft(motionLabel, motionSlider);
    placeLeft(diffusionLabel, diffusionSlider);
    
    // Oceanic controls at bottom left
    auto oceanicPanel = bounds.removeFromBottom(120).reduced(10);
    auto placeBottom = [&](juce::Label& l, juce::Slider& s)
    {
        auto col = oceanicPanel.removeFromLeft(90);
        l.setBounds(col.removeFromTop(14));
        s.setBounds(col.removeFromTop(60));
    };
    placeBottom(currentLabel, currentSlider);
    placeBottom(pressureLabel, pressureSlider);
    placeBottom(depthShiftLabel, depthShiftSlider);
    
    // Preset navigation at bottom center
    auto presetPanel = bounds.removeFromBottom(60).reduced(10);
    prevPresetButton.setBounds(presetPanel.removeFromLeft(40));
    nextPresetButton.setBounds(presetPanel.removeFromRight(40));
    presetNameLabel.setBounds(presetPanel);
    presetComboBox.setBounds(presetPanel.reduced(10));
    
    // Logging toggle
    loggingToggle.setBounds(bounds.removeFromTop(30).removeFromRight(100));
    
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
    presetComboBox.setColour(juce::ComboBox::backgroundColourId, juce::Colour(20, 40, 60));
    presetComboBox.setColour(juce::ComboBox::textColourId, juce::Colour(200, 220, 240));
    presetComboBox.setColour(juce::ComboBox::outlineColourId, juce::Colour(100, 200, 255));

    presetComboBox.onChange = [this]()
    {
        int presetIndex = presetComboBox.getSelectedId() - 1;
        if (presetIndex >= 0 && presetIndex < presetManager->factoryPresets.size())
        {
            presetManager->loadPreset(presetIndex, processor.getValueTreeState());
            updatePresetDisplay();
        }
    };
}

void KimuVerbAudioProcessorEditor::setupPresetNavigation()
{
    prevPresetButton.setButtonText("<");
    nextPresetButton.setButtonText(">");
    prevPresetButton.onClick = [this]() {
        if (presetManager->currentPresetIndex > 0)
        {
            presetManager->loadPreset(presetManager->currentPresetIndex - 1, processor.getValueTreeState());
            updatePresetDisplay();
        }
    };
    
    nextPresetButton.onClick = [this]() {
        if (presetManager->currentPresetIndex < presetManager->factoryPresets.size() - 1)
        {
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
}

void KimuVerbAudioProcessorEditor::mouseDown(const juce::MouseEvent& event)
{
    // Handle clicks on orca body parts
}

void KimuVerbAudioProcessorEditor::mouseMove(const juce::MouseEvent& event)
{
    // Update hover state for orca body parts
    hoveredBodyPart = "";
    
    for (const auto& [partName, bounds] : orcaBodyParts)
    {
        if (bounds.contains(event.position))
        {
            hoveredBodyPart = partName;
            repaint();
            return;
        }
    }
    
    if (hoveredBodyPart.isNotEmpty())
    {
        hoveredBodyPart = "";
        repaint();
    }
}


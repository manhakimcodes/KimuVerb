#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"

class OceanLookAndFeel : public juce::LookAndFeel_V4
{
public:
    void setKnobImage(const juce::Image& img);
    void setKnobStripImage(const juce::Image& img);
    void drawRotarySlider(juce::Graphics& g, int x, int y, int width, int height, float sliderPosProportional, float rotaryStartAngle, float rotaryEndAngle, juce::Slider& slider) override;
    void drawLinearSlider(juce::Graphics& g, int x, int y, int width, int height, float sliderPos, float minSliderPos, float maxSliderPos, const juce::Slider::SliderStyle, juce::Slider&) override;
    void drawToggleButton(juce::Graphics& g, juce::ToggleButton& button, bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown) override;
    void drawLabel(juce::Graphics& g, juce::Label& label) override;
private:
    juce::Image knobImage;
    juce::Image knobStripImage;
    int knobStripFrames = 0;
};

class WaveformVisualizer : public juce::Component, private juce::Timer
{
public:
    WaveformVisualizer();
    void paint(juce::Graphics& g) override;
    void resized() override;
    void pushSample(float sample);
    
private:
    void timerCallback() override;
    
    static constexpr int bufferSize = 1024;
    juce::AudioBuffer<float> waveformBuffer;
    int writePosition = 0;
    juce::Path waveformPath;
};

class PresetManager
{
public:
    struct Preset
    {
        juce::String name;
        std::map<juce::String, float> parameters;
    };
    
    std::vector<Preset> factoryPresets;
    int currentPresetIndex = 0;
    
    void initializeFactoryPresets();
    void loadPreset(int index, juce::AudioProcessorValueTreeState& params);
    juce::String getCurrentPresetName() const;
};

class OceanBackground : public juce::Component, private juce::Timer
{
public:
    OceanBackground();
    void paint(juce::Graphics& g) override;
    void resized() override;
    
private:
    void timerCallback() override;
    
    float animationOffset = 0.0f;
    std::vector<juce::Point<float>> particles;
    std::vector<juce::Line<float>> lightRays;
};

class KimuVerbAudioProcessorEditor : public juce::AudioProcessorEditor, private juce::Timer
{
public:
    KimuVerbAudioProcessorEditor(KimuVerbAudioProcessor&);
    ~KimuVerbAudioProcessorEditor() override;

    void paint(juce::Graphics&) override;
    void resized() override;

private:
    void timerCallback() override;
    KimuVerbAudioProcessor& processor;

    OceanLookAndFeel lnf;
    
    // Main parameters mapped to orca body
    juce::Slider preDelaySlider;    // Orca Head
    juce::Slider sizeSlider;       // Top Dorsal Fin  
    juce::Slider decaySlider;       // Upper Body
    juce::Slider mixSlider;        // Belly
    juce::Slider depthSlider;       // Tail
    juce::Slider dampingSlider;    // Lower Fin
    
    // Additional parameters
    juce::Slider toneSlider;
    juce::Slider widthSlider;
    juce::Slider lowCutSlider;
    juce::Slider highCutSlider;
    juce::Slider motionSlider;
    juce::Slider diffusionSlider;
    
    // Advanced controls
    juce::Slider currentSlider;     // Oceanic control
    juce::Slider pressureSlider;   // Oceanic control
    juce::Slider depthShiftSlider; // Oceanic control
    
    juce::ToggleButton loggingToggle { "Logging" };

    // UI Components
    std::unique_ptr<WaveformVisualizer> visualizer;
    std::unique_ptr<OceanBackground> oceanBg;
    std::unique_ptr<PresetManager> presetManager;
    
    // Preset navigation
    juce::TextButton prevPresetButton { "◀" };
    juce::TextButton nextPresetButton { "▶" };
    juce::Label presetNameLabel;
    
    // Parameter attachments
    using SliderAttachment = juce::AudioProcessorValueTreeState::SliderAttachment;
    using ButtonAttachment = juce::AudioProcessorValueTreeState::ButtonAttachment;
    
    std::unique_ptr<SliderAttachment> preDelayAttachment;
    std::unique_ptr<SliderAttachment> sizeAttachment;
    std::unique_ptr<SliderAttachment> decayAttachment;
    std::unique_ptr<SliderAttachment> mixAttachment;
    std::unique_ptr<SliderAttachment> depthAttachment;
    std::unique_ptr<SliderAttachment> dampingAttachment;
    std::unique_ptr<SliderAttachment> toneAttachment;
    std::unique_ptr<SliderAttachment> widthAttachment;
    std::unique_ptr<SliderAttachment> lowCutAttachment;
    std::unique_ptr<SliderAttachment> highCutAttachment;
    std::unique_ptr<SliderAttachment> motionAttachment;
    std::unique_ptr<SliderAttachment> diffusionAttachment;
    std::unique_ptr<SliderAttachment> currentAttachment;
    std::unique_ptr<SliderAttachment> pressureAttachment;
    std::unique_ptr<SliderAttachment> depthShiftAttachment;
    std::unique_ptr<ButtonAttachment> loggingAttachment;

    // UI assets
    juce::Image backgroundImage;
    juce::Image orcaImage;
    juce::Image logoImage;
    juce::Image waveImage;
    juce::Image knobStripImage;
    juce::Image labelMixImage;
    juce::Image labelSizeImage;
    juce::Image labelPreDelayImage;
    juce::Image knobImage;

    juce::Label toneLabel;
    juce::Label widthLabel;
    juce::Label lowCutLabel;
    juce::Label highCutLabel;
    juce::Label motionLabel;
    juce::Label diffusionLabel;
    juce::Label currentLabel;
    juce::Label pressureLabel;
    juce::Label depthShiftLabel;
    juce::Label preDelayLabel;
    juce::Label sizeLabel;
    juce::Label decayLabel;
    juce::Label dampingLabel;
    juce::Label depthLabel;
    juce::Label mixLabel;

    // Preset dropdown
    juce::ComboBox presetComboBox;
    juce::StringArray presetNames;

    // Orca graphics
    juce::Path orcaPath;
    std::map<juce::String, juce::Rectangle<float>> orcaBodyParts;
    
    void createOrcaBodyMapping();
    void setupParameters();
    void layoutOrcaControls();
    void layoutUIComponents();
    void setupPresetNavigation();
    void setupPresetDropdown();
    void updatePresetDisplay();
    void loadImages();
    juce::Image loadImageFromBinary(const char* name);
    juce::Image loadImageFromFile(const juce::String& relativePath);
    
    // Mouse interaction for orca body parts
    void mouseDown(const juce::MouseEvent& event) override;
    void mouseMove(const juce::MouseEvent& event) override;
    juce::String hoveredBodyPart;
    struct OrcaTooltip;
    void showOrcaTooltip(const juce::String& title, const juce::String& content, const juce::Rectangle<int>& anchor);
    void hideOrcaTooltip();
    void updateOrcaTooltip(const juce::String& newHover);

    struct UiSampleSource;
    struct SampleData;

    void setupUiAudio();
    void shutdownUiAudio();
    void triggerWhaleSfx();
    void triggerWaterSfx();
    void triggerBetterOceanSfx(double durationSeconds, double startOffsetSeconds);
    double getPresetOceanDuration(int presetIndex) const;
    juce::Rectangle<int> computeOrcaArea() const;
    void loadAudioFiles();
    bool loadAudioFile(const juce::File& file, SampleData& outData, const juce::String& label);

    struct WaterRipple
    {
        juce::Point<float> position {};
        float radius = 0.0f;
        float maxRadius = 60.0f;
        float alpha = 0.0f;
        bool active = false;
    };

    struct WaveEffect
    {
        float waveOffset = 0.0f;
        float waveSpeed = 0.9f;
        float waveAmplitude = 3.0f;

        void update(float deltaTime)
        {
            waveOffset += waveSpeed * deltaTime;
            if (waveOffset > juce::MathConstants<float>::twoPi)
                waveOffset -= juce::MathConstants<float>::twoPi;
        }

        float getWaveHeight(float x) const
        {
            return std::sin(x * 0.012f + waveOffset) * waveAmplitude;
        }
    };

    void addRipple(juce::Point<float> pos);
    void updateRipples(float deltaTime);
    void drawRipples(juce::Graphics& g, const juce::Rectangle<float>& area);
    void drawWaveOverlay(juce::Graphics& g, const juce::Rectangle<float>& area);
    void draw3DMarineEffects(juce::Graphics& g, const juce::Rectangle<float>& bounds);
    void drawWaterCaustics(juce::Graphics& g, const juce::Rectangle<float>& bounds);
    void drawDepthParticles(juce::Graphics& g, const juce::Rectangle<float>& bounds);

    juce::AudioDeviceManager uiAudioDevice;
    juce::AudioSourcePlayer uiAudioPlayer;
    juce::MixerAudioSource uiMixer;
    std::unique_ptr<UiSampleSource> whaleSample;
    std::unique_ptr<UiSampleSource> betterOceanSample;
    juce::AudioFormatManager uiFormatManager;
    std::shared_ptr<SampleData> whaleData;
    std::shared_ptr<SampleData> betterOceanData;
    bool uiAudioReady = false;
    juce::Random uiRng;
    uint32 lastWhaleMs = 0;
    uint32 lastWaterMs = 0;
    juce::Rectangle<int> orcaHit;
    juce::Rectangle<int> visualizerHit;
    juce::Rectangle<int> bottomHit;
    bool wasInOrca = false;
    bool wasInVisualizer = false;
    bool wasInBottom = false;
    juce::ComponentAnimator animator;
    std::unique_ptr<OrcaTooltip> currentTooltip;
    juce::String currentHoveredPart;
    bool tooltipVisible = false;

    std::vector<WaterRipple> ripples;
    WaveEffect waveEffect;
    juce::Point<float> lastRipplePos { -1000.0f, -1000.0f };
    uint32 lastRippleMs = 0;
    uint32 lastAnimMs = 0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(KimuVerbAudioProcessorEditor)
};

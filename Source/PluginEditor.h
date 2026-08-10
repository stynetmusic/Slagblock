#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include "PluginProcessor.h"

//==============================================================================
// Commercial-grade LookAndFeel (Serum / Vital / Pigments vibe)
//==============================================================================
class SlagLookAndFeel : public juce::LookAndFeel_V4
{
public:
    // Palette
    static constexpr juce::uint32 kBg0      = 0xff0b0b10;
    static constexpr juce::uint32 kBg1      = 0xff101018;
    static constexpr juce::uint32 kPanel    = 0xff15151f;
    static constexpr juce::uint32 kPanelHi  = 0xff1c1c28;
    static constexpr juce::uint32 kInset    = 0xff0a0a0f;
    static constexpr juce::uint32 kBorder   = 0xff2a2a3a;
    static constexpr juce::uint32 kBorderHi = 0xff3d3d52;
    static constexpr juce::uint32 kText     = 0xffeceaf2;
    static constexpr juce::uint32 kTextDim  = 0xff8a8798;
    static constexpr juce::uint32 kTextMute = 0xff5a5768;
    static constexpr juce::uint32 kCyan     = 0xff2ee6d6;
    static constexpr juce::uint32 kCyanHi   = 0xff7af5ea;
    static constexpr juce::uint32 kViolet   = 0xffa78bfa;
    static constexpr juce::uint32 kOrange   = 0xffff9f43;
    static constexpr juce::uint32 kTrack    = 0xff1a1a26;

    SlagLookAndFeel();

    void drawRotarySlider (juce::Graphics&, int x, int y, int w, int h,
                           float sliderPosProportional, float rotaryStartAngle,
                           float rotaryEndAngle, juce::Slider&) override;

    void drawLinearSlider (juce::Graphics&, int x, int y, int width, int height,
                           float sliderPos, float minSliderPos, float maxSliderPos,
                           juce::Slider::SliderStyle, juce::Slider&) override;

    void drawButtonBackground (juce::Graphics&, juce::Button&,
                               const juce::Colour& backgroundColour,
                               bool shouldDrawButtonAsHighlighted,
                               bool shouldDrawButtonAsDown) override;

    void drawToggleButton (juce::Graphics&, juce::ToggleButton&,
                           bool shouldDrawButtonAsHighlighted,
                           bool shouldDrawButtonAsDown) override;

    void drawComboBox (juce::Graphics&, int width, int height, bool isButtonDown,
                       int buttonX, int buttonY, int buttonW, int buttonH,
                       juce::ComboBox&) override;

    void drawPopupMenuBackground (juce::Graphics&, int width, int height) override;
    void drawPopupMenuItem (juce::Graphics&, const juce::Rectangle<int>& area,
                            bool isSeparator, bool isActive, bool isHighlighted,
                            bool isTicked, bool hasSubMenu,
                            const juce::String& text, const juce::String& shortcutKeyText,
                            const juce::Drawable* icon, const juce::Colour* textColour) override;

    juce::Font getTextButtonFont (juce::TextButton&, int buttonHeight) override;
    juce::Font getLabelFont (juce::Label&) override;
    juce::Font getComboBoxFont (juce::ComboBox&) override;
};

//==============================================================================
class SlagblockAudioProcessorEditor : public juce::AudioProcessorEditor,
                                      private juce::Timer
{
public:
    explicit SlagblockAudioProcessorEditor (SlagblockAudioProcessor&);
    ~SlagblockAudioProcessorEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    void timerCallback() override;

    void setupKnob   (juce::Slider& s, juce::Label& lab, const juce::String& name);
    void setupVFader (juce::Slider& s, juce::Label& lab, const juce::String& name);
    void setupHSlider(juce::Slider& s, juce::Label& lab, const juce::String& name);
    void setupToggle (juce::ToggleButton& b, const juce::String& text);
    void setupButton (juce::TextButton& b, const juce::String& text, bool toggle = false);
    void setupSectionTitle (juce::Label& lab, const juce::String& text);

    void layoutKnobRow (juce::Rectangle<int> area,
                        juce::Slider* const* knobs,
                        juce::Label*  const* labels,
                        int count);
    void layoutVFaders (juce::Rectangle<int> area,
                        juce::Slider* const* faders,
                        juce::Label*  const* labels,
                        int count);
    void layoutHRows (juce::Rectangle<int> area,
                      juce::Slider* const* sliders,
                      juce::Label*  const* labels,
                      int count,
                      int labelW = 78);
    void layoutShaper (juce::Rectangle<int> area);

    void drawGlassPanel (juce::Graphics& g, juce::Rectangle<float> r, float radius = 14.0f);
    void drawInsetWell  (juce::Graphics& g, juce::Rectangle<float> r, float radius = 10.0f);
    void drawSectionChrome (juce::Graphics& g, juce::Rectangle<float> r,
                            const juce::String& title, juce::Colour accent);
    void drawWaveform   (juce::Graphics& g, juce::Rectangle<float> r);
    void drawSpectrum   (juce::Graphics& g, juce::Rectangle<float> r);
    void drawEnvMini    (juce::Graphics& g, juce::Rectangle<float> r);
    void drawTopBar     (juce::Graphics& g, juce::Rectangle<int> r);
    void drawBottomBar  (juce::Graphics& g, juce::Rectangle<int> r);

    float readParam (const char* id, float fallback) const;
    void updateLfoRateEnabled();
    void syncFromProcessor();
    void setFxOn (bool on);
    void setImOn (bool on);
    void savePresetClicked();
    void loadPresetClicked();

    SlagLookAndFeel laf;
    SlagblockAudioProcessor& proc;

    // OSC
    juce::Slider ratioKnob, indexKnob, driveKnob, feedbackKnob;
    juce::Label  ratioLab, indexLab, driveLab, feedbackLab;

    // SUB
    juce::Slider subLevelKnob, subOctaveKnob;
    juce::Label  subLevelLab, subOctaveLab;

    // FILTER
    juce::Slider cutoffKnob, resoKnob, noiseKnob;
    juce::Label  cutoffLab, resoLab, noiseLab;

    // ADSR vertical
    juce::Slider attackFader, decayFader, sustainFader, releaseFader;
    juce::Label  attackLab, decayLab, sustainLab, releaseLab;

    // LFO
    juce::Slider lfoRateKnob, lfoCutKnob, lfoIdxKnob, lfoDrvKnob, lfoAmpKnob;
    juce::Label  lfoRateLab, lfoCutLab, lfoIdxLab, lfoDrvLab, lfoAmpLab;
    juce::Label  lfoSyncLab, lfoGridLab;
    juce::ToggleButton lfoSyncBtn { "SYNC" };
    juce::ToggleButton lfoAmpBtn  { "AMP" };
    juce::ComboBox lfoGridBox;

    // FX
    juce::Slider fxDist, fxBits, fxRate, fxDly, fxDlyFb, fxDlyMix, fxCho, fxChoDep;
    juce::Label  fxDistLab, fxBitsLab, fxRateLab, fxDlyLab, fxDlyFbLab, fxDlyMixLab, fxChoLab, fxChoDepLab;

    // Shaper
    juce::ToggleButton imVolOn, imDriveOn, imTimeOn, imFiltOn, imCrushOn, imNoiseOn, imWidthOn;
    juce::Slider imVol, imDrive, imTime, imFilt, imCrush, imNoise, imWidth;
    juce::Label  imVolLab, imDriveLab, imTimeLab, imFiltLab, imCrushLab, imNoiseLab, imWidthLab;

    // Chrome labels
    juce::Label logoLabel, presetLabel, tagLabel;
    juce::Label oscTitle, subTitle, filterTitle, envTitle, lfoTitle, fxTitle, shaperTitle;

    // Buttons
    juce::TextButton btnSave, btnLoad, btnUndo, btnRedo;
    juce::TextButton btnRandom, btnRandomFx, btnFx, btnShaper, btnRandomIm;

    bool fxOn = false;
    bool imOn = false;

    // Geometry
    juce::Rectangle<int> topBar, bottomBar;
    juce::Rectangle<int> oscPanel, subPanel, filterPanel, lfoPanel;
    juce::Rectangle<int> displayPanel, envPanel, fxPanel, shaperPanel;
    juce::Rectangle<float> waveArea, spectrumArea, envMiniArea;

    float phase = 0.0f;
    float spectrumPhase = 0.0f;

    std::unique_ptr<juce::FileChooser> fileChooser;

    using SA = juce::AudioProcessorValueTreeState::SliderAttachment;
    using BA = juce::AudioProcessorValueTreeState::ButtonAttachment;
    using CA = juce::AudioProcessorValueTreeState::ComboBoxAttachment;

    std::unique_ptr<SA> aRatio, aIndex, aDrive, aFeedback;
    std::unique_ptr<SA> aCut, aReso, aNoise;
    std::unique_ptr<SA> aAtt, aDec, aSus, aRel;
    std::unique_ptr<SA> aSubLvl, aSubOct;
    std::unique_ptr<SA> aLfoRate, aLfoCut, aLfoIdx, aLfoDrv, aLfoAmp;
    std::unique_ptr<BA> aLfoSync, aLfoToAmp;
    std::unique_ptr<CA> aLfoGrid;
    std::unique_ptr<SA> aFxDist, aFxBits, aFxRate, aFxDly, aFxDlyFb, aFxDlyMix, aFxCho, aFxChoDep;
    std::unique_ptr<SA> aImVol, aImDrive, aImTime, aImFilt, aImCrush, aImNoise, aImWidth;
    std::unique_ptr<BA> aImVolOn, aImDriveOn, aImTimeOn, aImFiltOn, aImCrushOn, aImNoiseOn, aImWidthOn;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SlagblockAudioProcessorEditor)
};

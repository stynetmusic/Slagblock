#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include "PluginProcessor.h"

//==============================================================================
// PHASEDYN LookAndFeel — teal panels, bronze knobs, gold viz (from refer.png)
//==============================================================================
class SlagLookAndFeel : public juce::LookAndFeel_V4
{
public:
    // Exact PHASEDYN palette from refer.png
    static constexpr juce::uint32 kFrame     = 0xff1c1610; // dark brown wood frame
    static constexpr juce::uint32 kFrameHi   = 0xff2a2218;
    static constexpr juce::uint32 kHeader    = 0xff241e16;
    static constexpr juce::uint32 kPanel     = 0xff2f524e; // seafoam teal panel
    static constexpr juce::uint32 kPanelHi   = 0xff3a625c;
    static constexpr juce::uint32 kPanelLo   = 0xff274540;
    static constexpr juce::uint32 kInset     = 0xff1a2e2c;
    static constexpr juce::uint32 kBorder    = 0xff1a2826;
    static constexpr juce::uint32 kBorderHi  = 0xff4a7870;
    static constexpr juce::uint32 kText      = 0xfff0ebe0;
    static constexpr juce::uint32 kTextDim   = 0xffc4bba8;
    static constexpr juce::uint32 kTextMute  = 0xff8a8474;
    static constexpr juce::uint32 kGold      = 0xffc9a84c;
    static constexpr juce::uint32 kGoldHi    = 0xffe8c86a;
    static constexpr juce::uint32 kBronze    = 0xff6b4e2e;
    static constexpr juce::uint32 kBronzeHi  = 0xff8a6a3e;
    static constexpr juce::uint32 kTealKnob  = 0xff3d8a82; // cyan-teal metal (filter/fx)
    static constexpr juce::uint32 kTealKnobHi= 0xff5ab0a6;
    static constexpr juce::uint32 kFaderTrack= 0xff1a2422;
    static constexpr juce::uint32 kBadge     = 0xff3d3830;

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

    /** ComponentId "teal" → teal metal knobs; default bronze. */
    static constexpr const char* kTealKnobId = "teal";
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

    void setupKnob (juce::Slider& s, juce::Label& lab, const juce::String& name, bool teal = false);
    void setupVFader (juce::Slider& s, juce::Label& lab, const juce::String& name);
    void setupHSlider (juce::Slider& s, juce::Label& lab, const juce::String& name);
    void setupToggle (juce::ToggleButton& b, const juce::String& text);
    void setupButton (juce::TextButton& b, const juce::String& text, bool toggle = false);
    void setupSectionTitle (juce::Label& lab, const juce::String& text);
    void setupBadge (juce::Label& lab, const juce::String& text);

    void layoutKnobRow (juce::Rectangle<int> area,
                        juce::Slider* const* knobs,
                        juce::Label*  const* labels,
                        int count);
    void layoutVFaders (juce::Rectangle<int> area,
                        juce::Slider* const* faders,
                        juce::Label*  const* labels,
                        int count);
    void layoutShaper (juce::Rectangle<int> area);

    void drawPanel (juce::Graphics& g, juce::Rectangle<float> r, bool active = true);
    void drawInset (juce::Graphics& g, juce::Rectangle<float> r, float radius = 8.0f);
    void drawWaveform (juce::Graphics& g, juce::Rectangle<float> r);
    void drawSpectrum (juce::Graphics& g, juce::Rectangle<float> r);
    void drawEnvShape (juce::Graphics& g, juce::Rectangle<float> r);
    void drawFilterCurve (juce::Graphics& g, juce::Rectangle<float> r);
    void drawHeader (juce::Graphics& g, juce::Rectangle<int> r);
    void drawFooter (juce::Graphics& g, juce::Rectangle<int> r);

    float readParam (const char* id, float fallback) const;
    void updateLfoRateEnabled();
    void syncFromProcessor();
    void setFxOn (bool on);
    void setImOn (bool on);
    void savePresetClicked();
    void loadPresetClicked();

    SlagLookAndFeel laf;
    SlagblockAudioProcessor& proc;

    // FM Osc
    juce::Slider ratioKnob, indexKnob, driveKnob, feedbackKnob;
    juce::Label  ratioLab, indexLab, driveLab, feedbackLab;

    // Sub
    juce::Slider subLevelKnob, subOctaveKnob;
    juce::Label  subLevelLab, subOctaveLab;

    // Filter — large teal cutoff
    juce::Slider cutoffKnob, resoKnob, noiseKnob;
    juce::Label  cutoffLab, resoLab, noiseLab;

    // ADSR
    juce::Slider attackFader, decayFader, sustainFader, releaseFader;
    juce::Label  attackLab, decayLab, sustainLab, releaseLab;

    // LFO
    juce::Slider lfoRateKnob, lfoCutKnob, lfoIdxKnob, lfoDrvKnob, lfoAmpKnob;
    juce::Label  lfoRateLab, lfoCutLab, lfoIdxLab, lfoDrvLab, lfoAmpLab;
    juce::Label  lfoSyncLab, lfoGridLab;
    juce::ToggleButton lfoSyncBtn { "Sync" };
    juce::ToggleButton lfoAmpBtn  { "Amp" };
    juce::ComboBox lfoGridBox;

    // FX — teal knobs like reference
    juce::Slider fxDist, fxBits, fxRate, fxDly, fxDlyFb, fxDlyMix, fxCho, fxChoDep;
    juce::Label  fxDistLab, fxBitsLab, fxRateLab, fxDlyLab, fxDlyFbLab, fxDlyMixLab, fxChoLab, fxChoDepLab;

    // Shaper
    juce::ToggleButton imVolOn, imDriveOn, imTimeOn, imFiltOn, imCrushOn, imNoiseOn, imWidthOn;
    juce::Slider imVol, imDrive, imTime, imFilt, imCrush, imNoise, imWidth;
    juce::Label  imVolLab, imDriveLab, imTimeLab, imFiltLab, imCrushLab, imNoiseLab, imWidthLab;

    juce::Label logoLabel, tagLabel, presetLabel;
    juce::Label oscTitle, subTitle, filterTitle, envTitle, lfoTitle, fxTitle, shaperTitle, vizTitle;
    juce::Label oscBadge, subBadge, filterBadge, envBadge, lfoBadge, fxBadge, shaperBadge, vizBadge;

    juce::TextButton btnSave, btnLoad, btnUndo, btnRedo;
    juce::TextButton btnRandom, btnRandomFx, btnFx, btnShaper, btnRandomIm;

    bool fxOn = false;
    bool imOn = false;

    juce::Rectangle<int> topBar, bottomBar;
    juce::Rectangle<int> oscPanel, subPanel, filterPanel, envPanel, lfoPanel;
    juce::Rectangle<int> vizPanel, fxPanel, shaperPanel;
    juce::Rectangle<float> waveArea, spectrumArea, envShapeArea, filterCurveArea;

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

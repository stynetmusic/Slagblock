#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include "PluginProcessor.h"
#include <vector>

/** Soft-synth LookAndFeel: cyan accents, smooth thumbs, subtle hover. */
class SlagLookAndFeel : public juce::LookAndFeel_V4
{
public:
    SlagLookAndFeel();

    void drawLinearSlider (juce::Graphics&, int x, int y, int width, int height,
                           float sliderPos, float minSliderPos, float maxSliderPos,
                           juce::Slider::SliderStyle, juce::Slider&) override;

    void drawRotarySlider (juce::Graphics&, int x, int y, int width, int height,
                           float sliderPosProportional, float rotaryStartAngle,
                           float rotaryEndAngle, juce::Slider&) override;

    void drawButtonBackground (juce::Graphics&, juce::Button&,
                               const juce::Colour& backgroundColour,
                               bool shouldDrawButtonAsHighlighted,
                               bool shouldDrawButtonAsDown) override;

    juce::Font getTextButtonFont (juce::TextButton&, int buttonHeight) override;
    juce::Font getLabelFont (juce::Label&) override;
};

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

    void setupHSlider (juce::Slider& s, juce::Label& lab, const juce::String& name);
    void setupKnob (juce::Slider& s, juce::Label& lab, const juce::String& name);
    void setupToggle (juce::ToggleButton& b, const juce::String& text);
    void setupButton (juce::TextButton& b, const juce::String& text, bool toggle = false);
    void setupHeader (juce::Label& lab, const juce::String& text);

    void setFxPanelVisible (bool show);
    void setImitationPanelVisible (bool show);
    void updateWindowSize();
    void syncPanelsFromProcessor();
    void setFxControlsVisible (bool v);
    void setImControlsVisible (bool v);

    void layoutHList (juce::Rectangle<int> area,
                      juce::Slider* const* sliders,
                      juce::Label* const* labels,
                      int count);
    void layoutKnobRow (juce::Rectangle<int> area,
                        juce::Slider* const* knobs,
                        juce::Label* const* labels,
                        int count);
    void layoutImitation (juce::Rectangle<int> area);
    void layoutFxGrid (juce::Rectangle<int> area);

    void drawPanel (juce::Graphics& g, juce::Rectangle<int> r);
    void drawWaveform (juce::Graphics& g, juce::Rectangle<float> r);
    void drawAdsrShape (juce::Graphics& g, juce::Rectangle<float> r);

    float readParam (const char* id, float fallback) const;

    void savePresetClicked();
    void loadPresetClicked();

    SlagLookAndFeel slagLaf;
    SlagblockAudioProcessor& audioProcessor;

    // Osc / FM
    juce::Slider ratioSlider, indexSlider, feedbackSlider, driveSlider;
    juce::Label  ratioLabel, indexLabel, feedbackLabel, driveLabel;

    // Sub
    juce::Slider subLevelSlider, subOctaveSlider;
    juce::Label  subLevelLabel, subOctaveLabel;

    // Envelope
    juce::Slider attackSlider, decaySlider, sustainSlider, releaseSlider;
    juce::Label  attackLabel, decayLabel, sustainLabel, releaseLabel;

    // Filter
    juce::Slider cutoffSlider, resonanceSlider, noiseSlider;
    juce::Label  cutoffLabel, resonanceLabel, noiseLabel;

    // LFO
    juce::Slider lfoRateSlider, lfoCutoffSlider, lfoIndexSlider, lfoDriveSlider, lfoAmpDepthSlider;
    juce::Label  lfoRateLabel, lfoCutoffLabel, lfoIndexLabel, lfoDriveLabel, lfoAmpDepthLabel;
    juce::Label  lfoSyncLabel, lfoDivLabel;
    juce::ToggleButton lfoSyncToggle { "Sync" };
    juce::ToggleButton lfoToAmpToggle { "→ Amp" };
    juce::ComboBox lfoDivisionBox;

    // Bottom primary knobs (same params — convenient large controls)
    juce::Slider knobCutoff, knobReso, knobDrive, knobIndex, knobFeedback, knobSub;
    juce::Label  knobCutoffLab, knobResoLab, knobDriveLab, knobIndexLab, knobFeedbackLab, knobSubLab;

    // FX
    juce::Slider fxDistSlider, fxBitsSlider, fxRateSlider;
    juce::Slider fxDlyTimeSlider, fxDlyFbSlider, fxDlyMixSlider;
    juce::Slider fxChoMixSlider, fxChoDepthSlider;
    juce::Label  fxDistLabel, fxBitsLabel, fxRateLabel;
    juce::Label  fxDlyTimeLabel, fxDlyFbLabel, fxDlyMixLabel;
    juce::Label  fxChoMixLabel, fxChoDepthLabel;

    // Imitation
    juce::ToggleButton imVolOn, imDriveOn, imTimeOn, imFiltOn, imCrushOn, imNoiseOn, imWidthOn;
    juce::Slider imVolSlider, imDriveSlider, imTimeSlider, imFiltSlider;
    juce::Slider imCrushSlider, imNoiseSlider, imWidthSlider;
    juce::Label  imVolLabel, imDriveLabel, imTimeLabel, imFiltLabel;
    juce::Label  imCrushLabel, imNoiseLabel, imWidthLabel;

    juce::Label titleLabel, tagLabel, statusLabel;
    juce::Label oscHeader, subHeader, envHeader, filterHeader, lfoHeader;
    juce::Label fxHeader, imHeader, knobsHeader;

    juce::TextButton randomButton, fxButton, randomFxButton;
    juce::TextButton imitationButton, randomImButton;
    juce::TextButton savePresetButton, loadPresetButton, undoButton, redoButton;

    bool fxPanelVisible = false;
    bool imPanelVisible = false;

    juce::Rectangle<int> leftCol, centerCol, rightCol;
    juce::Rectangle<int> scopeArea, adsrArea;
    juce::Rectangle<int> knobsSec, fxSec, imSec;
    juce::Rectangle<int> headerBar;

    float phaseAnim = 0.0f;

    std::unique_ptr<juce::FileChooser> fileChooser;

    using SA = juce::AudioProcessorValueTreeState::SliderAttachment;
    using BA = juce::AudioProcessorValueTreeState::ButtonAttachment;

    std::unique_ptr<SA> ratioAtt, indexAtt, driveAtt, feedbackAtt;
    std::unique_ptr<SA> cutoffAtt, resonanceAtt, noiseAtt;
    std::unique_ptr<SA> attackAtt, decayAtt, sustainAtt, releaseAtt;
    std::unique_ptr<SA> subLevelAtt, subOctaveAtt;
    std::unique_ptr<SA> lfoRateAtt, lfoCutoffAtt, lfoIndexAtt, lfoDriveAtt, lfoAmpDepthAtt;
    std::unique_ptr<BA> lfoToAmpAtt, lfoSyncAtt;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> lfoDivisionAtt;

    void updateLfoRateEnabled();
    std::unique_ptr<SA> knobCutoffAtt, knobResoAtt, knoDriveAtt, knobIndexAtt, knobFdbkAtt, knobSubAtt;
    std::unique_ptr<SA> fxDistAtt, fxBitsAtt, fxRateAtt;
    std::unique_ptr<SA> fxDlyTimeAtt, fxDlyFbAtt, fxDlyMixAtt, fxChoMixAtt, fxChoDepthAtt;
    std::unique_ptr<SA> imVolAtt, imDriveAtt, imTimeAtt, imFiltAtt, imCrushAtt, imNoiseAtt, imWidthAtt;
    std::unique_ptr<BA> imVolOnAtt, imDriveOnAtt, imTimeOnAtt, imFiltOnAtt;
    std::unique_ptr<BA> imCrushOnAtt, imNoiseOnAtt, imWidthOnAtt;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SlagblockAudioProcessorEditor)
};

//
// Created by Damien Ronssin on 14.03.23.
//

#include "MinMaxNoteSlider.h"

MinMaxNoteSlider::MinMaxNoteSlider(std::atomic<int>& inAttachedMinValue,
                                   std::atomic<int>& inAttachedMaxValue)
    : mAttachedMinValue(inAttachedMinValue)
    , mAttachedMaxValue(inAttachedMaxValue)
{
    mSlider.setSliderStyle(juce::Slider::TwoValueHorizontal);
    mSlider.setRange(MIN_MIDI_NOTE, MAX_MIDI_NOTE, 1);
    mSlider.setTextBoxStyle(juce::Slider::TextEntryBoxPosition::NoTextBox, true, 0, 0);
    mSlider.onValueChange = [this]()
    {
        mAttachedMinValue.store(int(mSlider.getMinValue()));
        mAttachedMaxValue.store(int(mSlider.getMaxValue()));
        repaint();
    };

    mSlider.setMinAndMaxValues(mAttachedMinValue.load(), mAttachedMaxValue.load());

    addAndMakeVisible(mSlider);
}

void MinMaxNoteSlider::resized()
{
    mSlider.setBounds(26, 0, 102, 12);
}

void MinMaxNoteSlider::paint(Graphics& g)
{
    g.setColour(juce::Colours::black);
    g.setFont(DROPDOWN_FONT);

    g.drawText(NoteUtils::midiNoteToStr(int(mSlider.getMinValue())),
               Rectangle<int>(0, 0, 22, 12),
               juce::Justification::centredLeft);

    g.drawText(NoteUtils::midiNoteToStr(int(mSlider.getMaxValue())),
               Rectangle<int>(132, 0, 22, 12),
               juce::Justification::centredRight);
}

void MinMaxNoteSlider::addListener(juce::Slider::Listener* inListener)
{
    mSlider.addListener(inListener);
}

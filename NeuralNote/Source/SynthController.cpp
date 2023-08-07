//
// Created by Damien Ronssin on 10.06.23.
//

#include "SynthController.h"
#include "PluginProcessor.h"

SynthController::SynthController(NeuralNoteAudioProcessor* inProcessor, MPESynthesiser* inMPESynth)
    : mProcessor(inProcessor)
    , mSynth(inMPESynth)
{
    // Set midi buffer size to 200 elements to avoid allocating memory on audio thread.
    mMidiBuffer.ensureSize(3 * 200);
}

std::vector<MidiMessage> SynthController::buildMidiEventsVector(const std::vector<Notes::Event>& inNoteEvents)
{
    int pitch_bend_channel = 2;

    // Compute size of single event vector
    size_t num_midi_messages = 0;

    for (const auto& note_event: inNoteEvents) {
        num_midi_messages += 2 + note_event.bends.size();
    }

    std::vector<MidiMessage> out(num_midi_messages);

    size_t i = 0;

    for (const auto& note_event: inNoteEvents) {
        bool has_pitch_bends = !note_event.bends.empty();

        int channel = has_pitch_bends ? pitch_bend_channel : 1;

        if (has_pitch_bends)
            pitch_bend_channel = pitch_bend_channel < 16 ? pitch_bend_channel + 1 : 2;

        out[i++] = MidiMessage::noteOn(channel, note_event.pitch, (float) note_event.amplitude)
                       .withTimeStamp(note_event.startTime);

        for (size_t j = 0; j < note_event.bends.size(); j++) {
            out[i++] =
                MidiMessage::pitchWheel(
                    channel, MidiMessage::pitchbendToPitchwheelPos(static_cast<float>(note_event.bends[j]) / 3.0f, 2))
                    .withTimeStamp(note_event.startTime + (double) j * 256.0 / BASIC_PITCH_SAMPLE_RATE);
        }
        
        out[i++] = MidiMessage::noteOff(channel, note_event.pitch).withTimeStamp(note_event.endTime);

        jassert(i <= num_midi_messages);
    }

    std::sort(out.begin(), out.end(), [](const MidiMessage& a, const MidiMessage& b) {
        return a.getTimeStamp() < b.getTimeStamp();
    });

    return out;
}

void SynthController::setNewMidiEventsVectorToUse(std::vector<MidiMessage>& inEvents)
{
    const ScopedLock sl(mProcessor->getCallbackLock());
    std::swap(inEvents, mEvents);
    _updateCurrentEventIndex();
    _sanitizeVoices();
}

void SynthController::setSampleRate(double inSampleRate)
{
    mSampleRate = inSampleRate;
}

const MidiBuffer& SynthController::generateNextMidiBuffer(int inNumSamples)
{
    mMidiBuffer.clear();

    double end_time = mCurrentTime + inNumSamples / mSampleRate;

    while (mCurrentEventIndex < mEvents.size() && mEvents[mCurrentEventIndex].getTimeStamp() < end_time) {
        int index = std::clamp(static_cast<int>(std::round(mEvents[mCurrentEventIndex].getTimeStamp() - mCurrentTime)),
                               0,
                               inNumSamples - 1);

        mMidiBuffer.addEvent(mEvents[mCurrentEventIndex], index);
        mCurrentEventIndex += 1;
    }

    mCurrentTime = end_time;
    mCurrentSampleIndex += inNumSamples;

    if (mCurrentTime >= mProcessor->getSourceAudioManager()->getAudioSampleDuration()) {
        // Stop playing and reset to start
        mProcessor->getPlayer()->setPlayingState(false);
        setNewTimeSeconds(0);
    }

    return mMidiBuffer;
}

void SynthController::reset()
{
    mCurrentEventIndex = 0;
    mCurrentSampleIndex = 0;
    mCurrentTime = 0.0;
    mMidiBuffer.clear();
}

void SynthController::setNewTimeSeconds(double inNewTime)
{
    jassert(inNewTime >= 0);
    mCurrentTime = inNewTime;
    mCurrentSampleIndex = static_cast<int>(std::round(inNewTime * mSampleRate));
    _updateCurrentEventIndex();
    _sanitizeVoices();
}

double SynthController::getCurrentTimeSeconds() const
{
    return mCurrentTime;
}

void SynthController::_sanitizeVoices()
{
    // Insert note off event to avoid hanging notes
    for (int i = 0; i < mSynth->getNumVoices(); i++) {
        auto* voice = dynamic_cast<SynthVoice*>(mSynth->getVoice(i));
        if (voice->isActive()) {
            auto midi_note = voice->getCurrentMidiNote();

            if (!_isNextOnOffEventNoteOff(midi_note)) {
                voice->noteStopped(true);
            }
        }
    }
}

void SynthController::_updateCurrentEventIndex()
{
    mCurrentEventIndex = std::lower_bound(mEvents.begin(),
                                          mEvents.end(),
                                          mCurrentTime,
                                          [](const MidiMessage& a, double b) { return (a.getTimeStamp() < b); })
                         - mEvents.begin();
}

bool SynthController::_isNextOnOffEventNoteOff(int inMidiNote)
{
    auto iter = std::find_if(
        mEvents.begin() + static_cast<long>(mCurrentEventIndex), mEvents.end(), [inMidiNote](const MidiMessage& a) {
            return !a.isPitchWheel() && a.getNoteNumber() == inMidiNote;
        });

    if (iter == mEvents.end()) {
        return false;
    }

    return (*iter).isNoteOff();
}

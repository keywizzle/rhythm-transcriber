#include <string>

#include "Rhythm.h"

namespace RhythmTranscriber
{
    bool BaseRhythm::operator<(RhythmTranscriber::BaseRhythm &rhythm)
    {
        return complexity < rhythm.complexity;
    }
    bool BaseRhythm::operator>(RhythmTranscriber::BaseRhythm &rhythm)
    {
        return complexity > rhythm.complexity;
    }
    NoteRhythm::NoteRhythm(unsigned char notes, unsigned char beats, unsigned int complexity,
                           float bpm)
    {
        this->notes = notes;
        this->beats = beats;
        this->complexity = complexity;
        this->bpm = bpm;
    }
    NoteRhythm::NoteRhythm()
    {
        notes = 0;
        beats = 0;
        complexity = 0;
        bpm = 0;
    }
    std::string NoteRhythm::str(bool compact)
    {
        std::string str = "";
        if (compact)
        {
            return std::to_string(notes) + '/' + std::to_string(beats) +
                   ", complexity: " + std::to_string(complexity) + ", bpm: " + std::to_string(bpm) +
                   '\n';
        }

        str += "\n- Notes: " + std::to_string(notes);
        str += "\n- Beats: " + std::to_string(beats);
        str += "\n- Complexity: " + std::to_string(complexity);
        str += "\n- BPM: " + std::to_string(bpm) + '\n';

        return str;
    }
}
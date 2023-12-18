#pragma once

#include <vector>

#include "BeatAnalyzer.h"
#include "Config.h"
#include "Note.h"
#include "transcription/Transcription.h"

namespace RhythmTranscriber
{
    class Transcriber
    {
    public:
        Transcriber(float *timestamps, unsigned int length);

        std::vector<BaseNote> notes;

        RhythmTranscriber::Transcription::Transcription transcription;

        void transcribe();
        void transcribe(float bpm);
        void transcribe(float bpm, unsigned int depth);
        void transcribe(float *timestamps, unsigned int length);

    private:
        BeatAnalyzer beatAnalyzer;

        void create_notes(float *timestamps, unsigned int length);

        inline void add_beat(const Beat &beat)
        {
            for (unsigned int j = 0; j < beat.notesLen; j++)
            {
                Transcription::NoteElement note;

                note.timestamp = beat.notes[j].timestamp;

                note.duration = beat.notes[j].duration;

                /// @todo Simplify note ratio before adding to transcription.

                note.rhythm =
                    Transcription::NoteRhythm{.notes = (unsigned short)beat.division.consequent,
                                              .beats = (unsigned short)beat.noteValues[j]};

                note.placement = Transcription::NotePlacement::PLACEMENT_HEAD;

                transcription.notes.push_back(note);
            }
        }
    };
}

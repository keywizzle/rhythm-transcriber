#pragma once

#include "Note.h"

#include <string>
#include <vector>

typedef unsigned char BeatDivisionValue;

namespace RhythmTranscriber
{
    struct BeatDivision
    {
        BeatDivisionValue numerator = 0;
        BeatDivisionValue denominator = 0;

        float duration = 0;

        operator bool() const;
    };

    class Beat
    {
    public:
        /// @brief Timestamp of the start of the beat.
        float timestamp;

        /// @brief Duration of the beat (TODO: Isn't this the same as `division.duration`?).
        float duration;

        /// @brief If set to nonzero, the beat will add notes that have a division denominator as a
        /// factor of `forceDivision`. If a note is added whose estimated rhythm does not follow
        /// this criteria, the note's rhythm interpretation will be set to the next matching rhythm
        /// that does follow the criteria.
        BeatDivisionValue forceDivision = 0;

        /// @brief Division of the beat. Basically, how much of the beat is occupied by notes. For
        /// example, a division of 1/4 means 1/4th of the beat (equivalent to one sixteenth note)
        /// from the start of the beat is taken.
        BeatDivision division;

        /// @brief Offest at the beginning of the beat until the first note. This is a
        BeatDivision offset;

        /// @brief Beat division of a note that extends outside the end of the beat. This can be
        /// used to define the next note's `offset`.
        BeatDivision tail;

        /// TODO: Maybe we change this to a stack array (std::array) with a size of like 16 (or
        /// maybe defined in config) if performance seems to be an issue.
        std::vector<NoteInterpretation> notes;
        /* NoteInterpretation notes[16]; */ // On my momma this shit ran 10x faster with stack
                                            // instead of heap allocs

        Beat(float timestamp);
        Beat(float timestamp, BeatDivision offset);
        Beat(float timestamp, BeatDivisionValue forceDivision);
        Beat(float timestamp, BeatDivision offset, BeatDivisionValue forceDivision);

        /* void setOffset(BeatDivision offset);
        void setForceDivision(BeatDivisionValue division); */

        void addNoteInterpretation(NoteInterpretation &note);

        void addNote(float timestamp, float duration, NoteRhythm rhythm);

        std::string str(bool compact = false);
    };
}
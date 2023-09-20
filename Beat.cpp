#include "Beat.h"

#include <numeric>

namespace RhythmTranscriber
{
    Beat::Beat(float timestamp) { this->timestamp = timestamp; }
    Beat::Beat(float timestamp, BeatDivision offset) : Beat::Beat(timestamp)
    {
        /// Since forceDivision wasn't passed, we don't have to worry about it existing already
        this->offset = offset;
        this->division = offset;
        this->duration = offset.duration;
    }
    Beat::Beat(float timestamp, BeatDivisionValue forceDivision) : Beat::Beat(timestamp)
    {
        /// Since offset wasn't passed, we don't have to worry about it existing already
        this->forceDivision = forceDivision;
    }
    Beat::Beat(float timestamp, BeatDivision offset, BeatDivisionValue forceDivision)
        : Beat::Beat(timestamp, offset)
    {
        /// Offset has priority over forceDivision, which is why the above constructor is delegated.

        this->forceDivision = forceDivision;

        /// Check if attempting to create a beat with difference forced division and offset division
        if (forceDivision != offset.denominator)
        {
            auto denomLCM = std::lcm(offset.denominator, forceDivision);
            this->division.denominator = denomLCM;
            this->division.numerator = offset.numerator * (denomLCM / offset.denominator);
        }
    }

    /* void Beat::setOffset(BeatDivision offset) {}
    void Beat::setForceDivision(BeatDivisionValue division) {} */

    void Beat::addNoteInterpretation(NoteInterpretation &note)
    {
        /* auto noteDuration = (60 * note.rhythm.beats) / (note.rhythm.bpm * note.rhythm.notes); */
        auto noteDuration = note.rhythm.get_duration();

        /// Division of the note itself (as a BeatDivision), beats becomes numerator and notes
        /// becomes denominator, since notes/rhythms are represented as notes/beats and beat
        /// divisions are represented as fractions of space in the beat.
        /// Ex: 4/1 (rhythm) => 4 notes / 1 beat => sixteenth note, which takes up 1/4th of a beat.
        BeatDivision noteDivision =
            BeatDivision{note.rhythm.beats, note.rhythm.notes, noteDuration};

        if (division.denominator == 0)
        {
            division = noteDivision;
        }
        else
        {
            auto denomLCM = std::lcm(division.denominator, noteDivision.denominator);

            auto numerator1 = division.numerator * (denomLCM / division.denominator);
            auto numerator2 = noteDivision.numerator * (denomLCM / noteDivision.denominator);

            division.numerator = numerator1 + numerator2;
            division.denominator = denomLCM;
            division.duration += noteDivision.duration;
        }

        notes.push_back(note);

        /// Check if we're adding a note that extends past the note's duration
        if (division.numerator > division.denominator)
        {
            /// unsigned char - unsigned char results in an int output, and since numerator >
            /// denominator we will guaranteed have enough space anyways.
            tail = BeatDivision{(BeatDivisionValue)(division.numerator - division.denominator),
                                division.denominator, note.rhythm.bpm};
        }

        /// Only add to duration the part of the note that would be inside this beat
        duration += noteDuration - (tail.denominator == 0 ? 0 : tail.duration);

        /// Stats
        /// TODO
    }

    /// @brief TESTING PURPOSES
    /// @param timestamp
    /// @param duration
    /// @param rhythm
    void Beat::addNote(float timestamp, float duration, NoteRhythm rhythm)
    {
        /// Fix rhythm BPM
        rhythm.set_duration(duration);

        /// Division of the note itself (as a BeatDivision), beats becomes numerator and notes
        /// becomes denominator, since notes/rhythms are represented as notes/beats and beat
        /// divisions are represented as fractions of space in the beat.
        /// Ex: 4/1 (rhythm) => 4 notes / 1 beat => sixteenth note, which takes up 1/4th of a beat.
        BeatDivision noteDivision = BeatDivision{rhythm.beats, rhythm.notes, duration};

        if (division.denominator == 0)
        {
            division = noteDivision;
        }
        else
        {
            auto denomLCM = std::lcm(division.denominator, noteDivision.denominator);

            auto numerator1 = division.numerator * (denomLCM / division.denominator);
            auto numerator2 = noteDivision.numerator * (denomLCM / noteDivision.denominator);

            division.numerator = numerator1 + numerator2;
            division.denominator = denomLCM;
            division.duration += noteDivision.duration;
        }

        /// Check if we're adding a note that extends past the note's duration
        if (division.numerator > division.denominator)
        {
            /// unsigned char - unsigned char results in an int output, and since numerator >
            /// denominator we will guaranteed have enough space anyways.
            tail = BeatDivision{(BeatDivisionValue)(division.numerator - division.denominator),
                                division.denominator, rhythm.bpm};
        }

        /// Only add to duration the part of the note that would be inside this beat
        this->duration += duration - (tail.denominator == 0 ? 0 : tail.duration);

        /// Stats
        /// TODO
    }

    /// BeatDivision

    BeatDivision::operator bool() const
    {
        return (bool)numerator;
        /// A better check would go as follows:
        /// return (bool)numerator || (bool)denominator || (bool)duration
        /// However, this would probably slow down every time the division exists it has to check
        /// all three instead of just one
    }

    std::string Beat::str(bool compact)
    {
        std::string str = "\n";
        if (compact)
        {
            str += "timestamp: " + std::to_string(timestamp) +
                   ", duration: " + std::to_string(duration) +
                   ", division: " + std::to_string(division.numerator) + "/" +
                   std::to_string(division.denominator) +
                   ", offset: " + std::to_string(offset.numerator) + "/" +
                   std::to_string(offset.denominator) +
                   " duration: " + std::to_string(offset.duration) +
                   ", forceDivision: " + std::to_string(forceDivision) + '\n';
        }
        else
        {
            str += "- Timestamp: " + std::to_string(timestamp) +
                   "\n- Duration: " + std::to_string(duration) +
                   "\n- Division: " + std::to_string(division.numerator) + "/" +
                   std::to_string(division.denominator) + ", with a duration of " +
                   std::to_string(division.duration) + '\n';
            if (offset.denominator != 0)
            {
                str += "- Offset: " + std::to_string(offset.numerator) + "/" +
                       std::to_string(offset.denominator) + ", with a duration of " +
                       std::to_string(offset.duration) + '\n';
            }
            if (forceDivision)
            {
                str += "- forceDivision: " + std::to_string(forceDivision) + '\n';
            }
            auto notesSize = notes.size();
            for (unsigned int i = 0; i < notesSize; i++)
            {
                auto note = notes[i];
                str += "\t- Note at " + std::to_string(note.note->timestamp) +
                       " rhythm: " + std::to_string(note.rhythm.notes) + "/" +
                       std::to_string(note.rhythm.beats) + " at " +
                       std::to_string(note.rhythm.bpm) + " BPM\n";
            }
        }

        return str;
    }
}
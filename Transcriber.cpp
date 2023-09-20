#include "Transcriber.h"
#include "Beat.h"
#include "Note.h"
#include "Rhythm.h"

#include "utils.h"

#include <algorithm>
#include <iostream>
#include <math.h>
#include <unordered_set>
#include <vector>

inline float divisionValue_(float num1, float num2)
{
    /// Measures how well the nums divide each other
    /// Can we make this so regardless of which is num1/num2, the output is the same?

    // num1 = 0.5
    // num2 = 1
    // output = perfect match

    // num1 = 0.49
    // num2 = 1
    // output = almost perfect match

    float div = num1 > num2 ? (num1 / num2) : (num2 / num1);
    float roundDiv = std::round(div);

    std::cout << "roundDiv: " << std::to_string(roundDiv) << " (div: " << std::to_string(div) << ')'
              << '\n';

    /// Scale value
    return 1 - (std::abs(div - roundDiv) * 2);

    // return std::abs(div - roundDiv);
}

namespace RhythmTranscriber
{
    void Transcriber::init_rhythms()
    {
        std::unordered_set<float> complexities;

        unsigned int index = 0;
        for (unsigned char beats = 1; beats <= RhythmTranscriber::maxBeats; beats++)
        {
            for (unsigned char notes = 1; notes <= RhythmTranscriber::maxNotes; notes++)
            {
                // std::cout << std::to_string(notes) << '/' << std::to_string(beats) << '\n';
                if (complexities.emplace((float)notes / (float)beats).second)
                {
                    baseRhythms[index] = BaseRhythm{notes, beats, (unsigned int)(notes * beats)};
                    index++;
                }
            }
        }

        // Sort by lowest->highest complexity
        std::sort(&(baseRhythms[0]), &(baseRhythms[index - 1]));
    }

    /// @brief Gives each note their rhythms with BPM for each rhythm. Nothing much.
    void Transcriber::create_notes(float *timestamps, unsigned int length)
    {
        notes.clear(); // Clear if we previously had notes

        notes.reserve(length);

        for (unsigned int i = 0; i < length - 1; i++)
        {
            auto note = Note{*(timestamps + i), *(timestamps + i + 1) - *(timestamps + i)};

            for (unsigned int j = 0; j < rhythmCount; j++)
            {
                auto rhythm = baseRhythms[j];
                note.rhythms[j] = NoteRhythm(rhythm.notes, rhythm.beats, rhythm.complexity,
                                             (60 * rhythm.beats) / (note.duration * rhythm.notes));
            }

            notes.push_back(note);
        }

        // Add last note with -1 duration
        notes.push_back(Note{*(timestamps + length - 1), -1.f});
    }

    /*
        Labels each note's duration ratio with the note before/after it (if applicable)
    */
    void Transcriber::label_neighbor_ratios()
    {
        // For right now, duration ratios will be defined as: current note duration / neighbor note
        // duration So if we have note1 and note2 (note2 comes after note1), with note1: 0.5, note2:
        // 1 => note1 neighbor ratio to note2: 0.5 note2 neighbor ratio to note1: 2

        // Maybe we could have the numerator always be the note that comes first, might be more
        // consistent + performant but less intuitive

        auto notesSize = notes.size();

        if (notesSize < 2)
            return; // Only one note, it has no neighbors so we do nothing

        // Label first note
        notes.at(0).neighbor_duration_ratios[1] =
            notes.at(0).duration /
            notes.at(1).duration; // There are at least two notes, so this statement is safe

        for (unsigned int i = 1; i < notesSize - 1; i++)
        {
            auto duration = notes.at(i).duration;
            notes.at(i).neighbor_duration_ratios[0] = duration / notes.at(i - 1).duration;
            notes.at(i).neighbor_duration_ratios[1] = duration / notes.at(i + 1).duration;
        }

        // Label last note
        notes.at(notesSize - 1).neighbor_duration_ratios[0] =
            notes.at(notesSize - 2).neighbor_duration_ratios[1];
    }

    unsigned int Transcriber::get_note_string(unsigned int noteIndex, float bpm)
    {
        const float quarterNoteLen = 60 / bpm;

        const auto notesSize = notes.size();

        unsigned int stringLen = 0;

        float startTime = notes.at(noteIndex).timestamp;
        float prevTimestamp = startTime;

        for (unsigned int i = noteIndex; i < notesSize - 1; i++)
        {
            auto timestamp = notes.at(i).timestamp;
            if (timestamp - startTime > quarterNoteLen)
            {
                // Can be optimized by just returning stringLen + (math expression converted to
                // bool) (I think)
                if ((timestamp - startTime) - quarterNoteLen <
                    quarterNoteLen - (prevTimestamp - startTime))
                {
                    return stringLen; // Only include notes within the beat
                }
                return stringLen - 1; // Only include notes within the beat
            }

            prevTimestamp = timestamp;
            stringLen++;
        }

        return stringLen;
    }

    UniformNoteString Transcriber::get_uniform_note_str_at(unsigned int index)
    {
        UniformNoteString noteStr;
        noteStr.notes = &(notes[index]);
        noteStr.length = 1;

        auto notesLen = notes.size();
        for (unsigned int i = index + 1; i < notesLen; i++)
        {
            auto note = notes[i];

            auto val = noteStr.check_note(note);

            if (val > 0.6f)
            {
                /// Note is very likely the same division/duration
                noteStr.increaseRight();
                continue;
            }
            if (val < 0.5f)
            {
                return noteStr;
            }

            /// This is the grey area. This will occur in situations where a note is unsure to
            /// be either the same or different division/duration. To check, we can recursively
            /// get the next note string as if this one ended right now.

            /// Check to see if the note string starting at this note fits well
            UniformNoteString nextStr = get_uniform_note_str_at(i);

            /// Compare strings

            auto compareVal = noteStr.compare(nextStr);
            std::cout << "compare val: " << compareVal << '\n';
        }

        return noteStr;
    }

    std::vector<UniformNoteString> Transcriber::get_uniform_note_strs()
    {
        std::vector<UniformNoteString> noteStrs;

        unsigned int start = 100;

        /* auto notesLen = notes.size(); */
        auto notesLen = start + 12;

        UniformNoteString noteStr;
        noteStr.notes = &(notes[start]);
        noteStr.length = 1;

        /// Ignore last note because it has -1 duration (it will get a note string of its own)
        for (unsigned int i = start + 1; i < notesLen - 1; i++)
        {
            /* auto nextNoteStr = get_uniform_note_str_at(i); */
            noteStr.notes = &notes[i];
            noteStr.length = 1;

            noteStr.search(notesLen - 1 - i, 0);

            noteStrs.push_back(noteStr);

            i += noteStr.length;

            /* std::cout << "i = " << std::to_string(i) << '\n';
            auto note = notes[i];
            auto val = noteStr.checkNote(note);
            std::cout << "(timestamp: " << std::to_string(note.timestamp)
                      << ") val: " << std::to_string(val) << '\n';

            if (val < 0.5f)
            {
                /// In case of outliers, we should see if any more notes don't fit the division
                /// Maybe we could have it depend on the length of the note string as well, such
                /// that shorter length is more lenient (but only by a little)
                noteStrs.push_back(noteStr);

                noteStr.notes = &(notes[i]);
                noteStr.length = 1;
                continue;
            }

            noteStr.increaseRight(); */
        }

        /* /// Add recent note string
        noteStrs.push_back(noteStr); */

        /// Add last note as own note string
        UniformNoteString lastNoteString;
        lastNoteString.notes = &(notes[notesLen - 1]);
        lastNoteString.length = 1;
        noteStrs.push_back(lastNoteString);

        return noteStrs;
    }

    void Transcriber::transcribe(float *timestamps, unsigned int length, float bpm)
    {
        initialBpm = bpm;

        if (!loadedRhythms)
        {
            init_rhythms();
            loadedRhythms = true;
        }

        create_notes(timestamps, length);

        //////////////////////////////////////////////////////////////////////////////////////////////////////////////

        /// - Iterate through notes, adding them to a note string
        /// - Stop the note string when there are at least two consecutive sequences of division
        ///   values that are significantly different from each other
        ///     - Method 1: Have threshold value to use with a new note, such that if the new note
        ///       has a bad division value compared to others in the string, stop the current string
        ///     - Method 2: Detect for string of bad division values
        ///         - ex: [0.81, 0.8, 0.78, 0.8, 0.49, 0.5, 0.46]

        // auto noteStrs = get_uniform_note_strs();

        auto index = 95;

        auto noteStr = UniformNoteString();
        noteStr.notes = &notes[index];
        noteStr.length = 1;
        noteStr.search(notes.size() - 2 - index, 0);

        for (unsigned int i = 0; i < noteStr.length; i++)
        {
            std::cout << "note string has timestamp " << std::to_string(noteStr.notes[i].timestamp)
                      << '\n';
        }

        /* for (unsigned int i = 0; i < noteStrs.size(); i++)
        {
            auto noteStr = noteStrs[i];
            std::cout << "NoteString with " << std::to_string(noteStr.length) << ":\n";
            for (unsigned int j = 0; j < noteStr.length; j++)
            {
                std::cout << noteStr[j].str() << '\n';
            }
        } */
    }
}
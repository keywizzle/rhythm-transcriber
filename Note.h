#pragma once

#include "Config.h"
#include "Rhythm.h"
#include <vector>

namespace RhythmTranscriber
{
    struct Note
    {
        float timestamp;

        /// Duration of the note, usually the difference between the next note's timestamp and the
        /// current note's timestamp. If there is no next note, it will be set to `-1`.
        float duration;

        NoteRhythm rhythms[rhythmCount];

        float neighbor_duration_ratios[2] = {-1.f, -1.f};

        Note(float timestamp, float duration);

        NoteRhythm rhythms_for_bpm(float bpm);

        std::string str();
    };

    /// @brief Contains data about an interpretation of a specific note, mainly its rhythm.
    struct NoteInterpretation
    {
        /// @brief Reference to the note.
        Note *note;

        /// @brief Interpreted rhythm for the note.
        NoteRhythm rhythm;

        NoteInterpretation();
        NoteInterpretation(Note *note);

        void assign_bpm(float bpm);
    };

    class NoteString
    {
    public:
        Note *notes = nullptr;

        unsigned int length = 0;

        /// @brief Offset from the start of the note array. (TODO: May not need this, we can access
        /// nearby notes by dereferencing the notes pointer)
        unsigned int offset;

        NoteString();
        NoteString(Note *notes, unsigned int offset);
        NoteString(Note *notes, unsigned int offset, unsigned int length);
        NoteString(std::vector<Note> &notes);

        Note operator[](unsigned int index);
        NoteString operator=(const NoteString &noteStr);

        /// @brief Increase the note string by 1 note to the right
        /// @note Ex: `1 [2 3 4] 5` => `1 [2 3 4 5]`
        inline void increaseRight() { length++; }
        /// @brief Increase the note string by 1 note to the left
        /// @note Ex: `1 [2 3 4] 5` => `[1 2 3 4] 5`
        inline void increaseLeft()
        {
            notes--;
            length++;
        }
        /// @brief Decrease the note string by 1 note to the right
        /// @note Ex: `1 [2 3 4] 5` => `1 [2 3] 4 5`
        inline void decreaseRight() { length--; }
        /// @brief Decrease the note string by 1 note to the left
        /// @note Ex: `1 [2 3 4] 5` => `1 2 [3 4] 5`
        inline void decreaseLeft()
        {
            notes++;
            length--;
        }

        void calcStats();

        inline float getDurationAvg() { return durationAvg; }
        inline float getDurationSD() { return durationSD; }
        inline float getDivisionAvg() { return divisionAvg; }

        unsigned int findIndex(float timestamp);

        std::string str();

    private:
        /// @brief Sum of the durations
        float durationSum = 0;
        /// @brief Average duration
        float durationAvg = 0;
        /// @brief Standard deviation of the durations
        float durationSD = 0;

        /// Division stats are similar to duration stats, except if there is a note with a duration
        /// (approximately) as a multiple of another note's duration, they are assigned an
        /// approximately equal division.

        /// @brief Sum of the durations with rounded LCM applied
        float divisionSum = 0;
        /// @brief Average duration with rounded LCM applied
        float divisionAvg = 0;
        /// @brief Standard deviation of the durations with rounded LCM applied
        float divisionSD = 0;
    };

    /// @brief Represents a strict note string where every note should be approximately the same
    /// duration. This means diddles will not be counted.
    class UniformNoteString : public NoteString
    {
    public:
        UniformNoteString operator=(const UniformNoteString &noteStr);

        /// TODO Maybe make this inline
        /// @brief Checks how well a note would fit into this note string, based on duration.
        /// @param note
        /// @return Value between 0-1, 0 being poor fit and 1 being perfect fit
        float check_note(const Note &note);

        float get_score();

        /// @brief Compares the two note strings.
        /// @param noteStr Note string to compare with.
        /// @return
        float compare(const UniformNoteString &noteStr);

        /// @brief Searches for notes and increases length to include only notes that are judged to
        /// be the same division.
        /// @param maxLength
        void search(unsigned int maxLength, unsigned int recursiveDepth);
    };

    class NoteStringContainer
    {
        Note *notes = nullptr;

        unsigned int notesLen = 0;

        /// @brief Each element of the outer vector corresponds to a note, and each inner element
        /// corresponds to the length of a specific NoteString that starts on that note.
        /// TODO: We should probably just make it a vector of set-length arrays. We're not likely
        /// gonna have a ton of possible note strings for an individual note.
        std::vector<std::vector<unsigned int>> lengths;

        void init();

    private:
        inline float compare_uniform(float num1, float num2)
        {
            // TODO: Take into account duration, ie longer duration should look more at the difference instead of the 
            float div = num1 > num2 ? (num1 / num2) : (num2 / num1);

            if (div >= 2)
            {
                return 0;
            }

            return -div + 2;
        }
    };
}
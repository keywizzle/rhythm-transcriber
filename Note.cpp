#include "Note.h"
#include "Config.h"
#include "Transcriber.h"
#include <iostream>
#include <math.h>

namespace RhythmTranscriber
{
    /// Note
    Note::Note(float timestamp, float duration)
    {
        this->timestamp = timestamp;
        this->duration = duration;
    }

    NoteRhythm Note::rhythms_for_bpm(float bpm)
    {
        double minDist = 9999999;
        NoteRhythm *bestRhythm;

        for (unsigned int i = 0; i < rhythmCount; i++)
        {
            auto rhythm = rhythms[i];
            auto bpmDiff = bpm - rhythm.bpm;

            /* auto dist = sqrt(bpmDiff * bpmDiff + rhythm.complexity * rhythm.complexity); */
            auto dist = bpmDiff * bpmDiff + rhythm.complexity * rhythm.complexity;

            if (dist < minDist)
            {
                minDist = dist;
                bestRhythm = &(rhythms[i]);
            }
        }

        return *bestRhythm;
    }

    std::string Note::str()
    {
        return "Timestamp: " + std::to_string(timestamp) +
               ", Duration: " + std::to_string(duration);
    }

    /// NoteInterpretation
    NoteInterpretation::NoteInterpretation() {}
    NoteInterpretation::NoteInterpretation(Note *note) { this->note = note; }

    /// @brief Gets the best matching rhythm for the given BPM, based on note duration and
    /// complexity of the rhythm.
    /// @param bpm
    void NoteInterpretation::assign_bpm(float bpm)
    {
        double minDist = 9999999;

        auto rhythms = note->rhythms;
        unsigned int bestIndex = 0; // I guess default to first rhythm if somehow none are found

        for (unsigned int i = 0; i < rhythmCount; i++)
        {
            auto rhythm = rhythms[i];

            /* if (rhythm.bpm < 100 || rhythm.bpm > 300)
                continue; */

            auto bpmDiff = bpm - rhythm.bpm;

            /// Using square root may be more accurate in terms of magnitude, but since we only need
            /// to do comparisons, it's faster to just avoid the extra cycles for sqrt.
            /// TODO: Maybe we don't need the multiplication either? Might be worth to check out.
            // auto dist = sqrt(bpmDiff * bpmDiff + rhythm.complexity * rhythm.complexity);
            auto dist = bpmDiff * bpmDiff + rhythm.complexity * rhythm.complexity;

            if (dist < minDist)
            {
                minDist = dist;
                bestIndex = i;
            }
        }

        rhythm = rhythms[bestIndex];
    }

    /// NoteString

    NoteString::NoteString() {}
    NoteString::NoteString(Note *notes, unsigned int offset)
    {
        this->notes = notes;
        this->offset = offset;
    }
    NoteString::NoteString(Note *notes, unsigned int offset, unsigned int length)
    {
        this->notes = notes;
        this->offset = offset;
        this->length = length;
    }
    NoteString::NoteString(std::vector<Note> &notes)
    {
        this->notes = &(notes[0]);
        length = notes.size();

        // Calculate stats here maybe?
    }

    Note NoteString::operator[](unsigned int index) { return notes[index]; }

    NoteString NoteString::operator=(const NoteString &noteStr)
    {
        // TODO: I don't know if this is needed
        notes = noteStr.notes;
        length = noteStr.length;
        offset = noteStr.offset;

        return *this;
    }

    void NoteString::calcStats()
    {
        // I don't think there's a way to minimize this calculation and calculate average and
        // standard deviation at the same time

        /// Get average
        durationSum = 0.f;
        for (unsigned int i = 0; i < length; i++)
        {
            durationSum += (notes + i)->duration;
        }
        durationAvg = durationSum / length;

        /// Get standard deviation
        float sumDiffMean = 0.f;
        for (unsigned int i = 0; i < length; i++)
        {
            auto diff = (notes + i)->duration - durationAvg;
            sumDiffMean += diff * diff;
        }
        durationSD = std::sqrt(sumDiffMean / length);
    }

    unsigned int NoteString::findIndex(float timestamp)
    {
        /// Simple linear search. Maybe at some point we can implement a binary search of some sort
        /// since timestamps are in order.
        for (unsigned int i = 0; i < length; i++)
        {
            if (notes[i].timestamp == timestamp)
                return i;
        }
        return -1; /// This wraps to max value for unsigned int
    }

    std::string NoteString::str()
    {
        std::string str = "";
        for (unsigned int i = 0; i < length; i++)
        {
            str += "- " + notes[i].str() + '\n';
        }
        str += "Duration average: " + std::to_string(durationAvg) + '\n' +
               "Duration stanrdard deviation: " + std::to_string(durationSD) + '\n';
        return str;
    }

    /// UniformNoteString

    UniformNoteString UniformNoteString::operator=(const UniformNoteString &noteStr)
    {
        // TODO: I don't know if this is needed
        notes = noteStr.notes;
        length = noteStr.length;
        offset = noteStr.offset;

        return *this;
    }

    float UniformNoteString::check_note(const Note &note)
    {
        /// Check if this note is included in this array
        /// TODO: Check if address of `note` is between address of `this->notes` and address of
        /// `this->notes` plus (`this->length` * sizeof(Note)). (I think)

        if (length == 0)
            return 1.f;

        auto num1 = note.duration;
        float divSum = 0.f;
        for (unsigned int i = 0; i < length; i++)
        {
            auto num2 = notes[i].duration;

            /// The more variation in duration of `notes`, the less accurate this will be, since a
            /// random note is more likely to get a high score on some of the notes, even if it is
            /// significantly different. Maybe we can overcome this by comparing it to the average
            /// of all the durations (instead of each duration individually), or maybe take into
            /// account standard deviation/variance somehow.
            /// Maybe an average between the note compared against the average duration, and the
            /// note compared against the note durations individually. This may be a statistical
            /// fallacy of some sort though. We could also just have separate values and maybe can
            /// gain some insight off of them.

            float div = num1 > num2 ? (num1 / num2) : (num2 / num1);

            if (div >= 2)
            {
                /// This implicitly adds a value of 0
                continue;
            }

            divSum += -div + 2;
        }
        /* std::cout << "divSum / length = " << std::to_string(divSum) << " / "
                  << std::to_string(length) << '\n'; */
        return divSum / length;
    }

    float UniformNoteString::get_score()
    {
        if (length < 2)
            return 1.f;

        float totalSum = 0.f;

        for (unsigned int i = 0; i < length; i++)
        {
            auto num1 = (notes + i)->duration;
            float divSum = 0.f;

            for (unsigned int j = 0; j < length; j++)
            {
                auto num2 = (notes + j)->duration;
                float div = num1 > num2 ? (num1 / num2) : (num2 / num1);

                if (div >= 2)
                {
                    /// This implicitly adds a value of 0
                    continue;
                }

                divSum += -div + 2;
            }

            /// Since each note eventually checks with itself, adjust for this padding of score by
            /// excluding it from the sum and length.
            totalSum += (divSum - 1) / (length - 1);
        }

        return totalSum / length;
    }

    float UniformNoteString::compare(const UniformNoteString &noteStr)
    {
        float totalSum = 0.f;
        for (unsigned int i = 0; i < length; i++)
        {
            float num1 = (notes + i)->duration;
            float rowSum = 0.f;
            for (unsigned int j = 0; j < noteStr.length; j++)
            {
                float num2 = (noteStr.notes + j)->duration;
                float div = num1 > num2 ? (num1 / num2) : (num2 / num1);

                if (div >= 2)
                {
                    /// This implicitly adds a value of 0
                    continue;
                }

                rowSum += -div + 2;
            }
            totalSum += rowSum / noteStr.length;
        }
        return totalSum / length;
    }

    /// @brief Searches through `notes` and increases length to match the next notes that are
    /// approximately equal in duration.
    /// @param maxLength Max length of the note string, this should be used to make sure that the
    /// search doesn't go out of bounds.
    /// @param recursiveDepth Recursion depth. Recursive calls will be made in situations where its
    /// unsure whether a note is the same duration or not.
    void UniformNoteString::search(unsigned int maxLength, unsigned int recursiveDepth)
    {
        for (unsigned int i = length; length <= maxLength; i++)
        {
            auto val = check_note(*(notes + i));

            if (val > 0.6f)
            {
                /// Note is very likely the same division/duration
                std::cout << "note at timestamp " << std::to_string((notes + i)->timestamp)
                          << " fits well with val: " << std::to_string(val) << ", length is now "
                          << std::to_string(length + 1) << '\n';
                increaseRight();
                continue;
            }
            if (val < 0.5f || recursiveDepth >= maxRecursiveDepth)
            {
                std::cout << "note at timestamp " << std::to_string((notes + i)->timestamp)
                          << " causing break with val: " << std::to_string(val) << '\n';
                return;
            }

            /// This is the grey area. This will occur in situations where a note is unsure to
            /// be either the same or different division/duration. To check, we can recursively
            /// get the next note string as if this one ended right now.

            /// Check to see if the note string starting at this note fits well

            std::cout << "note at timestamp " << std::to_string((notes + i)->timestamp)
                      << " doesnt fit well with val: " << std::to_string(val) << '\n';

            UniformNoteString nextStr;
            nextStr.notes = notes + i;
            nextStr.length = 1;
            nextStr.search(maxLength - i, recursiveDepth + 1);

            /// Compare strings
            /// There are many cases to cover in this comparison
        }
    }

    void NoteStringContainer::init()
    {
        lengths.reserve(notesLen);

        for (unsigned int i = 0; i < notesLen; i++)
        {
            lengths.push_back(std::vector<unsigned int>());

            for (unsigned int j = i + 1; j < notesLen; j++)
            {
                // Current approach (no order testing):
                // -If note has <0.5, add existing string and break
                // -If note has <0.6, >0.5, push back current string but continue adding
                // -Continue adding otherwise

                // Possible approach (order testing)
                // -For this we need to account for the fact that with the current approach, if a
                // note gets a bad score as the second note in a string, it's possible if that note
                // was further into the string, the score would be higher.


            }
        }
    }
}
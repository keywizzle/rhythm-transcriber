#include "Beat.h"

#include <iostream>
#include <math.h>
#include <numeric>

namespace RhythmTranscriber
{
    extern unsigned int iters;
    /// Weight definition
    float divisionWeight = 0.2f;
    float noteWeight = 0.3f;
    float distWeight = 0.7f;

    /// BaseRatio
    void BaseRatio::simplify()
    {
        float gcd = std::gcd(antecedent, consequent);
        antecedent /= gcd;
        consequent /= gcd;
    }

    void BaseRatio::add(const BaseRatio ratio)
    {
        if (consequent == 0)
        {
            this->antecedent = ratio.antecedent;
            this->consequent = ratio.consequent;
        }
        else if (ratio.consequent == consequent)
        {
            antecedent += ratio.antecedent;
        }
        else
        {
            /// I think `ratio` needs to be in simplest form for the below to work properly. Haven't
            /// had to deal with it yet, but if it becomes an issue `simplify` will have to be used.
            /* ratio.simplify(); */

            auto consequentLCM = std::lcm(consequent, ratio.consequent);

            antecedent = antecedent * (consequentLCM / consequent) +
                         ratio.antecedent * (consequentLCM / ratio.consequent);
            consequent = consequentLCM;
        }
    }

    BaseRatio &BaseRatio::operator+=(const BaseRatio ratio)
    {
        add(ratio);

        return *this;
    }

    /// Beat

    Beat::Beat()
    { /* noteRatios = std::vector<NoteRatio>(32, NoteRatio{}); */
    }

    Beat::Beat(BaseNote *notes)
    {
        this->notes = notes;
        notesLen = 0;

        /// Set start/end time, assuming all notes fit perfectly within the beat.
        startTime = notes->timestamp;
        /// Hopefully this doesn't cause any errors.
        /* endTime = (notes + notesLen)->timestamp; */ /// It caused errors.
        endTime = notes[0].timestamp + notes[0].duration;

        duration = endTime - startTime;

        noteRatios = std::vector<NoteRatio>(16, NoteRatio{});
    }

    Beat::Beat(BaseNote *notes, unsigned int notesLen)
    {
        this->notes = notes;
        this->notesLen = notesLen;

        /// Set start/end time, assuming all notes fit perfectly within the beat.
        startTime = notes->timestamp;
        /// Hopefully this doesn't cause any errors.
        /* endTime = (notes + notesLen)->timestamp; */ /// It caused errors.
        endTime = notes[notesLen - 1].timestamp + notes[notesLen - 1].duration;
        /* endTime = (notes + notesLen - 1)->timestamp + (notes + notesLen - 1)->duration; */

        duration = endTime - startTime;

        /* noteRatios = std::vector<NoteRatio>(notesLen == 0 ? 16 : notesLen, NoteRatio{}); */
        noteRatios = std::vector<NoteRatio>(notesLen, NoteRatio{});

        /* if (noteRatios.size() < notesLen)
        {
            noteRatios.resize(notesLen, NoteRatio{});
        } */
    }
    Beat::Beat(BaseNote *notes, unsigned int notesLen, float beatDuration)
    {
        this->notes = notes;
        this->notesLen = notesLen;

        /// Set start/end time
        startTime = notes->timestamp;
        endTime = startTime + beatDuration;

        duration = beatDuration;

        noteRatios = std::vector<NoteRatio>(notesLen == 0 ? 16 : notesLen, NoteRatio{});

        /* if (noteRatios.size() < notesLen)
        {
            noteRatios.resize(notesLen, NoteRatio{});
        } */
    }

    Beat Beat::create_next()
    {
        /* auto newBeat = Beat(notes + notesLen, 0); */
        auto newBeat = Beat(notes + notesLen);

        /// A beat with antecedent < consequent should not be trying to create the next beat, as it
        /// itself is not complete yet. This is why modulus should catch all *correct* cases.

        if (division.antecedent >= division.consequent * 2)
        {
            newBeat.set_offset(division.antecedent % division.consequent, division.consequent);

            /// This essentially just adds the time of the empty trailing beats.
            newBeat.startTime =
                endTime + duration * (division.antecedent / division.consequent - 1);
        }
        else
        {
            if (division.antecedent > division.consequent)
            {
                newBeat.set_offset(division.antecedent - division.consequent, division.consequent);
            }

            newBeat.startTime = endTime;
        }

        return newBeat;
    }

    void Beat::add_note(NoteRatio noteRatio)
    {
        /// Add to division.
        if (division.consequent == 0)
        {
            division.antecedent = noteRatio.antecedent;
            division.consequent = noteRatio.consequent;
        }
        else if (noteRatio.consequent == division.consequent)
        {
            division.antecedent += noteRatio.antecedent;
        }
        else
        {
            auto consequentLCM = std::lcm(division.consequent, noteRatio.consequent);

            if (consequentLCM != division.consequent)
            {
                /// Note ratio antecedents must be adjusted. This should be fairly rare when
                /// transcribing, but adding notes from fully-simplified ratios could cause this.
                transform_note_ratios(consequentLCM / division.consequent);
            }

            noteRatio.antecedent = noteRatio.antecedent * (consequentLCM / noteRatio.consequent);
            noteRatio.consequent = consequentLCM;

            division.antecedent =
                division.antecedent * (consequentLCM / division.consequent) + noteRatio.antecedent;
            division.consequent = consequentLCM;
        }

        /// Add note to noteRatios.
        if (noteRatios.size() == notesLen)
        {
            noteRatios.resize(notesLen + 8, NoteRatio{});
        }

        noteRatios[notesLen] = noteRatio;

        notesLen++;

        /// If beat is complete, set `endTime`.
        if (division.antecedent == division.consequent)
        {
            endTime = notes[notesLen - 1].timestamp + notes[notesLen - 1].duration;
            duration = endTime - startTime;
        }
        else if (division.antecedent > division.consequent)
        {
            endTime =
                (notes + notesLen - 1)->timestamp +
                (notes + notesLen - 1)->duration *
                    (float)(division.consequent - (division.antecedent - noteRatio.antecedent)) /
                    noteRatio.antecedent;
            duration = endTime - startTime;
        }
    }

    void Beat::transform_note_ratios(float ratio)
    {
        for (unsigned int i = 0; i < notesLen; i++)
        {
            noteRatios[i].antecedent *= ratio;
        }
    }

    bool Beat::set_note_ratios(unsigned int division)
    {
        if (!offset.antecedent)
        {
            this->division = BaseRatio{0, division};
        }

        float baseDuration = duration / division;

        set_note_ratios(division, baseDuration);

        unsigned int doubleDivision = division * 2;

        /// We could have an option to immediately eliminate any division possibilities if the
        /// initial note divisions don't fit into the beat perfectly. This would be advised against,
        /// as it would really only work for simpler rhythms.

        unsigned int iters = 0;
        while (this->division.antecedent != division)
        {
            if (iters > 4)
            {
                /// It is possible that there is no beat duration that can represent the specific
                /// division while fitting all notes perfectly in the beat. This can happen when two
                /// notes have the exact same duration.
                /// It is also rarely possible that this loop can get stuck infinitely ping-ponging
                /// around the desired antecedent.

                return false;
            }

            /// Estimates the beat duration to set to get the desired antecedent.
            baseDuration *= (partialSum + this->division.antecedent) / doubleDivision;

            set_note_ratios(division, baseDuration);

            iters++;
        }

        return true;
    }

    bool Beat::set_offbeat_note_ratios(unsigned int division)
    {
        if (!offset.antecedent)
        {
            this->division = BaseRatio{0, division};
        }

        float baseDuration = duration / division;

        set_note_ratios(division, baseDuration);

        /// Factor to multiply/divide `baseDuration` by. Since this is static, it could lead to
        /// ping-ponging, but due to the low chance of an actual offbeat requiring multiple
        /// adjustments to beat duration it doesn't matter too much at the moment.
        float divisionFactor = 1 + 1.f / division;

        bool endIsValid = end_is_valid();
        bool hasTail = has_tail();

        unsigned int iters = 0;
        while (!endIsValid || !hasTail)
        {
            /// Invalid end => increase base duration (divisions have become too big)
            /// No tail => decrease base duration (divisions have become too small)

            /// End validity and tail presence are exclusive, they cannot both be false (invalid end
            /// means tail is added to already complete beat).

            if (iters > 4)
            {
                /// It is possible that there is no beat duration that can represent the specific
                /// division while fitting all notes perfectly in the beat. This can happen when two
                /// notes have the exact same duration.
                /// It is also rarely possible that this loop can get stuck infinitely ping-ponging
                /// around the desired antecedent.

                return false;
            }

            if (!hasTail)
            {
                /// Decrease `baseDuration`.
                baseDuration /= divisionFactor;
            }
            else
            {
                /// End must be invalid due to the condition for the while loop + conditions cannot
                /// both be false.
                /// Increase `baseDuration`.
                baseDuration *= divisionFactor;
            }

            set_note_ratios(division, baseDuration);

            endIsValid = end_is_valid();
            hasTail = has_tail();

            iters++;
        }

        return true;
    }

    void Beat::calc_note_partials()
    {
        float baseDuration = duration / division.consequent;

        for (unsigned int i = 0; i < notesLen; i++)
        {
            noteRatios[i].partial = notes[i].duration / baseDuration;
        }
    }

    /// @brief
    /// @note This assumes all `noteRatio`s have `partial` set correctly.
    /// @return
    float Beat::calc_score()
    {
        distScore = 0.f;
        divisionScore = 1.f;
        noteScore = 0.f;

        unsigned int noteChangeCount = 0;
        unsigned int prevAntecedent = noteRatios[0].antecedent;
        unsigned int beatAntecedent = 0;

        float divScore = 0.f;

        float distScoreSum = 0.f;
        float distWeightSum = 0.f;

        float beatScoreSum = 0.f;
        float beatWeightSum = 0.f;

        for (unsigned int i = 0; i < notesLen; i++)
        {
            float beatScore = 0.f;
            float beatWeight = 0.f;

            auto antecedent = noteRatios[i].antecedent;

            /// Dist score

            float distScoreBase = noteRatios[i].partial / antecedent;

            /// Humans aren't as good at timing longer notes than they are shorter notes (I think),
            /// unless the notes are repeated consecutively with a metronome.
            /// Basically, longer notes leave more room for error, so make longer notes have
            /// slightly less impact on overall dist score.
            float noteDistWeight =
                (0.05f / ((notes + i)->duration * (notes + i)->duration + 0.05f));

            distScore += noteDistWeight * (distScoreBase > 1 ? 1.f / distScoreBase : distScoreBase);

            distWeightSum += noteDistWeight;

            /// In the case we're scoring a multi-beat, going over the consequent should reset the
            /// running antecedent.
            /// `prevAntecedent` should also be reset since we're essentially in a new beat and
            /// don't want the actual previous antecedent to affect anything.
            if (beatAntecedent >= this->division.consequent)
            {
                beatAntecedent = beatAntecedent - this->division.consequent;

                prevAntecedent = beatAntecedent == 0 ? antecedent : beatAntecedent;
            }

            /// Division score
            /// Omit when division changes on eighth note (at least for now) to avoid always buffing
            /// (or hurting) with ~0.97 division score.
            if (antecedent != prevAntecedent /* &&
                beatAntecedent != (float)this->division.consequent / 2 */
                && beatAntecedent < this->division.consequent)
            {
                if (antecedent > prevAntecedent)
                {
                    if (antecedent % prevAntecedent != 0)
                    {
                        divisionScore *=
                            beatDivisionScoreTable[this->division.consequent][beatAntecedent];

                        noteChangeCount++;
                    }
                }
                else if (prevAntecedent % antecedent != 0)
                {
                    divisionScore *=
                        beatDivisionScoreTable[this->division.consequent][beatAntecedent];

                    noteChangeCount++;
                }
            }

            /// Note score
            noteScore += noteDivisionScoreTable[this->division.consequent][antecedent];

            prevAntecedent = antecedent;

            beatAntecedent += antecedent;
        }

        distScore /= distWeightSum;

        noteScore /= notesLen;

        return score = ((distWeight * distScore * divisionScore) + noteWeight * noteScore) /
                       (distWeight + noteWeight);
    }

    void Beat::calc_time()
    {
        /// I'm not sure which method for calculating time is the correct one, but I think it's
        /// this one.
        /* endTime = (notes + notesLen - 1)->timestamp +
                  (notes + notesLen - 1)->duration *
                      ((float)(division.consequent -
                               (division.antecedent - noteRatios[notesLen - 1].antecedent)) /
                       division.consequent); */
        endTime = (notes + notesLen - 1)->timestamp +
                  (notes + notesLen - 1)->duration *
                      ((float)(division.consequent -
                               (division.antecedent - noteRatios[notesLen - 1].antecedent)) /
                       noteRatios[notesLen - 1].antecedent);

        duration = endTime - startTime;
    }

    std::vector<Beat> Beat::get_trailing_beats()
    {
        if (division.antecedent <= division.consequent)
        {
            return std::vector<Beat>();
        }

        auto tailAntecedent = division.antecedent - division.consequent;
        auto tailConsequent = division.consequent;

        unsigned int trailingBeatLen = std::ceil(tailAntecedent / tailConsequent);

        std::vector<Beat> trailingBeats = std::vector<Beat>(trailingBeatLen, Beat());
        trailingBeats.at(trailingBeatLen - 1).division.antecedent =
            tailConsequent - (trailingBeatLen * tailConsequent - tailAntecedent);

        return trailingBeats;
    }

    std::string Beat::str()
    {
        std::string str =
            "bpm: " + std::to_string(60.f / duration) + " " + std::to_string(startTime) + "->" +
            std::to_string(endTime) + /* ", notesLen: " + std::to_string(notesLen) + */
            (offset.antecedent == 0 ? ""
                                    : (", offset: " + std::to_string(offset.antecedent) + '/' +
                                       std::to_string(offset.consequent))) +
            ", division: " + std::to_string(division.antecedent) + "/" +
            std::to_string(division.consequent) + ", score: " + std::to_string(score) +
            (score == 0.f ? ""
                          : " (distScore: " + std::to_string(distScore) +
                                ", divScore: " + std::to_string(divisionScore) +
                                ", noteScore: " + std::to_string(noteScore)) +
            ", notes: ";

        for (unsigned int i = 0; i < notesLen; i++)
        {
            str += std::to_string(noteRatios[i].antecedent) + '/' +
                   std::to_string(/* noteRatios[i].consequent */ division.consequent);
            if (i < notesLen - 1)
            {
                str += ", ";
            }
        }

        return str;
    }

    /* Beat::Beat(float timestamp) { this->timestamp = timestamp; }
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

    void Beat::addNoteInterpretation(NoteInterpretation &note)
    {
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
    } */
}
#include "Beat.h"

#include <iostream>
#include <math.h>
#include <numeric>

namespace RhythmTranscriber
{
    /// Weight definition
    float divisionWeight = 0.2f;
    float noteWeight = 0.3f;
    float distWeight = 0.7f;

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

        /* noteValues = std::vector<NoteRatio>(16, NoteRatio{}); */
        noteValues = std::vector<unsigned int>(16, 0);
    }

    Beat::Beat(BaseNote *notes, unsigned int notesLen)
    {
        this->notes = notes;
        this->notesLen = notesLen;

        /// Set start/end time, assuming all notes fit perfectly within the beat.
        startTime = notes->timestamp;
        endTime = notes[notesLen - 1].timestamp + notes[notesLen - 1].duration;

        duration = endTime - startTime;

        noteValues = std::vector<unsigned int>(notesLen, 0);
    }
    Beat::Beat(BaseNote *notes, unsigned int notesLen, float beatDuration)
    {
        this->notes = notes;
        this->notesLen = notesLen;

        /// Set start/end time
        startTime = notes->timestamp;
        endTime = startTime + beatDuration;

        duration = beatDuration;

        /* noteValues = std::vector<NoteRatio>(notesLen == 0 ? 16 : notesLen, NoteRatio{}); */
        noteValues = std::vector<unsigned int>(notesLen, 0);

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

    void Beat::add_note(Ratio noteRatio)
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
                transform_note_values(consequentLCM / division.consequent);
            }

            noteRatio.antecedent = noteRatio.antecedent * (consequentLCM / noteRatio.consequent);

            division.antecedent =
                division.antecedent * (consequentLCM / division.consequent) + noteRatio.antecedent;
            division.consequent = consequentLCM;
        }

        if (noteValues.size() == notesLen)
        {
            noteValues.resize(notesLen + 8, 0);
        }

        noteValues[notesLen] = noteRatio.antecedent;

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

    void Beat::transform_note_values(float ratio)
    {
        for (unsigned int i = 0; i < notesLen; i++)
        {
            noteValues[i] *= ratio;
        }
    }

    bool Beat::set_note_values(unsigned int division)
    {
        /// I don't think we actually should bother checking for offset, since I don't think it's
        /// possible to call this on a beat with an offset due to `expand_beat` being the one to set
        /// offsets. TODO remove...? Although maybe we get into a situation where this will be
        /// important.
        if (!offset.antecedent)
        {
            this->division = Ratio{0, division};
        }

        division *= subBeatCount;

        float baseDuration = duration / division;

        set_note_values(baseDuration);

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

            set_note_values(baseDuration);

            iters++;
        }

        return true;
    }

    bool Beat::set_offbeat_note_values(unsigned int division)
    {
        /// I don't think we actually should bother checking for offset, since I don't think it's
        /// possible to call this on a beat with an offset due to `expand_beat` being the one to set
        /// offsets. TODO remove...? Although maybe we get into a situation where this will be
        /// important.
        if (!offset.antecedent)
        {
            this->division = Ratio{0, division};
        }

        division *= subBeatCount;

        float baseDuration = duration / division;

        set_note_values(baseDuration);

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

            set_note_values(baseDuration);

            endIsValid = end_is_valid();
            hasTail = has_tail();

            iters++;
        }

        return true;
    }

    float Beat::calc_score()
    {
        distScore = 0.f;
        divisionScore = 1.f;
        noteScore = 0.f;

        float baseDuration = duration / (division.consequent * subBeatCount);

        unsigned int prevAntecedent = noteValues[0];
        unsigned int beatAntecedent = 0;

        float distScoreSum = 0.f;
        float distWeightSum = 0.f;

        float beatScoreSum = 0.f;
        float beatWeightSum = 0.f;

        /// These are for calculating (weighted) standard deviation to apply with dist score. It
        /// adds a bit of overhead for a very slight boost in accuracy. TODO: Add this at some
        /// point.
        /* float noteDistSum = 0.f;
        /// sum x
        float distSumSquared = 0.f;
        /// sum wx^2
        float weightDistSquaredSum = 0.f; */

        for (unsigned int i = 0; i < notesLen; i++)
        {
            float beatScore = 0.f;
            float beatWeight = 0.f;

            auto antecedent = noteValues[i];

            /// Dist score

            /* float distScoreBase = noteValues[i].partial / antecedent; */
            float distScoreBase = (notes[i].duration / baseDuration) / antecedent;

            /// Humans aren't as good at timing longer notes than they are shorter notes (I think),
            /// unless the notes are repeated consecutively with a metronome.
            /// Basically, longer notes leave more room for error, so make longer notes have
            /// slightly less impact on overall dist score.
            /* float noteDistWeight = (0.05f / (notes[i].duration * notes[i].duration + 0.05f)); */
            float noteDistWeight = (0.1f / (notes[i].duration * notes[i].duration + 0.1f));

            float noteDistScore = (distScoreBase > 1 ? 1.f / distScoreBase : distScoreBase);

            distScore += noteDistWeight * noteDistScore;
            distWeightSum += noteDistWeight;

            /* noteDistSum += noteDistScore;
            distSumSquared += noteDistScore * noteDistScore;
            weightDistSquaredSum += noteDistWeight * noteDistScore * noteDistScore; */

            /// In the case we're scoring a multi-beat, going over the consequent should reset the
            /// running antecedent.
            /// `prevAntecedent` should also be reset since we're essentially in a new beat and
            /// don't want the actual previous antecedent to affect anything.
            if (beatAntecedent >= this->division.consequent)
            {
                beatAntecedent = beatAntecedent - this->division.consequent;

                prevAntecedent = beatAntecedent == 0 ? antecedent : beatAntecedent;
            }

            /// Sub-beat score
            if (antecedent != prevAntecedent && beatAntecedent < this->division.consequent)
            {
                if (antecedent > prevAntecedent)
                {
                    if (antecedent % prevAntecedent != 0)
                    {
                        divisionScore *=
                            beatDivisionScoreTable[this->division.consequent][beatAntecedent];
                    }
                }
                else if (prevAntecedent % antecedent != 0)
                {
                    divisionScore *=
                        beatDivisionScoreTable[this->division.consequent][beatAntecedent];
                }
            }

            /// Note score
            noteScore += noteDivisionScoreTable[this->division.consequent][antecedent];

            prevAntecedent = antecedent;

            beatAntecedent += antecedent;
        }

        distScore /= distWeightSum;

        noteScore /= notesLen;

        /* float noteDistMean = noteDistSum / notesLen;
        float noteDistSd =
            std::sqrt((weightDistSquaredSum +
                       distScore * (distScore * distWeightSum - 2 * distScore * distWeightSum)) /
                      distWeightSum);
        distScore = 0.7f * distScore + 0.3f * (0.01f / (noteDistSd * noteDistSd + 0.01f)); */

        if (division.antecedent % division.consequent && division.consequent % division.antecedent)
        {
            /// Segfault here if antecedent is smaller than consequent (but that shouldn't occur
            /// anyways)
            divisionScore *= beatDivisionScoreTable[division.consequent]
                                                   [division.antecedent - division.consequent];
        }

        /* return score = ((distWeight * distScore * divisionScore) + noteWeight * noteScore) /
                       (distWeight + noteWeight); */
        return score = ((distWeight * distScore * divisionScore) + noteWeight * noteScore) /
                       (distWeight + noteWeight);
    }

    void Beat::calc_time()
    {
        endTime =
            (notes + notesLen - 1)->timestamp +
            (notes + notesLen - 1)->duration *
                ((float)(division.consequent - (division.antecedent - noteValues[notesLen - 1])) /
                 noteValues[notesLen - 1]);

        duration = endTime - startTime;
    }

    float Beat::note_dist_sd()
    {
        unsigned int effectiveConsequent = division.consequent * subBeatCount;

        float baseDuration = duration / effectiveConsequent;

        float distSum = 0.f;
        float distWeightSum = 0.f;

        for (unsigned int i = 0; i < notesLen; i++)
        {
            float distScoreBase = (notes[i].duration / baseDuration) / noteValues[i];

            float noteDistWeight = (0.1f / (notes[i].duration * notes[i].duration + 0.1f));

            distSum += noteDistWeight * (distScoreBase > 1 ? 1.f / distScoreBase : distScoreBase);

            distWeightSum += noteDistWeight;
        }

        float avgDist = distSum / distWeightSum;

        float avgDiffSum = 0.f;
        for (unsigned int i = 0; i < notesLen; i++)
        {
            float distScoreBase = (notes[i].duration / baseDuration) / noteValues[i];

            avgDiffSum += (distScoreBase - avgDist) * (distScoreBase - avgDist);
        }

        /* float distSd = std::sqrt(avgDiffSum / notesLen); */
        float distSd = avgDiffSum / notesLen;

        /* std::cout << "sd: " << distSd << '\n'; */

        return distSd;
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
            str += std::to_string(noteValues[i]) + '/' + std::to_string(division.consequent);
            if (i < notesLen - 1)
            {
                str += ", ";
            }
        }

        return str;
    }
}
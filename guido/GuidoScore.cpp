#include "GuidoScore.h"

namespace RhythmTranscriber
{
    /// Note

    /// Beat

    GuidoScore::Beat::Beat() {}

    GuidoScore::Beat::Beat(std::span<Transcription::NoteElement> notes) { add_notes(notes); }

    GuidoScore::Beat::Beat(Ratio offset, std::span<Transcription::NoteElement> notes)
    {
        this->offset.antecedent = offset.antecedent;
        this->offset.consequent = offset.consequent;

        division.antecedent = offset.antecedent;
        division.consequent = offset.consequent;

        add_notes(notes);
    }

    void GuidoScore::Beat::add_notes(std::span<Transcription::NoteElement> notes)
    {
        notesEnd = notes.end();

        for (auto pNote = notes.begin(); pNote <= notesEnd; ++pNote)
        {
            if (pNote->flam)
            {
                continue;
            }

            division.add(
                Ratio{.antecedent = pNote->rhythm.beats, .consequent = pNote->rhythm.notes});

            if (pNote > notes.begin() && (pNote - 1)->rhythm.notes != pNote->rhythm.notes)
            {
                isUniform = false;
            }

            if (division.antecedent >= division.consequent)
            {
                this->notes = std::span<Transcription::NoteElement>(notes.begin(), pNote + 1);

                return;
            }
        }
    }

    GuidoScore::Beat GuidoScore::Beat::create_next()
    {
        Beat newBeat = division.antecedent > division.consequent
                           ? Beat(Ratio{.antecedent = division.antecedent % division.consequent,
                                        .consequent = division.consequent},
                                  std::span<Transcription::NoteElement>(notes.end(), notesEnd))
                           : Beat(std::span<Transcription::NoteElement>(notes.end(), notesEnd));

        return newBeat;
    }

    std::string GuidoScore::Beat::get_str()
    {
        std::string str = "";

        if (offset.antecedent > 0)
        {
            str += "offset: ";
            str += std::to_string(offset.antecedent);
            str += '/';
            str += std::to_string(offset.consequent);
            str += ", ";
        }

        str += "notes: ";

        for (auto &note : notes)
        {
            str += std::to_string(note.rhythm.beats);
            str += '/';
            str += std::to_string(note.rhythm.notes);
            str += ' ';
        }

        str += ", uniform: ";
        str += isUniform ? "true" : "false";
        str += ", tuplet: ";
        str += is_tuplet() ? "true" : "false";

        return str;
    }

    GuidoScore::GuidoScore(Transcription::Transcription transcription)
    {
        this->transcription = transcription;

        Beat beat = Beat(std::span<Transcription::NoteElement>(&(this->transcription.notes[0]),
                                                               this->transcription.notes.size()));

        beats.push_back(beat);

        while (beat.notes.end() != beat.notesEnd)
        {
            beat = beat.create_next();
            beats.push_back(beat);
        }

        currentBeat = beats.begin();
    }

    std::string GuidoScore::get_str()
    {
        scoreStr.reserve(512);

        scoreStr += '[';

        add_clef();

        add_meter('4', '4');

        scoreStr += '\n';

        for (currentBeat = beats.begin(); currentBeat < beats.end(); ++currentBeat)
        {
            std::cout << "beat: " << currentBeat->get_str() << '\n';

            add_beat();
        }

        return scoreStr += ']';
    }

    void GuidoScore::add_meter(unsigned int numerator, unsigned int denominator)
    {
        scoreStr += "\\meter<\"" + std::to_string(numerator) + '/' + std::to_string(denominator);
        scoreStr += "\">";
    }
    void GuidoScore::add_meter(char numerator, char denominator)
    {
        scoreStr += "\\meter<\"";
        scoreStr += numerator;
        scoreStr += '/';
        scoreStr += denominator;
        scoreStr += "\">";
    }

    void GuidoScore::add_clef() { scoreStr += "\\clef<\"perc\">"; }

    void GuidoScore::add_note(Transcription::NoteElement note)
    {
        if (note.flam)
        {
            /* scoreStr += "\\noteFormat<dx=0.5>(\\trem<\"/\",dy=-1.25,thickness=0.3>(\\grace(" +
                        format.defaultNoteName;
            scoreStr += "*1/8)))"; */
            scoreStr += "\\beam(\\noteFormat<dx=0.5>(\\grace(" +
                        format.defaultNoteName;
            scoreStr += "*1/8)))";

            return;
        }

        note.rhythm.notes *= 4;
        scoreStr += format.defaultNoteName + '*';
        scoreStr += std::to_string(note.rhythm.beats);
        scoreStr += '/';
        scoreStr += std::to_string(note.rhythm.notes);
        scoreStr += ' ';
    }
    void GuidoScore::add_note_value(Transcription::NoteElement note)
    {
        note.rhythm.notes *= 4;
        scoreStr += format.defaultNoteName + '*';
        scoreStr += std::to_string(note.rhythm.beats);
        scoreStr += '/';
        scoreStr += std::to_string(note.rhythm.notes);
    }

    void GuidoScore::add_rest(Transcription::NoteElement note)
    {
        note.rhythm.notes *= 4;
        scoreStr += "_*";
        scoreStr += std::to_string(note.rhythm.beats);
        scoreStr += '/';
        scoreStr += std::to_string(note.rhythm.notes);
    }

    void GuidoScore::add_beat()
    {
        bool isTuplet = currentBeat->is_tuplet() && currentBeat->is_uniform() &&
                        currentBeat->division.antecedent > 3;

        /* scoreStr += "\\beam("; */

        if (isTuplet)
        {
            scoreStr += "\\tuplet<\"-";
            scoreStr += std::to_string(currentBeat->division.antecedent);
            scoreStr += "-\",dy1=0.5,dy2=0.5>(";
        }

        if (currentBeat->offset_is_rest)
        {
            scoreStr += "_*";
            scoreStr += std::to_string(currentBeat->offset.antecedent);
            scoreStr += '/';
            scoreStr += std::to_string(currentBeat->offset.consequent * 4);

            scoreStr += ' ';
        }

        if (!isTuplet && currentBeat->has_tail())
        {
            /// Add notes like normal, except trim last note.

            currentBeat->tail_is_trimmed = true;

            for (auto noteIter = currentBeat->notes.begin(); noteIter < currentBeat->notes.end();
                 ++noteIter)
            {
                if (noteIter == currentBeat->notes.end() - 1)
                {
                    auto trimmedNote = *noteIter;

                    auto trimmedRatio = Ratio{noteIter->rhythm.beats, noteIter->rhythm.notes} -
                                        currentBeat->get_tail();

                    trimmedNote.rhythm = {.notes = (unsigned short)trimmedRatio.consequent,
                                          .beats = (unsigned short)trimmedRatio.antecedent};

                    add_note(trimmedNote);
                }
                else
                {
                    add_note(*noteIter);
                }
            }

            /// Flag next beat to add a rest for the offset
            (currentBeat + 1)->offset_is_rest = true;
        }
        else
        {
            for (const auto &note : currentBeat->notes)
            {
                add_note(note);
            }
        }

        if (isTuplet)
        {
            scoreStr += ')';
        }

        /* scoreStr += ')'; /// beam */

        scoreStr += '\n';
    }
}
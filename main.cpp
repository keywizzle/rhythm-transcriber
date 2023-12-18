#include "Transcriber.h"
#include "utils.h"
#include <algorithm>
#include <iostream>
#include <math.h>
#include <numeric>
#include <vector>

#include "guido/GuidoScore.h"
#include "tests/tests.h"
#include "transcription/TranscriptionFile.h"

void view_transcription(std::string source)
{
    using namespace RhythmTranscriber;

    Transcription::TranscriptionFile transcriptionFile = Transcription::TranscriptionFile();
    auto transcription = transcriptionFile.read(".\\test data\\" + source + ".json");

    std::vector<BaseNote> notes;
    std::vector<Ratio> noteRatios;

    for (unsigned int i = 0; i < transcription.notes.size(); i++)
    {
        auto note = transcription.notes[i];

        if (note.flam)
            continue;

        notes.push_back(
            BaseNote{note.timestamp, transcription.notes[i + 1].timestamp - note.timestamp});
        noteRatios.push_back(Ratio{note.rhythm.beats, note.rhythm.notes});
    }

    std::vector<Beat> beats;

    /* Beat beat = Beat(reinterpret_cast<BaseNote *>(&(transcription.notes[0])), 0); */
    Beat beat = Beat(&(notes[0]));

    for (unsigned int i = 0; i < noteRatios.size(); i++)
    {
        auto noteRatio = Ratio{noteRatios[i].antecedent, noteRatios[i].consequent};

        beat.add_note(Ratio{noteRatios[i].antecedent, noteRatios[i].consequent});

        if (beat.division.antecedent >= beat.division.consequent)
        {
            beat.calc_score();

            std::cout << beat.str() << '\n';
            /* std::cout << "sd: " << std::sqrt(beat.note_dist_sd())
                      << " (no sqrt: " << beat.note_dist_sd() << ")\n"; */

            beats.push_back(beat);

            if (i == noteRatios.size() - 1)
            {
                break;
            }

            beat = beat.create_next();
        }
    }
}

int main()
{
    using namespace RhythmTranscriber::Transcription;

    /* view_transcription("bd 2022"); */

    RhythmTranscriber::run_tests();

    /* RhythmTranscriber::run_benchmarks(); */

    Sleep(50000);
    return 0;

    std::string fileName = "rhythm X 2022";
    float bpm = 120.f;

    TranscriptionFile transcriptionFile = TranscriptionFile();
    auto transcription = transcriptionFile.read(".\\test data\\" + fileName + ".json");
    /* transcriptionFile.write(".\\test data\\" + fileName + ".json", transcription,
                            TranscriptionFile::WriteOptions{
                                TranscriptionFile::WriteOptions::Type::JSON, 1,
                                TranscriptionFile::WriteOptions::Compression::OMIT_EMPTY_FIELDS});
    transcriptionFile.write(".\\test data\\" + fileName + ".transcription", transcription); */

    auto guidoScore = RhythmTranscriber::GuidoScore(transcription);
    std::cout << "****************************************************\n";
    std::cout << guidoScore.get_str() << '\n';
    std::cout << "****************************************************\n";

    Sleep(5000);
    return 0;

    std::vector<float> timestamps;
    for (unsigned int i = 0; i < transcription.notes.size(); i++)
    {
        if (transcription.notes.at(i).flam)
            continue;
        timestamps.push_back(transcription.notes.at(i).timestamp);
    }

    RhythmTranscriber::Transcriber transcriber =
        RhythmTranscriber::Transcriber(&(timestamps[0]), timestamps.size());
    transcriber.transcribe(bpm, 1);

    Sleep(5000);

    return 0;
}
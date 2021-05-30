#include <cstddef>
#include <cstring>
#include "high_scores.h"
#include "gamevars.h"

using namespace ZZT;

HighScoreList::HighScoreList(Game *game) {
    this->game = game;
    Clear();
}

void HighScoreList::Clear(void) {
    for (int i = 0; i < HIGH_SCORE_COUNT; i++) {
        StrClear(entries[i].name);
        entries[i].score = -1;
    }
}

void HighScoreList::Load(const char *filename) {
    sstring<255> filename_joined;
    StrJoin(filename_joined, 2, filename, ".HI");

    IOStream *stream = game->filesystem->open_file(filename_joined, false);

    for (int i = 0; i < HIGH_SCORE_COUNT; i++) {
        if (stream->errored()) break;
        stream->read_pstring(entries[i].name, StrSize(entries[i].name), 50, false);
        entries[i].score = stream->read16();
    }

    if (stream->errored()) {
        Clear();
    }

    delete stream;
}

void HighScoreList::Save(const char *filename) {
    sstring<255> filename_joined;
    StrJoin(filename_joined, 2, filename, ".HI");

    IOStream *stream = game->filesystem->open_file(filename_joined, true);

    for (int i = 0; i < HIGH_SCORE_COUNT; i++) {
        if (stream->errored()) break;
        stream->write_pstring(entries[i].name, 50, false);
        stream->write16(entries[i].score);
    }

    if (stream->errored()) {
        game->DisplayIOError(*stream);
    }

    delete stream;
}

void HighScoreList::InitTextWindow(TextWindow &window) {
    sstring<20> scoreStr;
    sstring<20> scoreStrPadded;
    scoreStrPadded[20] = 0;

    window.Clear();
    window.Append("Score  Name");
    window.Append("-----  ----------------------------------");
    for (int i = 0; i < HIGH_SCORE_COUNT; i++) {
        if (entries[i].name[0] != 0) {
            StrFromInt(scoreStr, entries[i].score);
            int scoreStrLen = StrLength(scoreStr);
            if (scoreStrLen < 5) {
                int padLen = 5 - scoreStrLen;
                for (int i = 0; i < padLen; i++) {
                    scoreStrPadded[i] = ' ';
                }
                strncpy(scoreStrPadded + padLen, scoreStr, StrSize(scoreStrPadded) - padLen);
            } else {
                StrCopy(scoreStrPadded, scoreStr);
            }

            window.Append(DynString(scoreStrPadded) + "  " + entries[i].name);
        }
    }
}

void HighScoreList::Display(const char *worldName, int16_t line_pos) {
    TextWindow window = TextWindow(game->driver, game->filesystem);

    window.line_pos = line_pos;
    InitTextWindow(window);
    if (window.line_count > 2) {
        StrJoin(window.title, 2, "High scores for ", worldName);
        window.DrawOpen();
        window.Select(false, true);
        window.DrawClose();
    }
}

void HighScoreList::Add(const char *worldName, int16_t score) {
    if (score <= 0) return;
    if (game->filesystem->is_read_only()) return;

    sstring<50> name;
    int16_t list_pos = 0;

    while (list_pos < HIGH_SCORE_COUNT && score < entries[list_pos].score) {
        list_pos++;
    }
    
    if (list_pos < HIGH_SCORE_COUNT) {
        TextWindow window = TextWindow(game->driver, game->filesystem);

        for (int i = (HIGH_SCORE_COUNT - 2); i >= list_pos; i--) {
            entries[i + 1] = entries[i];
        }

        entries[list_pos].score = score;
        StrCopy(entries[list_pos].name, "-- You! --");

        InitTextWindow(window);
        window.line_pos = list_pos;
        StrJoin(window.title, 2, "New high score for ", worldName);
        window.DrawOpen();
        window.Draw(false, false);

        StrClear(name);
        game->interface->PopupPromptString("Congratulations!  Enter your name:", name, sizeof(name));
        StrCopy(entries[list_pos].name, name);
        Save(worldName);

        window.DrawClose();
        game->TransitionDrawToBoard();
    }
}
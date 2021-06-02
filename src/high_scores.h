#ifndef __HIGH_SCORES_H__
#define __HIGH_SCORES_H__

#include <cstdint>
#include "txtwind.h"

#define HIGH_SCORE_COUNT 30

// separated from EDITOR.PAS

namespace ZZT {
    class Game;

    struct HighScoreEntry {
        int16_t score;
        sstring<60> name;
    };

    class HighScoreList {
    protected:
        Game *game;
        HighScoreEntry entries[HIGH_SCORE_COUNT];

        void InitTextWindow(TextWindow &window);

    public:
        HighScoreList(Game *game);

        void Clear(void);
        void Load(const char *worldName);
        void Save(const char *worldName);
        void Display(const char *worldName, int16_t line_pos);
        void Add(const char *worldName, int16_t score);
    };
}

#endif
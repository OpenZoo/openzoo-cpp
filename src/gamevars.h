#ifndef __GAMEVARS_H__
#define __GAMEVARS_H__

#include <cstddef>
#include <cstdint>
#include <cstdlib> // TODO
#include "utils.h"
#include "filesystem.h"
#include "driver.h"
#include "sounds.h"
#include "txtwind.h"
#include "high_scores.h"
#include "user_interface.h"
#include "world_serializer.h"

#define MAX_BOARD 100
#define MAX_STAT 150
#define MAX_FLAG 10
#define TORCH_DURATION 200
#define TORCH_DX 8
#define TORCH_DY 5
#define TORCH_DIST_SQR 50

namespace ZZT {
    typedef enum {
        EditorCategoryNone = 0,
        EditorCategoryItem = 1,
        EditorCategoryCreature = 2,
        EditorCategoryTerrain = 3
    } EditorCategory;

    typedef enum {
        ShotSourcePlayer = 0,
        ShotSourceEnemy = 1
    } ShotSource;

    typedef enum {
        EEmpty = 0,
        EBoardEdge = 1,
        EMessageTimer = 2,
        EMonitor = 3,
        EPlayer = 4,
        EAmmo = 5,
        ETorch = 6,
        EGem = 7,
        EKey = 8,
        EDoor = 9,
        EScroll = 10,
        EPassage = 11,
        EDuplicator = 12,
        EBomb = 13,
        EEnergizer = 14,
        EStar = 15,
        EConveyorCW = 16,
        EConveyorCCW = 17,
        EBullet = 18,
        EWater = 19,
        EForest = 20,
        ESolid = 21,
        ENormal = 22,
        EBreakable = 23,
        EBoulder = 24,
        ESliderNS = 25,
        ESliderEW = 26,
        EFake = 27,
        EInvisible = 28,
        EBlinkWall = 29,
        ETransporter = 30,
        ELine = 31,
        ERicochet = 32,
        EBlinkRayEw = 33,
        EBear = 34,
        ERuffian = 35,
        EObject = 36,
        ESlime = 37,
        EShark = 38,
        ESpinningGun = 39,
        EPusher = 40,
        ELion = 41,
        ETiger = 42,
        EBlinkRayNs = 43,
        ECentipedeHead = 44,
        ECentipedeSegment = 45,
        ETextBlue = 47,
        ETextGreen = 48,
        ETextCyan = 49,
        ETextRed = 50,
        ETextPurple = 51,
        ETextYellow = 52,
        ETextWhite = 53,
        ElementCount
    } ElementType;

    static constexpr const uint8_t ColorSpecialMin = 0xF0;
    static constexpr const uint8_t ColorChoiceOnBlack = 0xFF;
    static constexpr const uint8_t ColorWhiteOnChoice = 0xFE;
    static constexpr const uint8_t ColorChoiceOnChoice = 0xFD;

    struct Coord {
        uint8_t x, y;
    };

    struct Tile {
        uint8_t element;
        uint8_t color;
    };

    struct RLETile {
	    uint8_t count;
        Tile tile;
    };

    class StatData {
    public:
        char *data = nullptr;
        int16_t len = 0;
    
        StatData() = default;

        inline bool operator==(const StatData &b) const {
            return data == b.data;
        }

        inline bool operator!=(const StatData &b) const {
            return data != b.data;
        }

        inline void duplicate() {
            if (data != nullptr && len > 0) {
                char *new_data = (char*) malloc(len);
                memcpy(new_data, data, len);
                data = new_data;
            }
        }

        inline void clear_data() {
            data = nullptr;
            len = 0;
        }

        inline void free_data() {
            if (data != nullptr && len > 0) {
                free(data);
                clear_data();
            }
        }

        inline void alloc_data(int16_t length) {
            if (len != length) {
                len = length;
                if (len > 0) {
                    data = (char*) malloc(len);
                }
            }
        }
    };

    struct Stat {
        uint8_t x = 0, y = 0;
        int16_t step_x = 0, step_y = 0;
        int16_t cycle = 0;
        uint8_t p1 = 0, p2 = 0, p3 = 0;
        int16_t follower = -1, leader = -1;
        Tile under = {
            .element = 0,
            .color = 0x00
        };
        StatData data = StatData();
        int16_t data_pos = 0;
    };

    struct BoardInfo {
        uint8_t max_shots;
        bool is_dark;
        uint8_t neighbor_boards[4];
        bool reenter_when_zapped;
        sstring<58> message;
        uint8_t start_player_x;
        uint8_t start_player_y;
        int16_t time_limit_seconds;
    };

    struct WorldInfo {
        int16_t ammo;
        int16_t gems;
        bool keys[7];
        int16_t health;
        int16_t current_board;
        int16_t torches;
        int16_t torch_ticks;
        int16_t energizer_ticks;
        int16_t score;
        sstring<20> name;
        sstring<20> flags[MAX_FLAG];
        int16_t board_time_sec;
        int16_t board_time_hsec;
        bool is_save;
    };

    template<int bwidth, int bheight>
    class TileMap {
        Tile tiles[(bwidth + 2) * (bheight + 2)];
        Tile empty = {.element = EBoardEdge, .color = 0x00};

    public:
        bool valid(int16_t x, int16_t y) const {
            return x >= 0 && y >= 0 && x <= (bwidth + 1) && y <= (bheight + 1);
        }

        const Tile& get(int16_t x, int16_t y) const {
            return valid(x, y) ? tiles[x * (bheight + 2) + y] : empty;
        }

        void set(int16_t x, int16_t y, Tile tile) {
            if (valid(x, y)) {
                tiles[x * (bheight + 2) + y] = tile;
            }
        }

        void set_element(int16_t x, int16_t y, uint8_t element) {
            if (valid(x, y)) {
                tiles[x * (bheight + 2) + y].element = element;
            }
        }

        void set_color(int16_t x, int16_t y, uint8_t color) {
            if (valid(x, y)) {
                tiles[x * (bheight + 2) + y].color = color;
            }
        }
    };

    template<size_t size>
    class StatList {
        // -1 .. size + 1 => size + 3 stats
        Stat stats[size + 3];
    
    public:
        StatList() {
            // set stat -1 to out of bounds values
            stats[0] = {
                .x = 0,
                .y = 1,
                .step_x = 256,
                .step_y = 256,
                .cycle = 256,
                .p1 = 0,
                .p2 = 1,
                .p3 = 0,
                .follower = 1,
                .leader = 1,
                .under = {
                    .element = 1,
                    .color = 0x00
                },
                .data = {
                    .len = 1
                },
                .data_pos = 1
            };
        }

        int16_t count;

        Stat& operator[](int16_t idx) {
            return stats[idx + 1];
        }

        Stat& at(int16_t x, int16_t y) {
            for (int i = 1; i <= count+1; i++) {
                if (stats[i].x == x && stats[i].y == y)
                    return stats[i];
            }
            return stats[0];
        }

        int16_t id_at(int16_t x, int16_t y) {
            for (int i = 1; i <= count+1; i++) {
                if (stats[i].x == x && stats[i].y == y)
                    return i - 1;
            }
            return -1;
        }

        bool exists(int16_t value) {
            return value >= 0 && value <= count;
        }

        bool exists_non_player(int16_t value) {
            return value >= 1 && value <= count;
        }

        bool valid(int16_t value) {
            return value >= -1 && value <= (size + 1);
        }

        void free_data_if_unused(int16_t stat_id) {
            if (!valid(stat_id)) return;
            Stat &stat = stats[stat_id + 1];
            if (stat.data.len != 0) {
                bool found = false;
                for (int i = 0; i <= count; i++) {
                    if (i == stat_id) continue;
                    if (stats[i + 1].data == stat.data) {
                        found = true;
                        break;
                    }
                }
                if (found) {
                    stat.data.clear_data();
                } else {
                    stat.data.free_data();
                }
            }
        }
    };

    class Board {
    public:
        Board() { }

        sstring<50> name;
        TileMap<60, 25> tiles;
        StatList<MAX_STAT> stats;
        BoardInfo info;

        constexpr int width(void) { return 60; }
        constexpr int height(void) { return 25; }
    };

    class World {
    public:
        World() { }

        int16_t board_count;
        uint8_t *board_data[MAX_BOARD + 1];
        size_t board_len[MAX_BOARD + 1];
        WorldInfo info;
    };

    class Game;

    struct ElementDef {
        uint8_t character = ' ';
        uint8_t color = ColorChoiceOnBlack;
        bool destructible = false;
        bool pushable = false;
        bool visible_in_dark = false;
        bool placeable_on_top = false;
        bool walkable = false;
        bool has_draw_proc = false;
        void (*draw)(Game &game, int16_t x, int16_t y, uint8_t &chr);
        int16_t cycle = -1;
        void (*tick)(Game &game, int16_t stat_id);
        void (*touch)(Game &game, int16_t x, int16_t y, int16_t source_stat_id, int16_t &delta_x, int16_t &delta_y);
        int16_t editor_category = 0;
        char editor_shortcut = '\0';
        sstring<20> name = "";
        sstring<20> category_name = "";
        sstring<20> p1_name = "";
        sstring<20> p2_name = "";
        sstring<20> param_bullet_type_name = "";
        sstring<20> param_board_name = "";
        sstring<20> param_direction_name = "";
        sstring<20> param_text_name = "";
        int16_t score_value = 0;
    };
    
    struct MessageFlags {
        bool AmmoNotShown;
		bool OutOfAmmoNotShown;
		bool NoShootingNotShown;
		bool TorchNotShown;
		bool OutOfTorchesNotShown;
		bool RoomNotDarkNotShown;
		bool HintTorchNotShown;
		bool ForestNotShown;
		bool FakeNotShown;
		bool GemNotShown;
		bool EnergizerNotShown;
    };

    void ElementMove(Game &game, int16_t old_x, int16_t old_y, int16_t new_x, int16_t new_y);
    void ElementPushablePush(Game &game, int16_t x, int16_t y, int16_t delta_x, int16_t delta_y);

    class Game {
    private:
        // elements.cpp
        void InitElementDefs(void);

    public:
        Utils::Random random;

        int16_t playerDirX;
        int16_t playerDirY;

        Coord transitionTable[60 * 25];
        sstring<50> loadedGameFileName;
        sstring<50> savedGameFileName;
        sstring<50> savedBoardFileName;
        sstring<50> startupWorldFileName;

        Board board;
        World world;
        MessageFlags msgFlags;

        bool gameTitleExitRequested;
        bool gamePlayExitRequested;
        uint8_t gameStateElement;
        int16_t returnBoardId;

        int16_t transitionTableSize;
        uint8_t tickSpeed;

        const ElementDef *elementDefs;
        uint8_t elementCharOverrides[ElementCount];

        int16_t editorPatternCount;
        uint8_t editorPatterns[10];

        int16_t tickTimeDuration;
        int16_t currentTick;
        int16_t currentStatTicked;
        bool gamePaused;
        int16_t tickTimeCounter;

        bool forceDarknessOff;
        uint8_t initialTextAttr;

        char oopChar;
        sstring<20> oopWord;
        int16_t oopValue;

        bool debugEnabled;

        HighScoreList highScoreList;
        sstring<255> configRegistration;
        sstring<50> configWorldFile;
// TODO: expose as configure flag
#ifdef DISABLE_EDITOR
        constexpr static bool editorEnabled = false;
#else
        bool editorEnabled;
#endif
        sstring<50> gameVersion;
        bool parsingConfigFile;
        bool justStarted;

        Driver *driver;
        FilesystemDriver *filesystem;

        UserInterface *interface;
        BoardSerializer *boardSerializer;
        WorldSerializer *worldSerializer;

        inline const ElementDef& elementDefAt(int16_t x, int16_t y) const {
            uint8_t element = board.tiles.get(x, y).element;
            return elementDefs[element >= ElementCount ? EEmpty : element];
        }

        Game(void);
        ~Game();

        // elements.cpp
        void DrawPlayerSurroundings(int16_t x, int16_t y, int16_t bomb_phase);
        void GamePromptEndPlay(void);
        void ResetMessageNotShownFlags(void);
        void InitElementsGame(void);
        void InitElementsEditor(void);

        // game.cpp
        void SidebarClearLine(int y);
        void SidebarClear();
        void GenerateTransitionTable(void);
        void BoardClose();
        void BoardOpen(int16_t board_id);
        void BoardChange(int16_t board_id);
        void BoardCreate(void);
        void WorldCreate(void);
        void TransitionDrawToFill(uint8_t chr, uint8_t color);
        void BoardDrawTile(int16_t x, int16_t y);
        void BoardDrawBorder(void);
        void TransitionDrawToBoard(void);
        void SidebarPromptCharacter(bool editable, int16_t x, int16_t y, const char *prompt, uint8_t &value);
        void SidebarPromptSlider(bool editable, int16_t x, int16_t y,  const char *prompt, uint8_t &value);
        void SidebarPromptChoice(bool editable, int16_t y, const char *prompt, const char *choiceStr, uint8_t &result);
        void SidebarPromptDirection(bool editable, int16_t y, const char *prompt, int16_t &dx, int16_t &dy);
        void PauseOnError(void);
        void DisplayIOError(Utils::IOStream &stream);
        void WorldUnload(void);
        bool WorldLoad(const char *filename, const char *extension, bool titleOnly);
        bool WorldSave(const char *filename, const char *extension);
        void GameWorldSave(const char *prompt, char* filename, size_t filename_len, const char *extension);
        bool GameWorldLoad(const char *extension);
        void AddStat(int16_t x, int16_t y, uint8_t element, uint8_t color, int16_t cycle, Stat tpl);
        void RemoveStat(int16_t stat_id);
        bool BoardPrepareTileForPlacement(int16_t x, int16_t y);
        void MoveStat(int16_t stat_id, int16_t newX, int16_t newY);
        void GameDrawSidebar(void);
        void GameUpdateSidebar(void);
        void DisplayMessage(int16_t time, const char *message);
        void DamageStat(int16_t attacker_stat_id);
        void BoardDamageTile(int16_t x, int16_t y);
        void BoardAttack(int16_t attacker_stat_id, int16_t x, int16_t y);
        bool BoardShoot(uint8_t element, int16_t x, int16_t y, int16_t dx, int16_t dy, int16_t source);
        void CalcDirectionRnd(int16_t &deltaX, int16_t &deltaY);
        void CalcDirectionSeek(int16_t x, int16_t y, int16_t &deltaX, int16_t &deltaY);
        void TransitionDrawBoardChange(void);
        void BoardEnter(void);
        void BoardPassageTeleport(int16_t x, int16_t y);
        void GameDebugPrompt(void);
        void GameAboutScreen(void);
        void GamePlayLoop(bool boardChanged);
        void GameTitleLoop(void);

        // oop.cpp
        void OopError(Stat& stat, const char *message);
        void OopReadChar(Stat& stat, int16_t& position);
        void OopReadWord(Stat& stat, int16_t& position);
        void OopReadValue(Stat& stat, int16_t& position);
        void OopSkipLine(Stat& stat, int16_t& position);
        bool OopParseDirection(Stat& stat, int16_t& position, int16_t& dx, int16_t& dy);
        void OopReadDirection(Stat& stat, int16_t& position, int16_t& dx, int16_t& dy);
        int16_t OopFindString(Stat& stat, const char *str);
        bool OopIterateStat(int16_t stat_id, int16_t &i_stat, const char *lookup);
        bool OopFindLabel(int16_t stat_id, const char *sendLabel, int16_t &i_stat, int16_t &i_data_pos, const char *labelPrefix);
        int16_t WorldGetFlagPosition(const char *name);
        void WorldSetFlag(const char *name);
        void WorldClearFlag(const char *name);
        void OopStringToWord(const char *input, char *buf, size_t len);
        bool OopParseTile(Stat &stat, int16_t &position, Tile &tile);
        uint8_t GetColorForTileMatch(const Tile &tile);
        bool FindTileOnBoard(int16_t &x, int16_t &y, Tile tile);
        void OopPlaceTile(int16_t x, int16_t y, Tile tile);
        bool OopCheckCondition(Stat &stat, int16_t &position);
        void OopReadLineToEnd(Stat &stat, int16_t &position, char *buf, size_t len);
        bool OopSend(int16_t stat_id, const char *sendLabel, bool ignoreLock);
        void OopExecute(int16_t stat_id, int16_t &position, const char *name);

        // sounds.cpp
        bool HasTimeElapsed(int16_t &counter, int16_t duration);
    };

    // game.cpp
    void CopyStatDataToTextWindow(const Stat &stat, TextWindow &window);

    extern const char ColorNames[7][8];
    extern const int16_t DiagonalDeltaX[8];
    extern const int16_t DiagonalDeltaY[8];
    extern const int16_t NeighborDeltaX[4];
    extern const int16_t NeighborDeltaY[4];
    extern const uint8_t LineChars[16];

    extern const MenuEntry TitleMenu[];
    extern const MenuEntry PlayMenu[];
}

#endif
#include "gamevars.h"
#include "txtwind.h"

using namespace ZZT;
using namespace ZZT::Utils;

static const uint8_t TransporterNSChars[8] = {'^', '~', '^', '-', 'v', '_', 'v', '-'};
static const uint8_t TransporterEWChars[8] = {'(', '<', '(', 179, ')', '>', ')', 179};
static const uint8_t StarAnimChars[4] = {179, '/', 196, '\\'};

void ElementDefaultDraw(Game &game, int16_t x, int16_t y, uint8_t &chr) {
    chr = '?';
}

void ElementDefaultTick(Game &game, int16_t stat_id) {

}

void ElementDefaultTouch(Game &game, int16_t x, int16_t y, int16_t source_stat_id, int16_t &delta_x, int16_t &delta_y) {

}

void ElementMessageTimerTick(Game &game, int16_t stat_id) {
    char message[256];
    Stat &stat = game.board.stats[stat_id];
    if (stat.x == 0) {
        StrJoin(message, 3, " ", game.board.info.message, " ");
        game.driver->draw_string((60 - StrLength(game.board.info.message)) / 2, 24, 9 + (stat.p2 % 7), message);

        if (--stat.p2 <= 0) {
            game.RemoveStat(stat_id);
            game.currentStatTicked--;
            game.BoardDrawBorder();
            StrClear(game.board.info.message);
        }
    }
}

void ElementDamagingTouch(Game &game, int16_t x, int16_t y, int16_t source_stat_id, int16_t &delta_x, int16_t &delta_y) {
    game.BoardAttack(source_stat_id, x, y);
}

void ElementLionTick(Game &game, int16_t stat_id) {
    Stat &stat = game.board.stats[stat_id];
    int16_t dx, dy;

    if (stat.p1 < game.random.Next(10)) {
        game.CalcDirectionRnd(dx, dy);
    } else {
        game.CalcDirectionSeek(stat.x, stat.y, dx, dy);
    }

    Tile destTile = game.board.tiles.get(stat.x + dx, stat.y + dy);
    if (game.elementDefs[destTile.element].walkable) {
        game.MoveStat(stat_id, stat.x + dx, stat.y + dy);
    } else if (destTile.element == EPlayer) {
        game.BoardAttack(stat_id, stat.x + dx, stat.y + dy);
    }
}

void ElementTigerTick(Game &game, int16_t stat_id) {
    Stat &stat = game.board.stats[stat_id];
    Stat &player = game.board.stats[0];
    uint8_t element = (stat.p2 >= 0x80) ? EStar : EBullet;

    if ((game.random.Next(10) * 3) <= (stat.p2 & 0x7F)) {
        bool shot = false;

        if (Difference(stat.x, player.x) <= 2) {
            shot = game.BoardShoot(element, stat.x, stat.y, 0, Signum(player.y - stat.y), ShotSourceEnemy);
        }

        if (!shot && Difference(stat.y, player.y) <= 2) {
            shot = game.BoardShoot(element, stat.x, stat.y, Signum(player.x - stat.x), 0, ShotSourceEnemy);
        }
    }

    ElementLionTick(game, stat_id);
}

void ElementRuffianTick(Game &game, int16_t stat_id) {
    Stat &stat = game.board.stats[stat_id];
    Stat &player = game.board.stats[0];

    if (stat.step_x == 0 && stat.step_y == 0) {
        if ((stat.p2 + 8) <= (game.random.Next(17))) {
            if (stat.p1 >= game.random.Next(9)) {
                game.CalcDirectionSeek(stat.x, stat.y, stat.step_x, stat.step_y);
            } else {
                game.CalcDirectionRnd(stat.step_x, stat.step_y);
            }
        }
    } else {
        if (((stat.y == player.y) || (stat.x == player.x)) && game.random.Next(9) <= stat.p1) {
            game.CalcDirectionSeek(stat.x, stat.y, stat.step_x, stat.step_y);
        }

        Tile destTile = game.board.tiles.get(stat.x + stat.step_x, stat.y + stat.step_y);
        if (destTile.element == EPlayer) {
            game.BoardAttack(stat_id, stat.x + stat.step_x, stat.y + stat.step_y);
        } else if (game.elementDefs[destTile.element].walkable) {
            game.MoveStat(stat_id, stat.x + stat.step_x, stat.y + stat.step_y);
            if ((stat.p2 + 8) <= game.random.Next(17)) {
                stat.step_x = 0;
                stat.step_y = 0;
            }
        } else {
            stat.step_x = 0;
            stat.step_y = 0;
        }
    }
}

void ElementBearTick(Game &game, int16_t stat_id) {
    Stat &stat = game.board.stats[stat_id];
    Stat &player = game.board.stats[0];
    int16_t deltaX, deltaY;

    if (stat.x != player.x) {
        if (Difference(stat.y, player.y) <= (8 - stat.p1)) {
            deltaX = Signum(player.x - stat.x);
            deltaY = 0;
            goto Movement;
        }
    }

    if (Difference(stat.x, player.x) <= (8 - stat.p1)) {
        deltaY = Signum(player.y - stat.y);
        deltaX = 0;
    } else {
        deltaX = 0;
        deltaY = 0;
    }

Movement:
    Tile destTile = game.board.tiles.get(stat.x + deltaX, stat.y + deltaY);
    if (game.elementDefs[destTile.element].walkable) {
        game.MoveStat(stat_id, stat.x + deltaX, stat.y + deltaY);
    } else if (destTile.element == EPlayer || destTile.element == EBreakable) {
        game.BoardAttack(stat_id, stat.x + deltaX, stat.y + deltaY);        
    }
}

static inline bool centipede_head_find_follower(Game &game, Stat &stat, int16_t x, int16_t y) {
    if (game.board.tiles.get(x, y).element == ECentipedeSegment) {
        int16_t stat_id = game.board.stats.id_at(x, y);
        if (game.board.stats[stat_id].leader < 0) {
            stat.follower = stat_id;
        }
        return true;
    }
    return false;
}

void ElementCentipedeHeadTick(Game &game, int16_t stat_id) {
    Stat &stat = game.board.stats[stat_id];
    Stat &player = game.board.stats[0];
    int16_t ix, iy;
    int16_t tx, ty;
    int16_t tmp;

    if (stat.x == player.x && (game.random.Next(10) < stat.p1)) {
        stat.step_y = Signum(player.y - stat.y);
        stat.step_x = 0;
    } else if (stat.y == player.y && (game.random.Next(10) < stat.p1)) {
        stat.step_x = Signum(player.x - stat.x);
        stat.step_y = 0;
    } else if (((game.random.Next(10) * 4) < stat.p2) || (stat.step_x == 0 && stat.step_y == 0)) {
        game.CalcDirectionRnd(stat.step_x, stat.step_y);
    }

    {
        Tile destTile = game.board.tiles.get(stat.x + stat.step_x, stat.y + stat.step_y);
        if (!game.elementDefs[destTile.element].walkable && destTile.element != EPlayer) {
            ix = stat.step_x;
            iy = stat.step_y;

            tmp = (game.random.Next(2) * 2 - 1) * stat.step_y;
            stat.step_y = (game.random.Next(2) * 2 - 1) * stat.step_x;
            stat.step_x = tmp;

            destTile = game.board.tiles.get(stat.x + stat.step_x, stat.y + stat.step_y);
            if (!game.elementDefs[destTile.element].walkable && destTile.element != EPlayer) {
                stat.step_x = -stat.step_x;
                stat.step_y = -stat.step_y;

                destTile = game.board.tiles.get(stat.x + stat.step_x, stat.y + stat.step_y);
                if (!game.elementDefs[destTile.element].walkable && destTile.element != EPlayer) {
                    destTile = game.board.tiles.get(stat.x - ix, stat.y - iy);
                    if (!game.elementDefs[destTile.element].walkable && destTile.element != EPlayer) {
                        stat.step_x = 0;
                        stat.step_y = 0;
                    } else {
                        stat.step_x = -ix;
                        stat.step_y = -iy;
                    }
                }
            }
        }
    }

    if (stat.step_x == 0 && stat.step_y == 0) {
        // flip centipede
        game.board.tiles.set_element(stat.x, stat.y, ECentipedeSegment);        
        stat.leader = -1;
        while (game.board.stats[stat_id].follower > 0) {
            tmp = game.board.stats[stat_id].follower;
            game.board.stats[stat_id].follower = game.board.stats[stat_id].leader;
            game.board.stats[stat_id].leader = tmp;
            stat_id = tmp;
        }
        game.board.stats[stat_id].follower = game.board.stats[stat_id].leader;
        game.board.stats[stat_id].leader = -1; // OpenZoo: Tim's Super ZZT centipede fixes.
        game.board.tiles.set_element(game.board.stats[stat_id].x, game.board.stats[stat_id].y, ECentipedeHead);
    } else if (game.board.tiles.get(stat.x + stat.step_x, stat.y + stat.step_y).element == EPlayer) {
        // attack player
        if (stat.follower > 0) { // OpenZoo: Tim's Super ZZT centipede fixes.
            game.board.tiles.set_element(game.board.stats[stat.follower].x, game.board.stats[stat.follower].y, ECentipedeHead);
            game.board.stats[stat.follower].step_x = stat.step_x;
            game.board.stats[stat.follower].step_y = stat.step_y;
            game.BoardDrawTile(game.board.stats[stat.follower].x, game.board.stats[stat.follower].y);
        }
        game.BoardAttack(stat_id, stat.x + stat.step_x, stat.y + stat.step_y);
    } else {
        game.MoveStat(stat_id, stat.x + stat.step_x, stat.y + stat.step_y);

        do {
            Stat& it = game.board.stats[stat_id];
            tx = it.x - it.step_x;
            ty = it.y - it.step_y;
            ix = it.step_x;
            iy = it.step_y;

            if (it.follower < 0) {
                if (!centipede_head_find_follower(game, it, tx - ix, ty - iy)) {
                    if (!centipede_head_find_follower(game, it, tx - iy, ty - ix)) {
                        centipede_head_find_follower(game, it, tx + iy, ty + ix);
                    }
                }
            }

            if (it.follower > 0) {
                Stat& follower = game.board.stats[it.follower];
                // OpenZoo: Tim's Super ZZT centipede fixes.
                if (game.board.tiles.get(follower.x, follower.y).element == ECentipedeSegment) {
                    follower.leader = stat_id;
                    follower.p1 = it.p1;
                    follower.p2 = it.p2;
                    follower.step_x = tx - follower.x;
                    follower.step_y = ty - follower.y;
                    game.MoveStat(it.follower, tx, ty);
                }
            }

            stat_id = it.follower;
        } while (stat_id != -1);
    }
}

void ElementCentipedeSegmentTick(Game &game, int16_t stat_id) {
    Stat& stat = game.board.stats[stat_id];
    if (stat.leader < 0) {
        if (stat.leader < -1) {
            game.board.tiles.set_element(stat.x, stat.y, ECentipedeHead);
        } else {
            stat.leader--;
        }
    }
}

void ElementBulletTick(Game &game, int16_t stat_id) {
    Stat& stat = game.board.stats[stat_id];
    bool firstTry = true;

TryMove:
    int16_t ix = stat.x + stat.step_x;
    int16_t iy = stat.y + stat.step_y;
    const Tile &tile = game.board.tiles.get(ix, iy);
    const ElementDef &tileDef = game.elementDefs[tile.element];

    if (tileDef.walkable || (tile.element == EWater)) {
        game.MoveStat(stat_id, ix, iy);
        return;
    }

    if (tile.element == ERicochet && firstTry) {
        stat.step_x = -stat.step_x;
        stat.step_y = -stat.step_y;
        game.driver->sound_queue(1, "\xF9\x01");
        firstTry = false;
        goto TryMove;
    }

    if (tile.element == EBreakable
        || (tileDef.destructible && (tile.element == EPlayer || stat.p1 == ShotSourcePlayer)))
        {
            if (tileDef.score_value != 0) {
                game.world.info.score += tileDef.score_value;
                game.GameUpdateSidebar();
            }
            game.BoardAttack(stat_id, ix, iy);
            return;
        }

    if (game.board.tiles.get(stat.x + stat.step_y, stat.y + stat.step_x).element == ERicochet && firstTry) {
        int16_t tmp = stat.step_x;
        stat.step_x = -stat.step_y;
        stat.step_y = -tmp;
        game.driver->sound_queue(1, "\xF9\x01");
        firstTry = false;
        goto TryMove;
    }

    if (game.board.tiles.get(stat.x - stat.step_y, stat.y - stat.step_x).element == ERicochet && firstTry) {
        int16_t tmp = stat.step_x;
        stat.step_x = stat.step_y;
        stat.step_y = tmp;
        game.driver->sound_queue(1, "\xF9\x01");
        firstTry = false;
        goto TryMove;
    }

    game.RemoveStat(stat_id);
    game.currentStatTicked--;
    if (tile.element == EObject || tile.element == EScroll) {
        int16_t target_id = game.board.stats.id_at(ix, iy);
        game.OopSend(-target_id, "SHOT", false);
    }
}

void ElementSpinningGunDraw(Game &game, int16_t x, int16_t y, uint8_t &chr) {
    switch (game.currentTick & 7) {
        case 0: case 1: chr = 24; return;
        case 2: case 3: chr = 26; return;
        case 4: case 5: chr = 25; return;
        default: chr = 27; return;
    }
}

void ElementLineDraw(Game &game, int16_t x, int16_t y, uint8_t &chr) {
    int v = 0;
    int shift = 1;
    for (int i = 0; i < 4; i++) {
        switch (game.board.tiles.get(x + NeighborDeltaX[i], y + NeighborDeltaY[i]).element) {
            case ELine: case EBoardEdge:
                v += shift;
                break;
        }
        shift <<= 1;
    }
    chr = LineChars[v];
}

void ElementSpinningGunTick(Game &game, int16_t stat_id) {
    int16_t delta_x, delta_y;
    Stat& stat = game.board.stats[stat_id];
    game.BoardDrawTile(stat.x, stat.y);

    uint8_t element = (stat.p2 >= 0x80) ? EStar : EBullet;

    if (game.random.Next(9) < (stat.p2 & 0x7F)) {
        bool shot = false;

        if (game.random.Next(9) <= stat.p1) {
            Stat& player = game.board.stats[0];
            if (Difference(stat.x, player.x) <= 2) {
                shot = game.BoardShoot(element, stat.x, stat.y, 0, Signum(player.y - stat.y), ShotSourceEnemy);
            }

            if (!shot && (Difference(stat.y, player.y) <= 2)) {
                shot = game.BoardShoot(element, stat.x, stat.y, Signum(player.x - stat.x), 0, ShotSourceEnemy);                
            }
        } else {
            game.CalcDirectionRnd(delta_x, delta_y);
            shot = game.BoardShoot(element, stat.x, stat.y, delta_x, delta_y, ShotSourceEnemy);
        }
    }
}

void ElementConveyorTick(Game &game, int16_t x, int16_t y, int16_t direction) {
    Tile tiles[8];
    int16_t stat_ids[8];
    int16_t iMin = (direction == 1) ? 0 : 7;
    int16_t iMax = (direction == 1) ? 8 : -1;
    bool canMove = true;

    for (int16_t i = iMin; i != iMax; i += direction) {
        tiles[i] = game.board.tiles.get(x + DiagonalDeltaX[i], y + DiagonalDeltaY[i]);
        if (tiles[i].element == EEmpty) {
            canMove = true;
        } else if (!game.elementDefs[tiles[i].element].pushable) {
            canMove = false;
        }
        // OpenZoo: In some cases, a movement could cause two stats to briefly overlap each other.
        // Pre-copy stat IDs to prevent "stat swapping" from occuring, potentially involving the player.
        if (game.elementDefs[tiles[i].element].cycle > -1) {
            stat_ids[i] = game.board.stats.id_at(x + DiagonalDeltaX[i], y + DiagonalDeltaY[i]);
        }
    }

    for (int16_t i = iMin; i != iMax; i += direction) {
        Tile& tile = tiles[i];

        if (canMove) {
            if (game.elementDefs[tile.element].pushable) {
                int16_t ix = x + DiagonalDeltaX[(i - direction) & 7];
                int16_t iy = y + DiagonalDeltaY[(i - direction) & 7];
                if (game.elementDefs[tile.element].cycle > -1) {
                    Tile tmpTile = game.board.tiles.get(x + DiagonalDeltaX[i], y + DiagonalDeltaY[i]);
                    // OpenZoo: "stat swapping" fix, part 2.
                    int16_t iStat = stat_ids[i];
                    game.board.tiles.set(x + DiagonalDeltaX[i], y + DiagonalDeltaY[i], tiles[i]);
                    game.board.tiles.set_element(ix, iy, EEmpty);
                    game.MoveStat(iStat, ix, iy);
                    game.board.tiles.set(x + DiagonalDeltaX[i], y + DiagonalDeltaY[i], tmpTile);
                    // OpenZoo: Added missing BoardDrawTile() call.
                    game.BoardDrawTile(x + DiagonalDeltaX[i], y + DiagonalDeltaY[i]);
                } else {
                    game.board.tiles.set(ix, iy, tile);
                    game.BoardDrawTile(ix, iy);
                }
                if (!game.elementDefs[tiles[(i + direction) & 7].element].pushable) {
                    game.board.tiles.set_element(x + DiagonalDeltaX[i], y + DiagonalDeltaY[i], EEmpty);
                    game.BoardDrawTile(x + DiagonalDeltaX[i], y + DiagonalDeltaY[i]);
                }
            } else {
                canMove = false;
            }
        } else if (tile.element == EEmpty) {
            canMove = true;
        } else if (!game.elementDefs[tile.element].pushable) {
            canMove = false;
        }
    }
}

void ElementConveyorCWDraw(Game &game, int16_t x, int16_t y, uint8_t &chr) {
    switch ((game.currentTick / game.elementDefs[EConveyorCW].cycle) % 4) {
        case 0: chr = 179; return;
        case 1: chr = 47; return;
        case 2: chr = 196; return;
        default: chr = 92; return;
    }
}

void ElementConveyorCWTick(Game &game, int16_t stat_id) {
    Stat& stat = game.board.stats[stat_id];
    game.BoardDrawTile(stat.x, stat.y);
    ElementConveyorTick(game, stat.x, stat.y, 1);
}

void ElementConveyorCCWDraw(Game &game, int16_t x, int16_t y, uint8_t &chr) {
    switch ((game.currentTick / game.elementDefs[EConveyorCCW].cycle) % 4) {
        case 3: chr = 179; return;
        case 2: chr = 47; return;
        case 1: chr = 196; return;
        default: chr = 92; return;
    }
}

void ElementConveyorCCWTick(Game &game, int16_t stat_id) {
    Stat& stat = game.board.stats[stat_id];
    game.BoardDrawTile(stat.x, stat.y);
    ElementConveyorTick(game, stat.x, stat.y, -1);
}

void ElementBombDraw(Game &game, int16_t x, int16_t y, uint8_t &chr) {
    Stat& stat = game.board.stats.at(x, y);
    chr = (stat.p1 <= 1) ? 11 : (48 + stat.p1);
}

void ElementBombTick(Game &game, int16_t stat_id) {
    Stat &stat = game.board.stats[stat_id];
    if (stat.p1 > 0) {
        stat.p1--;
        game.BoardDrawTile(stat.x, stat.y);

        if (stat.p1 == 1) {
        	game.driver->sound_queue(1, "\x60\x01\x50\x01\x40\x01\x30\x01\x20\x01\x10\x01");
            game.DrawPlayerSurroundings(stat.x, stat.y, 1);
        } else if (stat.p1 == 0) {
            int16_t old_x = stat.x;
            int16_t old_y = stat.y;
            game.RemoveStat(stat_id);
            game.DrawPlayerSurroundings(old_x, old_y, 2);
        } else {
			game.driver->sound_queue(1, ((stat.p1 & 1) == 0) ? "\xF8\x01" : "\xF5\x01");
        }
    }
}

void ElementBombTouch(Game &game, int16_t x, int16_t y, int16_t source_stat_id, int16_t &delta_x, int16_t &delta_y) {
    Stat& stat = game.board.stats.at(x, y);
    if (stat.p1 == 0) {
        stat.p1 = 9;
        game.BoardDrawTile(x, y);
        game.DisplayMessage(200, "Bomb activated!");
        game.driver->sound_queue(4, "\x30\x01\x35\x01\x40\x01\x45\x01\x50\x01");
    } else {
        ElementPushablePush(game, x, y, delta_x, delta_y);
    }
}

void ElementTransporterMove(Game &game, int16_t x, int16_t y, int16_t dx, int16_t dy) {
    Stat& stat = game.board.stats.at(x + dx, y + dy);
    if (dx == stat.step_x && dy == stat.step_y) {
        int16_t ix = stat.x;
        int16_t iy = stat.y;
        int16_t new_x = -1;
        int16_t new_y = -1;
        bool finish_search = false;
        bool is_valid_dest = true;
        do {
            ix += dx;
            iy += dy;
            const Tile& tile = game.board.tiles.get(ix, iy);
            if (tile.element == EBoardEdge) {
                finish_search = true;
            } else if (is_valid_dest) {
                is_valid_dest = false;

                if (!game.elementDefs[tile.element].walkable) {
                    ElementPushablePush(game, ix, iy, dx, dy);
                }

                if (game.elementDefs[tile.element].walkable) {
                    finish_search = true;
                    new_x = ix;
                    new_y = iy;
                } else {
                    new_x = -1;
                }
            }
            if (tile.element == ETransporter) {
                Stat& transporter = game.board.stats.at(ix, iy);
                if (transporter.step_x == -dx && transporter.step_y == -dy) {
                    is_valid_dest = true;
                }
            }
        } while (!finish_search);
        if (new_x != -1) {
            ElementMove(game, stat.x - dx, stat.y - dy, new_x, new_y);
			game.driver->sound_queue(3, "\x30\x01\x42\x01\x34\x01\x46\x01\x38\x01\x4A\x01\x40\x01\x52\x01");
        }
    }
}

void ElementTransporterTouch(Game &game, int16_t x, int16_t y, int16_t source_stat_id, int16_t &delta_x, int16_t &delta_y) {
    ElementTransporterMove(game, x - delta_x, y - delta_y, delta_x, delta_y);
    delta_x = 0;
    delta_y = 0;
}

void ElementTransporterTick(Game &game, int16_t stat_id) {
    Stat& stat = game.board.stats[stat_id];
    game.BoardDrawTile(stat.x, stat.y);
}

void ElementTransporterDraw(Game &game, int16_t x, int16_t y, uint8_t &ch) {
    Stat &stat = game.board.stats.at(x, y);
    // OpenZoo: Prevent division by zero on 0-cycle transporters.
    int i = stat.cycle == 0 ? 0 : (game.currentTick / stat.cycle) % 4;
    if (stat.step_x == 0) {
        ch = TransporterNSChars[stat.step_y * 2 + 2 + i];
    } else {
        ch = TransporterEWChars[stat.step_x * 2 + 2 + i];
    }
}

void ElementStarDraw(Game &game, int16_t x, int16_t y, uint8_t &ch) {
    ch = StarAnimChars[game.currentTick & 3];
    const Tile& tile = game.board.tiles.get(x, y);
    game.board.tiles.set_color(x, y, (tile.color >= 15) ? 9 : (tile.color + 1));
}

void ElementStarTick(Game &game, int16_t stat_id) {
    Stat &stat = game.board.stats[stat_id];
    stat.p2--;
    if (stat.p2 <= 0) {
        game.RemoveStat(stat_id);
    } else if ((stat.p2 & 1) == 0) {
        game.CalcDirectionSeek(stat.x, stat.y, stat.step_x, stat.step_y);
        const Tile &target = game.board.tiles.get(stat.x + stat.step_x, stat.y + stat.step_y);
        if (target.element == EPlayer || target.element == EBreakable) {
            game.BoardAttack(stat_id, stat.x + stat.step_x, stat.y + stat.step_y);
        } else {
            if (!game.elementDefs[target.element].walkable) {
                ElementPushablePush(game, stat.x + stat.step_x, stat.y + stat.step_y, stat.step_x, stat.step_y);
            }

            if (game.elementDefs[target.element].walkable || target.element == EWater) {
                game.MoveStat(stat_id, stat.x + stat.step_x, stat.y + stat.step_y);
            }
        }
    } else {
        game.BoardDrawTile(stat.x, stat.y);
    }
}

void ElementEnergizerTouch(Game &game, int16_t x, int16_t y, int16_t source_stat_id, int16_t &delta_x, int16_t &delta_y) {
	game.driver->sound_queue(9,
		"\x20\x03\x23\x03\x24\x03\x25\x03\x35\x03\x25\x03\x23\x03\x20\x03"
		"\x30\x03\x23\x03\x24\x03\x25\x03\x35\x03\x25\x03\x23\x03\x20\x03"
		"\x30\x03\x23\x03\x24\x03\x25\x03\x35\x03\x25\x03\x23\x03\x20\x03"
		"\x30\x03\x23\x03\x24\x03\x25\x03\x35\x03\x25\x03\x23\x03\x20\x03"
		"\x30\x03\x23\x03\x24\x03\x25\x03\x35\x03\x25\x03\x23\x03\x20\x03"
		"\x30\x03\x23\x03\x24\x03\x25\x03\x35\x03\x25\x03\x23\x03\x20\x03"
		"\x30\x03\x23\x03\x24\x03\x25\x03\x35\x03\x25\x03\x23\x03\x20\x03");

    game.board.tiles.set_element(x, y, EEmpty);
    game.BoardDrawTile(x, y);

    game.world.info.energizer_ticks = 75;
    game.GameUpdateSidebar();

    if (game.msgFlags.EnergizerNotShown) {
        game.DisplayMessage(200, "Energizer - You are invincible");
        game.msgFlags.EnergizerNotShown = false;
    }

    game.OopSend(0, "ALL:ENERGIZE", false);
}

void ElementSlimeTick(Game &game, int16_t stat_id) {
    Stat &stat = game.board.stats[stat_id];
    if (stat.p1 < stat.p2) {
        stat.p1++;
    } else {
        uint8_t color = game.board.tiles.get(stat.x, stat.y).color;
        stat.p1 = 0;
        int16_t start_x = stat.x;
        int16_t start_y = stat.y;
        int16_t changed_tiles = 0;

        for (int16_t dir = 0; dir < 4; dir++) {
            if (game.elementDefAt(start_x + NeighborDeltaX[dir], start_y + NeighborDeltaY[dir]).walkable) {
                if (changed_tiles == 0) {
                    game.MoveStat(stat_id, start_x + NeighborDeltaX[dir], start_y + NeighborDeltaY[dir]);
                    game.board.tiles.set(start_x, start_y, {
                        .element = EBreakable,
                        .color = color
                    });
                    game.BoardDrawTile(start_x, start_y);
                } else {
                    game.AddStat(start_x + NeighborDeltaX[dir], start_y + NeighborDeltaY[dir],
                        ESlime, color, game.elementDefs[ESlime].cycle, Stat());
                    game.board.stats[game.board.stats.count].p2 = stat.p2;
                }
                changed_tiles++;
            }
        }

        if (changed_tiles == 0) {
            game.RemoveStat(stat_id);
            game.board.tiles.set(start_x, start_y, {
                .element = EBreakable,
                .color = color
            });
            game.BoardDrawTile(start_x, start_y);
        }
    }
}

void ElementSlimeTouch(Game &game, int16_t x, int16_t y, int16_t source_stat_id, int16_t &delta_x, int16_t &delta_y) {
    uint8_t color = game.board.tiles.get(x, y).color;
    game.DamageStat(game.board.stats.id_at(x, y));
    game.board.tiles.set(x, y, {
        .element = EBreakable,
        .color = color
    });
    game.BoardDrawTile(x, y);
	game.driver->sound_queue(2, "\x20\x01\x23\x01");
}

void ElementSharkTick(Game &game, int16_t stat_id) {
    Stat &stat = game.board.stats[stat_id];
    int16_t delta_x, delta_y;

    if (stat.p1 < game.random.Next(10)) {
        game.CalcDirectionRnd(delta_x, delta_y);
    } else {
        game.CalcDirectionSeek(stat.x, stat.y, delta_x, delta_y);
    }

    const Tile &target = game.board.tiles.get(stat.x + delta_x, stat.y + delta_y);
    if (target.element == EWater) {
        game.MoveStat(stat_id, stat.x + delta_x, stat.y + delta_y);
    } else if (target.element == EPlayer) {
        game.BoardAttack(stat_id, stat.x + delta_x, stat.y + delta_y);
    }
}

void ElementBlinkWallDraw(Game &game, int16_t x, int16_t y, uint8_t &ch) {
    ch = 206;
}

void ElementBlinkWallTick(Game &game, int16_t stat_id) {
    Stat &stat = game.board.stats[stat_id];
    if (stat.p3 == 0) {
        stat.p3 = stat.p1 + 1;
    }
    if (stat.p3 == 1) {
        int16_t ix = stat.x + stat.step_x;
        int16_t iy = stat.y + stat.step_y;
        uint8_t element = (stat.step_x != 0) ? EBlinkRayEw : EBlinkRayNs;
        uint8_t color = game.board.tiles.get(stat.x, stat.y).color;

        while (true) {
            const Tile &target = game.board.tiles.get(ix, iy);
            if (target.element != element || target.color != color) {
                break;
            }
            game.board.tiles.set_element(ix, iy, EEmpty);
            game.BoardDrawTile(ix, iy);
            ix += stat.step_x;
            iy += stat.step_y;
            stat.p3 = stat.p2 * 2 + 1;
        }

        if (ix == (stat.x + stat.step_x) && iy == (stat.y + stat.step_y)) {
            bool hit_boundary = false;
            do {
                const Tile &target = game.board.tiles.get(ix, iy);

                if (target.element != EEmpty && game.elementDefs[target.element].destructible) {
                    game.BoardDamageTile(ix, iy);
                }

                if (target.element == EPlayer) {
                    int16_t player_stat_id = game.board.stats.id_at(ix, iy);
                    if (stat.step_x != 0) {
                        if (game.board.tiles.get(ix, iy - 1).element == EEmpty) {
                            game.MoveStat(player_stat_id, ix, iy - 1);
                        } else if (game.board.tiles.get(ix, iy + 1).element == EEmpty) {
                            game.MoveStat(player_stat_id, ix, iy + 1);
                        }
                    } else {
                        if (game.board.tiles.get(ix + 1, iy).element == EEmpty) {
                            game.MoveStat(player_stat_id, ix + 1, iy - 1);
                        } else if (game.board.tiles.get(ix - 1, iy).element == EEmpty) {
                            game.MoveStat(player_stat_id, ix + 1, iy);
                        }
                    }

                    if (target.element == EPlayer) {
                        while (game.world.info.health > 0) {
                            game.DamageStat(player_stat_id);
                        }
                        hit_boundary = true;
                    }
                }

                if (target.element == EEmpty) {
                    game.board.tiles.set(ix, iy, {
                        .element = element,
                        .color = color
                    });
                    game.BoardDrawTile(ix, iy);
                } else {
                    hit_boundary = true;
                }

                ix += stat.step_x;
                iy += stat.step_y;
            } while (!hit_boundary);

            stat.p3 = stat.p2 * 2 + 1;
        }
    } else {
        stat.p3--;
    }
}

void ZZT::ElementMove(Game &game, int16_t old_x, int16_t old_y, int16_t new_x, int16_t new_y) {
    int16_t stat_id = game.board.stats.id_at(old_x, old_y);

    if (stat_id >= 0) {
        game.MoveStat(stat_id, new_x, new_y);
    } else {
        game.board.tiles.set(new_x, new_y, game.board.tiles.get(old_x, old_y));
        game.BoardDrawTile(new_x, new_y);
        game.board.tiles.set_element(old_x, old_y, EEmpty);
        game.BoardDrawTile(old_x, old_y);
    }
}

void ZZT::ElementPushablePush(Game &game, int16_t x, int16_t y, int16_t delta_x, int16_t delta_y) {
    const Tile &from = game.board.tiles.get(x, y);
    if ((from.element == ESliderNS && delta_x == 0)
        || (from.element == ESliderEW && delta_y == 0)
        || game.elementDefs[from.element].pushable)
    {
        const Tile &to = game.board.tiles.get(x + delta_x, y + delta_y);
        if (to.element == ETransporter) {
            ElementTransporterMove(game, x, y, delta_x, delta_y);
        } else if (to.element != EEmpty) {
            // OpenZoo: Fix crashes based on an element recursively pushing itself.
            if (delta_x != 0 || delta_y != 0) {
                ElementPushablePush(game, x + delta_x, y + delta_y, delta_x, delta_y);
            }
        }

        if (!game.elementDefs[to.element].walkable
            && game.elementDefs[to.element].destructible
            && to.element != EPlayer)
        {
            game.BoardDamageTile(x + delta_x, y + delta_y);
        }

        if (game.elementDefs[to.element].walkable) {
            ElementMove(game, x, y, x + delta_x, y + delta_y);
        }
    }
}

void ElementDuplicatorDraw(Game &game, int16_t x, int16_t y, uint8_t &ch) {
    Stat &stat = game.board.stats.at(x, y);
    switch (stat.p1) {
        case 1: ch = 250; return;
        case 2: ch = 249; return;
        case 3: ch = 248; return;
        case 4: ch = 111; return;
        case 5: ch = 79; return;
        default: ch = 250; return;
    }
}

void ElementObjectTick(Game &game, int16_t stat_id) {
    Stat &stat = game.board.stats[stat_id];
    if (stat.data_pos >= 0) {
        game.OopExecute(stat_id, stat.data_pos, "Interaction");
    }

    if (stat.step_x != 0 || stat.step_y != 0) {
        int16_t dest_x = stat.x + stat.step_x;
        int16_t dest_y = stat.y + stat.step_y;
        if (game.elementDefAt(dest_x, dest_y).walkable) {
            game.MoveStat(stat_id, dest_x, dest_y);
        } else {
            game.OopSend(-stat_id, "THUD", false);
        }
    }
}

void ElementObjectDraw(Game &game, int16_t x, int16_t y, uint8_t &ch) {
    ch = game.board.stats.at(x, y).p1;
}

void ElementObjectTouch(Game &game, int16_t x, int16_t y, int16_t source_stat_id, int16_t &delta_x, int16_t &delta_y) {
    int16_t stat_id = game.board.stats.id_at(x, y);
    game.OopSend(-stat_id, "TOUCH", false);
}

void ElementDuplicatorTick(Game &game, int16_t stat_id) {
    Stat &stat = game.board.stats[stat_id];
    if (stat.p1 <= 4) {
        stat.p1++;
        game.BoardDrawTile(stat.x, stat.y);
    } else {
        stat.p1 = 0;

        int16_t src_x = stat.x + stat.step_x;
        int16_t src_y = stat.y + stat.step_y;
        const Tile &src = game.board.tiles.get(src_x, src_y);
        int16_t dest_x = stat.x - stat.step_x;
        int16_t dest_y = stat.y - stat.step_y;
        const Tile &dest = game.board.tiles.get(dest_x, dest_y);

        if (dest.element == EPlayer) {
            game.elementDefs[src.element]
                .touch(game, src_x, src_y, 0, game.driver->deltaX, game.driver->deltaY);
        } else {
            if (dest.element != EEmpty) {
                ElementPushablePush(game, dest_x, dest_y, -stat.step_x, -stat.step_y);
            }

            if (dest.element == EEmpty) {
                int16_t source_stat_id = game.board.stats.id_at(src_x, src_y);
                if (source_stat_id > 0) {
                    if (game.board.stats.count < (MAX_STAT + 24)) {
                        Stat &source_stat = game.board.stats[source_stat_id];
                        game.AddStat(dest_x, dest_y, src.element, src.color, source_stat.cycle, source_stat);
                        game.BoardDrawTile(dest_x, dest_y);
                    }
                } else if (source_stat_id != 0) {
                    game.board.tiles.set(dest_x, dest_y, src);
                    game.BoardDrawTile(dest_x, dest_y);
                }
				game.driver->sound_queue(3, "\x30\x02\x32\x02\x34\x02\x35\x02\x37\x02");
			} else {
				game.driver->sound_queue(3, "\x18\x01\x16\x01");
			}
        }

        stat.p1 = 0;
        game.BoardDrawTile(stat.x, stat.y);
    }

    stat.cycle = (9 - stat.p2) * 3;
}

void ElementScrollTick(Game &game, int16_t stat_id) {
    Stat &stat = game.board.stats[stat_id];
    const Tile &tile = game.board.tiles.get(stat.x, stat.y);
    game.board.tiles.set_color(stat.x, stat.y, tile.color >= 15 ? 9 : (tile.color + 1));
    game.BoardDrawTile(stat.x, stat.y);
}

void ElementScrollTouch(Game &game, int16_t x, int16_t y, int16_t source_stat_id, int16_t &delta_x, int16_t &delta_y) {
    int16_t stat_id = game.board.stats.id_at(x, y);
    Stat &stat = game.board.stats[stat_id];
    uint8_t buf[128];

	game.driver->sound_queue(2, buf, SoundParse("c-c+d-d+e-e+f-f+g-g", buf, sizeof(buf) - 1));
    stat.data_pos = 0;
    game.OopExecute(stat_id, stat.data_pos, "Scroll");

    // OpenZoo: OopExecute may call RemoveStat before we do.
    stat_id = game.board.stats.id_at(x, y);
    if (stat_id != -1) {
        game.RemoveStat(stat_id);
    }
}

void ElementKeyTouch(Game &game, int16_t x, int16_t y, int16_t source_stat_id, int16_t &delta_x, int16_t &delta_y) {
    char message[65];
    int16_t key = (game.board.tiles.get(x, y).color) & 0x07; 
    uint8_t value = key == 0 ? (game.world.info.gems >> 8) : (game.world.info.keys[key - 1] ? 1 : 0);

    // OpenZoo: Explicit handling for Black Keys/Doors (portability).
    if (value != 0) {
        if (key == 0) StrCopy(message, "You already have a Black key?");
        else StrJoin(message, 3, "You already have a ", ColorNames[key - 1], " key!");

        game.DisplayMessage(200, message);
        game.driver->sound_queue(2, "\x30\x02\x20\x02");
    } else {
        if (key == 0) game.world.info.gems = (game.world.info.gems & 0xFF) | 0x100;
        else game.world.info.keys[key - 1] = true;

        game.board.tiles.set_element(x, y, EEmpty);
        game.GameUpdateSidebar();

        if (key == 0) StrCopy(message, "You now have the Black key?");
        else StrJoin(message, 3, "You now have the ", ColorNames[key - 1], " key.");

        game.DisplayMessage(200, message);
        game.driver->sound_queue(2, "\x40\x01\x44\x01\x47\x01\x40\x01\x44\x01\x47\x01\x40\x01\x44\x01\x47\x01\x50\x02");
    }
}

void ElementAmmoTouch(Game &game, int16_t x, int16_t y, int16_t source_stat_id, int16_t &delta_x, int16_t &delta_y) {
    game.world.info.ammo += 5;

    game.board.tiles.set_element(x, y, EEmpty);
    game.GameUpdateSidebar();
	game.driver->sound_queue(2, "\x30\x01\x31\x01\x32\x01");

    if (game.msgFlags.AmmoNotShown) {
        game.DisplayMessage(200, "Ammunition - 5 shots per container.");
        game.msgFlags.AmmoNotShown = false;
    }
}

void ElementGemTouch(Game &game, int16_t x, int16_t y, int16_t source_stat_id, int16_t &delta_x, int16_t &delta_y) {
    game.world.info.gems += 1;
    game.world.info.health += 1;
    game.world.info.score += 10;

    game.board.tiles.set_element(x, y, EEmpty);
    game.GameUpdateSidebar();
	game.driver->sound_queue(2, "\x40\x01\x37\x01\x34\x01\x30\x01");

    if (game.msgFlags.GemNotShown) {
        game.DisplayMessage(200, "Gems give you Health!");
        game.msgFlags.GemNotShown = false;
    }
}

void ElementPassageTouch(Game &game, int16_t x, int16_t y, int16_t source_stat_id, int16_t &delta_x, int16_t &delta_y) {
    game.BoardPassageTeleport(x, y);
    delta_x = 0;
    delta_y = 0;
}

void ElementDoorTouch(Game &game, int16_t x, int16_t y, int16_t source_stat_id, int16_t &delta_x, int16_t &delta_y) {
    char message[65];
    int16_t key = (game.board.tiles.get(x, y).color >> 4) & 0x07; 
    uint8_t value = key == 0 ? (game.world.info.gems >> 8) : (game.world.info.keys[key - 1] ? 1 : 0);

    // OpenZoo: Explicit handling for Black Keys/Doors (portability).
    if (value != 0) {
        game.board.tiles.set_element(x, y, EEmpty);
        game.BoardDrawTile(x, y);

        if (key == 0) game.world.info.gems &= 0xFF;
        else game.world.info.keys[key - 1] = false; 
        game.GameUpdateSidebar();

        if (key == 0) StrCopy(message, "The Black door is now open?");
        else StrJoin(message, 3, "The ", ColorNames[key - 1], " door is now open.");

        game.DisplayMessage(200, message);
        game.driver->sound_queue(3, "\x30\x01\x37\x01\x3B\x01\x30\x01\x37\x01\x3B\x01\x40\x04");
    } else {
        if (key == 0) StrCopy(message, "The Black door is locked?");
        else StrJoin(message, 3, "The ", ColorNames[key - 1], " door is locked!");

        game.DisplayMessage(200, message);
        game.driver->sound_queue(3, "\x17\x01\x10\x01");
    }
}

void ElementPushableTouch(Game &game, int16_t x, int16_t y, int16_t source_stat_id, int16_t &delta_x, int16_t &delta_y) {
    ElementPushablePush(game, x, y, delta_x, delta_y);
	game.driver->sound_queue(2, "\x15\x01");
}

void ElementPusherDraw(Game &game, int16_t x, int16_t y, uint8_t &chr) {
    Stat& stat = game.board.stats.at(x, y);
    if (stat.step_x == 1) {
        chr = 16;
    } else if (stat.step_x == -1) {
        chr = 17;
    }  else if (stat.step_y == -1) {
        chr = 30;
    } else {
        chr = 31;
    }
}

void ElementPusherTick(Game &game, int16_t stat_id) {
    int16_t start_x, start_y;

    {
        Stat& stat = game.board.stats[stat_id];
        start_x = stat.x;
        start_y = stat.y;

        if (!game.elementDefAt(stat.x + stat.step_x, stat.y + stat.step_y).walkable) {
            ElementPushablePush(game, stat.x + stat.step_x, stat.y + stat.step_y, stat.step_x, stat.step_y);        
        }
    }

    stat_id = game.board.stats.id_at(start_x, start_y);
    {
        Stat& stat = game.board.stats[stat_id];
        if (game.elementDefAt(stat.x + stat.step_x, stat.y + stat.step_y).walkable) {
            game.MoveStat(stat_id, stat.x + stat.step_x, stat.y + stat.step_y);
			game.driver->sound_queue(2, "\x15\x01");

            int16_t behind_x = stat.x - (stat.step_x * 2);
            int16_t behind_y = stat.y - (stat.step_y * 2);
            int16_t behind_id = game.board.stats.id_at(behind_x, behind_y);
            Stat& behind_stat = game.board.stats[stat_id];
            if (behind_stat.step_x == stat.step_x && behind_stat.step_y == stat.step_y) {
                game.elementDefs[EPusher].tick(game, behind_id);
            }
        }
    }
}

void ElementTorchTouch(Game &game, int16_t x, int16_t y, int16_t source_stat_id, int16_t &delta_x, int16_t &delta_y) {
    game.world.info.torches++;
    game.board.tiles.set_element(x, y, EEmpty);

    game.BoardDrawTile(x, y);
    game.GameUpdateSidebar();

    if (game.msgFlags.TorchNotShown) {
        game.DisplayMessage(200, "Torch - used for lighting in the underground.");
        game.msgFlags.TorchNotShown = false;
    }

	game.driver->sound_queue(3, "\x30\x01\x39\x01\x34\x02");
}

void ElementInvisibleTouch(Game &game, int16_t x, int16_t y, int16_t source_stat_id, int16_t &delta_x, int16_t &delta_y) {
    game.board.tiles.set_element(x, y, ENormal);
    game.BoardDrawTile(x, y);

	game.driver->sound_queue(3, "\x12\x01\x10\x01");
    game.DisplayMessage(100, "You are blocked by an invisible wall.");
}

void ElementForestTouch(Game &game, int16_t x, int16_t y, int16_t source_stat_id, int16_t &delta_x, int16_t &delta_y) {
    game.board.tiles.set_element(x, y, EEmpty);
    game.BoardDrawTile(x, y);

	game.driver->sound_queue(3, "\x39\x01");

    if (game.msgFlags.ForestNotShown) {
        game.DisplayMessage(200, "A path is cleared through the forest.");
        game.msgFlags.ForestNotShown = false;
    }
}

void ElementFakeTouch(Game &game, int16_t x, int16_t y, int16_t source_stat_id, int16_t &delta_x, int16_t &delta_y) {
    if (game.msgFlags.FakeNotShown) {
        game.DisplayMessage(150, "A fake wall - secret passage!");
        game.msgFlags.FakeNotShown = false;
    }
}

void ElementBoardEdgeTouch(Game &game, int16_t x, int16_t y, int16_t source_stat_id, int16_t &delta_x, int16_t &delta_y) {
    Stat& player = game.board.stats[0];
    int16_t entry_x = player.x;
    int16_t entry_y = player.y;
    int16_t neighbor_board_id;
    if (delta_y == -1) {
        neighbor_board_id = 0;
        entry_y = game.board.height();
    } else if (delta_y == 1) {
        neighbor_board_id = 1;
        entry_y = 1;   
    } else if (delta_x == -1) {
        neighbor_board_id = 2;
        entry_x = game.board.width();
    } else {
        neighbor_board_id = 3;
        entry_x = 1;
    }

    if (game.board.info.neighbor_boards[neighbor_board_id] != 0) {
        int16_t board_id = game.world.info.current_board;
        game.BoardChange(game.board.info.neighbor_boards[neighbor_board_id]);
        if (game.board.tiles.get(entry_x, entry_y).element != EPlayer) {
            game.elementDefAt(entry_x, entry_y).touch(game, entry_x, entry_y, source_stat_id, game.driver->deltaX, game.driver->deltaY);
        }

        const Tile &entryTile = game.board.tiles.get(entry_x, entry_y);
        if (game.elementDefs[entryTile.element].walkable || entryTile.element == EPlayer) {
            if (entryTile.element != EPlayer) {
                game.MoveStat(0, entry_x, entry_y);
            }

            game.TransitionDrawBoardChange();
            delta_x = 0;
            delta_y = 0;
            game.BoardEnter();
        } else {
            game.BoardChange(board_id);
        }
    }
}

void ElementWaterTouch(Game &game, int16_t x, int16_t y, int16_t source_stat_id, int16_t &delta_x, int16_t &delta_y) {
	game.driver->sound_queue(3, "\x40\x01\x50\x01");
    game.DisplayMessage(100, "Your way is blocked by water.");
}

void Game::DrawPlayerSurroundings(int16_t x, int16_t y, int16_t bomb_phase) {
    for (int ix = (x - TORCH_DX - 1); ix <= (x + TORCH_DX + 1); ix++) {
        if (ix < 1 || ix > board.width()) continue;
        for (int iy = (y - TORCH_DY - 1); iy <= (y + TORCH_DY + 1); iy++) {
            if (iy < 1 || iy > board.height()) continue;
            if (bomb_phase > 0 && (Sqr(ix - x) + Sqr(iy - y)*2) < TORCH_DIST_SQR) {
                const Tile &tile = board.tiles.get(ix, iy);
                if (bomb_phase == 1) {
                    if (!StrEmpty(elementDefs[tile.element].param_text_name)) {
                        int16_t i_stat = board.stats.id_at(ix, iy);
                        if (i_stat > 0) {
                            OopSend(i_stat, "BOMBED", false);
                        }
                    }

                    if (elementDefs[tile.element].destructible || tile.element == EStar) {
                        BoardDamageTile(ix, iy);
                    }

                    if (tile.element == EEmpty || tile.element == EBreakable) {
                        board.tiles.set(ix, iy, {
                            .element = EBreakable,
                            .color = (uint8_t) (9 + random.Next(7))
                        });
                        BoardDrawTile(ix, iy);
                    }
                } else {
                    if (tile.element == EBreakable) {
                        board.tiles.set_element(ix, iy, EEmpty);
                    }
                }
            }
            BoardDrawTile(ix, iy);
        }
    }
}

void Game::GamePromptEndPlay(void) {
    if (world.info.health <= 0) {
        gamePlayExitRequested = true;
        BoardDrawBorder();
    } else {
        gamePlayExitRequested = interface->SidebarPromptYesNo("End this game?", true);
        if (driver->keyPressed == KeyEscape) {
            gamePlayExitRequested = false;
        }
    }
    driver->keyPressed = 0;
}

void ElementPlayerTick(Game &game, int16_t stat_id) {
    Stat &stat = game.board.stats[stat_id];

    if (game.world.info.energizer_ticks > 0) {
        game.elementCharOverrides[EPlayer] = game.elementCharOverrides[EPlayer] == 0 ? 1 : 0;

        game.board.tiles.set_color(stat.x, stat.y,
            (game.currentTick & 2) != 0 ? 0x0F : ((((game.currentTick % 7) + 1) << 4) | 0x0F));
        game.BoardDrawTile(stat.x, stat.y);
    } else if (game.board.tiles.get(stat.x, stat.y).color != game.elementDefs[EPlayer].color || game.elementCharOverrides[EPlayer] != 0) {
        game.board.tiles.set_color(stat.x, stat.y, game.elementDefs[EPlayer].color);
        game.elementCharOverrides[EPlayer] = 0;
        game.BoardDrawTile(stat.x, stat.y);
    }

    if (game.world.info.health <= 0) {
        game.driver->deltaX = 0;
        game.driver->deltaY = 0;
        game.driver->shiftPressed = false;

        if (game.board.stats.id_at(0, 0) == -1) {
            game.DisplayMessage(32000, " Game over  -  Press ESCAPE");
        }

        game.tickTimeDuration = 0;
        game.driver->sound_set_block_queueing(true);
    }

    // OpenZoo: Only allow shift *with* arrows.
    bool shootDirPressed = game.driver->shiftPressed && (game.driver->deltaX != 0 || game.driver->deltaY != 0);
    bool shootNondirPressed = game.driver->keyPressed == ' ';
    if (shootDirPressed || shootNondirPressed) {
        // shooting logic
        if (shootDirPressed) {
            game.playerDirX = game.driver->deltaX;
            game.playerDirY = game.driver->deltaY;
        }

        if (game.playerDirX != 0 || game.playerDirY != 0) {
            if (game.board.info.max_shots == 0) {
                if (game.msgFlags.NoShootingNotShown) {
                    game.DisplayMessage(200, "Can't shoot in this place!");
                    game.msgFlags.NoShootingNotShown = false;
                }
            } else if (game.world.info.ammo == 0) {
                if (game.msgFlags.OutOfAmmoNotShown) {
                    game.DisplayMessage(200, "You don't have any ammo!");
                    game.msgFlags.OutOfAmmoNotShown = false;
                }
            } else {
                int16_t bulletCount = 0;
                for (int16_t i = 0; i <= game.board.stats.count; i++) {
                    const Stat& istat = game.board.stats[i];
                    const Tile& itile = game.board.tiles.get(istat.x, istat.y);
                    if (itile.element == EBullet && istat.p1 == ShotSourcePlayer) {
                        bulletCount++;
                    }
                }

                if (bulletCount < game.board.info.max_shots) {
                    if (game.BoardShoot(EBullet, stat.x, stat.y, game.playerDirX, game.playerDirY, ShotSourcePlayer)) {
                        game.world.info.ammo--;
                        game.GameUpdateSidebar();

						game.driver->sound_queue(2, "\x40\x01\x30\x01\x20\x01");

                        game.driver->deltaX = 0;
                        game.driver->deltaY = 0;
                    }
                }                
            }
        }
    } else if (game.driver->deltaX != 0 || game.driver->deltaY != 0) {
        // moving logic
        game.playerDirX = game.driver->deltaX;
        game.playerDirY = game.driver->deltaY;

        int16_t dest_x = stat.x + game.driver->deltaX;
        int16_t dest_y = stat.y + game.driver->deltaY;
        game.elementDefAt(dest_x, dest_y).touch(game, dest_x, dest_y, 0, game.driver->deltaX, game.driver->deltaY);
        if (game.driver->deltaX != 0 || game.driver->deltaY != 0) {
            // TODO [ZZT]: player walking sound
            if (game.elementDefAt(dest_x, dest_y).walkable) {
                game.MoveStat(0, dest_x, dest_y);
            }
        }
    }

    switch (game.interface->HandleMenu(game, PlayMenu, false)) {
        case 'T': {
            if (game.world.info.torch_ticks <= 0) {
                if (game.world.info.torches > 0) {
                    if (game.board.info.is_dark) {
                        game.world.info.torches--;
                        game.world.info.torch_ticks = TORCH_DURATION;

                        game.DrawPlayerSurroundings(stat.x, stat.y, 0);
                        game.GameUpdateSidebar();
                    } else {
                        if (game.msgFlags.RoomNotDarkNotShown) {
                            game.DisplayMessage(200, "Don't need torch - room is not dark!");
                            game.msgFlags.RoomNotDarkNotShown = false;
                        }
                    }
                } else {
                    if (game.msgFlags.OutOfTorchesNotShown) {
                        game.DisplayMessage(200, "You don't have any torches!");
                        game.msgFlags.OutOfTorchesNotShown = false;
                    }
                }
            }
        } break;
        case 'Q': {
            game.GamePromptEndPlay();
        } break;
        case 'S': {
            game.GameWorldSave("Save game:", game.savedGameFileName, sizeof(game.savedGameFileName), ".SAV");
        } break;
        case 'P': {
            if (game.world.info.health > 0) {
                game.gamePaused = true;
            }
        } break;
        case 'B': {
            game.driver->sound_set_enabled(!game.driver->sound_is_enabled());
            game.driver->sound_clear_queue();
            game.GameUpdateSidebar();
            game.driver->keyPressed = ' ';
        } break;
        case 'H': {
            TextWindowDisplayFile(game.driver, game.filesystem, "GAME.HLP", "Playing ZZT");
        } break;
        case '?': {
            game.GameDebugPrompt();
            game.driver->keyPressed = 0;
        } break;
    }

    if (game.world.info.torch_ticks > 0) {
        game.world.info.torch_ticks--;
        if (game.world.info.torch_ticks <= 0) {
            game.DrawPlayerSurroundings(stat.x, stat.y, 0);
			game.driver->sound_queue(3, "\x30\x01\x20\x01\x10\x01");
        }

        if (game.world.info.torch_ticks % 40 == 0) {
            game.GameUpdateSidebar();
        }
    }

    if (game.world.info.energizer_ticks > 0) {
        game.world.info.energizer_ticks--;
        if (game.world.info.energizer_ticks == 10) {
			game.driver->sound_queue(9, "\x20\x03\x1A\x03\x17\x03\x16\x03\x15\x03\x13\x03\x10\x03");
        } else if (game.world.info.energizer_ticks <= 0) {
            game.board.tiles.set_color(stat.x, stat.y, game.elementDefs[EPlayer].color);
            game.BoardDrawTile(stat.x, stat.y);
        }
    }

    if (game.board.info.time_limit_seconds > 0 && game.world.info.health > 0) {
        if (game.HasTimeElapsed(game.world.info.board_time_hsec, 100)) {
            game.world.info.board_time_sec++;

            if ((game.board.info.time_limit_seconds - 10) == (game.world.info.board_time_sec)) {
                game.DisplayMessage(200, "Running out of time!");
				game.driver->sound_queue(3, "\x40\x06\x45\x06\x40\x06\x35\x06\x40\x06\x45\x06\x40\x0A");
            } else if (game.world.info.board_time_sec > game.board.info.time_limit_seconds) {
                game.DamageStat(0);
            }

            game.GameUpdateSidebar();
        }
    }
}

void ElementMonitorTick(Game &game, int16_t stat_id) {
    if (game.interface->HandleMenu(game, TitleMenu, true) > 0) {
        game.gamePlayExitRequested = true;
    }
}

#include "element_defs.inc"

void Game::ResetMessageNotShownFlags(void) {
    memset(&msgFlags, true, sizeof(MessageFlags));
}

void Game::InitElementDefs(void) {
    this->elementDefs = defaultElementDefs;
    memset(elementCharOverrides, 0, sizeof(elementCharOverrides));
}

void Game::InitElementsEditor(void) {
    InitElementDefs();

    elementCharOverrides[EBoardEdge] = 'E';
    elementCharOverrides[EInvisible] = 176;
    forceDarknessOff = true;   
}

void Game::InitElementsGame(void) {
    InitElementDefs();
    forceDarknessOff = false;
}

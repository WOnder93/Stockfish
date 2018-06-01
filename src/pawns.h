/*
  Stockfish, a UCI chess playing engine derived from Glaurung 2.1
  Copyright (C) 2004-2008 Tord Romstad (Glaurung author)
  Copyright (C) 2008-2015 Marco Costalba, Joona Kiiski, Tord Romstad
  Copyright (C) 2015-2018 Marco Costalba, Joona Kiiski, Gary Linscott, Tord Romstad

  Stockfish is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  Stockfish is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef PAWNS_H_INCLUDED
#define PAWNS_H_INCLUDED

#include "misc.h"
#include "position.h"
#include "types.h"

namespace Pawns {

template<class T>
static inline T get_bits(T w, int pos, int len)
{
    return (w >> pos) & ((T(1) << len) - 1);
}

template<class T>
static inline void set_bits(T& w, int pos, int len, T val)
{
    w ^= ((val << pos) ^ w) & (((T(1) << len) - 1) << pos);
}

/// Pawns::Entry contains various information about a pawn structure. A lookup
/// to the pawn hash table (performed by calling the probe function) returns a
/// pointer to an Entry object.

template<Color Us>
struct ColorData {
    /*
     * w[0]:
     *  0-10 -- mg score
     * 11-21 -- eg score
     * 22-45 -- ~pawn attacks span & OutpostRanks
     * 46-49 -- pawns on dark squares
     * 50-53 -- weak unopposed count
     * 54-55 -- castling rights
     * 56-62 -- king square
     *
     * w[1]:
     *  0- 7 -- passed pawn mask
     *  8-15 -- semiopen files
     *
     * w[2]:
     *  0-47 -- pawn attacks
     * 48-60 -- king safety mg
     * 61-63 -- min king-pawn distance
     */
    uint64_t w[3];

    Score pawn_score() const {
        int mg = int(get_bits(w[0],  0, 11)) - (1 << 10);
        int eg = int(get_bits(w[0], 11, 11)) - (1 << 10);
        return make_score(mg, eg);
    }
    Bitboard pawn_attacks_outpost() const {
        return get_bits(w[0], 22, 24) << (Us == WHITE ? 16 : 24);
    }
    int pawns_on_dark_squares() const { return int(get_bits(w[0], 46,  4)); }
    int weak_unopposed()        const { return int(get_bits(w[0], 50,  4)); }
    int castling_rights()       const {
        return int(get_bits(w[0], 54,  2)) << (Us == WHITE ? 0 : 2);
    }
    Square king_square()        const { return Square(get_bits(w[0], 56, 7)); }

    Bitboard passed_mask()    const { return     get_bits(w[1], 0, 8); }
    Bitboard semiopen_files() const { return     get_bits(w[1], 8, 8); }
    int semiopen_file(File f) const { return int(get_bits(w[1], 8 + f, 1)); }

    Bitboard pawn_attacks() const {
        return get_bits(w[2], 0, 48) << (Us == WHITE ? 16 : 0);
    }
    Score king_safety(const Position& pos, Square ksq) {
        if (   king_square() == ksq
            && castling_rights() == pos.can_castle(Us)) {
            int mg =       int(get_bits(w[2], 48, 13)) - (1 << 12);
            int eg = -16 * int(get_bits(w[2], 61,  3));
            return make_score(mg, eg);
        }
        return do_king_safety(pos, ksq);
    }

    int pawns_on_same_color_squares(const Position& pos, Square s) const {
        int ds = pawns_on_dark_squares();
        return DarkSquares & s ? ds : pos.count<PAWN>(Us) - ds;
    }

    void init(const Position& pos);

    Score do_king_safety(const Position& pos, Square ksq);
};

struct Entry {
  template<Color Us>
  ColorData<Us>& data() { return std::get<Us == BLACK>(cd); }

  template<Color Us>
  const ColorData<Us>& data() const { return std::get<Us == BLACK>(cd); }

  int pawn_asymmetry() const { return get_bits(common, 0, 4); }
  int open_files()     const { return get_bits(common, 4, 4); }

  Key key;
  std::tuple<ColorData<WHITE>, ColorData<BLACK>> cd;
  uint8_t common;
};

typedef HashTable<Entry, 16384> Table;

void init();
Entry* probe(const Position& pos);

} // namespace Pawns

#endif // #ifndef PAWNS_H_INCLUDED

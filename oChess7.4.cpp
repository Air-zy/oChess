/*=====================================
Programmer: Airzy T
Assignment: FINAL PROJECT CHESS

Description(I.P.O):
    Input:
        - menu driven program
          enter letter inside [here] to select options

          // inside playing
          * chess is turn based and white always moves first
          example input: e2 e4
                      or e2 enter e4 - to see moves beforehand

    Process:
        // move generation
        - every possible move is calculated before hand and stores in big arrays
for fast access (caches)
        - convert inputs into a number corresponding to the tile number in the
8x8 chessboard ex. a1 --> 0  or  h8 --> 63
        - find and generate moves using input by looking it up on the caches

        // bot algorithm
        - uses the RECURSION algorithm knows as alpha beta
          alpha beta [plays the game ahead of time] to see which moves are
winning and loosing the bot will always play the best moves it can find

        - everything is optimized in the binary level storing moves and pieces
smaller than an integer


    Outputs:
        // move generation
        - displays all the legal moves as highlights in the output

        // board
        - top bar shows an evaluation bar determining the winning team
        - uses ascii characters of chess pieces

Assumptions:
    - user is expected to enter valid inputs

    - putting variables in public instead of using setter getter FOR A FASTER
LOOKUP
    - using ++i pre-increment insted of post-increment i++, its abit faster
    - minimized & risking more memmory for speed
    - needs at least 0.9 - 2 MB for storing pseudo-legal moves


SOURCES:
    helped alot for understanding bitwise operations
    https://bitwisecmd.com/
    https://www.rapidtables.com/convert/number/decimal-to-binary.html

    fen and to compare with
    https://lichess.org/analysis
    https://lichess.org/editor

    main sources
    https://www.chessprogramming.org/
    https://www.youtube.com/@chessprogramming591
    https://github.com/SebLague/Chess-Coding-Adventure

=======================================*/

/*
#include <WinNls.h>
#include <consoleapi2.h>
#include <processenv.h>
#include <stdlib.h>
#include <windows.h>  // for console visuals

#include <algorithm>  // for move sorting
#include <cctype>
#include <chrono>    // for timing
#include <cstdint>   // For UINT64_MAX and var types
#include <iostream>  //
#include <random>    // for random uint64 numbers
#include <string>    // string to int stoi(
*/

#include <iostream>
#include <random>
#include <string>

#include <chrono>
#include <windows.h>

void setTxtColor(int colorValue);
void printUint32Binary(uint32_t num);
void toLowercase(std::string &input);
void allowEmojis();

uint16_t ***allocateMoveHHistory();
void deallocateMoveHHistory(uint16_t ***moveHHistory);

std::string intToString(int num);
std::string invertFen(const std::string STR);

// instead of using boolean array
// 1ULL = 00000000 00000000 00000000 00000000 00000000 00000000 00000000
// 00000001
class BitBoard {
 private:
  uint64_t bitBoard = 0;  // 64 bits

 public:
  inline void setSquare(uint8_t square) {
    bitBoard |=
        (1ULL << square);  // set the bit corresponding to the index square
  };

  inline void unSetSquare(uint8_t square) {
    bitBoard &=
        ~(1ULL << square);  // clear the bit corresponding to the index square
  };

  inline bool isSet(uint8_t square) const {
    return (bitBoard >> square) &
           1;  // check if the bit corresponding to the square is set
  };

  inline void clearBoard() { bitBoard = 0; };  // clear the entire bitboard
  inline uint64_t get() const { return bitBoard; };  // get bb
  inline void set(uint64_t bb) { bitBoard = bb; };   // set bb
  inline bool isEmpty() const { return bitBoard == 0; };

  // Population count (Hamming weight) function
  inline int populationCount() const {
    uint64_t x = bitBoard;  // copy bitboard for manipulation
    x = x - ((x >> 1) & 0x5555555555555555ULL);  // step 1: divide and conquer
                                                 // to sum 2 bits at a time
    x = (x & 0x3333333333333333ULL) +
        ((x >> 2) & 0x3333333333333333ULL);      // step 2: Sum groups of 4 bits
    x = (x + (x >> 4)) & 0x0F0F0F0F0F0F0F0FULL;  // step 3: sum groups of 8 bits
    x = x + (x >> 8);   // step 4: Sum groups of 16 bits
    x = x + (x >> 16);  // step 5: Sum groups of 32 bits
    x = x + (x >> 32);  // step 6: Sum all bits in the 64-bit integer
    return x & 0x7F;    // return only the least significant 7 bits (to handle
                        // overflow)
  }

  inline int populationCountBAND(uint64_t x2) const {
    uint64_t x = (bitBoard & x2);  // copy bitboard and band for manip
    x = x - ((x >> 1) & 0x5555555555555555ULL);  // step 1: divide and conquer
                                                 // to sum 2 bits at a time
    x = (x & 0x3333333333333333ULL) +
        ((x >> 2) & 0x3333333333333333ULL);      // step 2: Sum groups of 4 bits
    x = (x + (x >> 4)) & 0x0F0F0F0F0F0F0F0FULL;  // step 3: sum groups of 8 bits
    x = x + (x >> 8);   // step 4: Sum groups of 16 bits
    x = x + (x >> 16);  // step 5: Sum groups of 32 bits
    x = x + (x >> 32);  // step 6: Sum all bits in the 64-bit integer
    return x & 0x7F;    // return only the least significant 7 bits (to handle
                        // overflow)
  }

  BitBoard(){};
  BitBoard(uint64_t &newbb) { bitBoard = newbb; };
};

// lookup class
class PieceCache {
 private:
  static constexpr uint8_t TYPE_MASK = 0b111;    // 7
  static constexpr uint8_t COLOR_MASK = 0b1000;  // 8

  // stored unicodes for displaying PIECES
  const std::string KING_UNICODE = "\xE2\x99\x9A";
  const std::string QUEEN_UNICODE = "\xE2\x99\x9B";
  const std::string ROOK_UNICODE = "\xE2\x99\x9C";
  const std::string BISHOP_UNICODE = "\xE2\x99\x9D";
  const std::string KNIGHT_UNICODE = "\xE2\x99\x9E";
  const std::string PAWN_UNICODE = "\xE2\x99\x99";
  const std::string EMPTY_STRING = " ";
  const std::string UNKNOWN_TYPE = "?";

 public:
  // last three bit represent piece type
  // first bit represents whtie/black
  // (value & 7) to get. Reason = 7 = 0111 piece type bitmask
  const uint8_t EMPTY = 0;   // 0000
  const uint8_t PAWN = 1;    // 0001
  const uint8_t KNIGHT = 2;  // 0010
  const uint8_t BISHOP = 3;  // 0011
  const uint8_t ROOK = 4;    // 0100
  const uint8_t QUEEN = 5;   // 0101
  const uint8_t KING = 6;    // 0110

  // colors
  // (value & 8) to get. Reason = 8 = 1000 color bitmask
  const uint8_t WHITE = 0;
  const uint8_t BLACK = 8;

  // pre defined PIECES
  const uint8_t WPAWN = PAWN | WHITE;      // 1
  const uint8_t WKNIGHT = KNIGHT | WHITE;  // 2
  const uint8_t WBISHOP = BISHOP | WHITE;  // 3
  const uint8_t WROOK = ROOK | WHITE;      // 4
  const uint8_t WQUEEN = QUEEN | WHITE;    // 5
  const uint8_t WKING = KING | WHITE;      // 6

  const uint8_t BPAWN = PAWN | BLACK;      // 9
  const uint8_t BKNIGHT = KNIGHT | BLACK;  // 10
  const uint8_t BBISHOP = BISHOP | BLACK;  // 11
  const uint8_t BROOK = ROOK | BLACK;      // 12
  const uint8_t BQUEEN = QUEEN | BLACK;    // 13
  const uint8_t BKING = KING | BLACK;      // 14

  inline uint8_t type(const uint8_t piece) const { return (piece & TYPE_MASK); }

  inline uint8_t color(const uint8_t piece) const {
    return (piece & COLOR_MASK);
  }

  const std::string toUnicode(const uint8_t pieceType) const {
    switch (pieceType) {
      case 0:
        return EMPTY_STRING;
      case 1:
        return PAWN_UNICODE;
      case 2:
        return KNIGHT_UNICODE;
      case 3:
        return BISHOP_UNICODE;
      case 4:
        return ROOK_UNICODE;
      case 5:
        return QUEEN_UNICODE;
      case 6:
        return KING_UNICODE;
      default:
        return UNKNOWN_TYPE;
    }
    return 0;
  }
};

// defining my lookup
const PieceCache PIECES;

// lookup struct
struct MoveCache {
  // Masks
  static constexpr unsigned short fromTileMask = 0b0000000000111111;
  static constexpr unsigned short toTileMask = 0b0000111111000000;
  static constexpr unsigned short flagMask = 0b1111000000000000;
  static constexpr unsigned short inverseFlagMask = 0b0000111111111111;

  // Flags
  static constexpr uint8_t NoFlag = 0b0000;                // 0
  static constexpr uint8_t EnPassantCaptureFlag = 0b0001;  // 1
  static constexpr uint8_t CastleFlag = 0b0010;            // 2
  static constexpr uint8_t PawnTwoUpFlag = 0b0011;         // 3

  static constexpr uint8_t PromoteToQueenFlag = 0b0100;   // 4
  static constexpr uint8_t PromoteToKnightFlag = 0b0101;  // 5
  static constexpr uint8_t PromoteToRookFlag = 0b0110;    // 6
  static constexpr uint8_t PromoteToBishopFlag = 0b0111;  // 7

  // other
  static constexpr unsigned short Null = 0b0000000000000000;
};

// Compact (16 bit) move representation to preserve memory during search.
// The format is as follows (ffffttttttssssss)
// Bits 0-5: start square index
// Bits 6-11: target square index
// Bits 12-15: flag (promotion type, ect)
class Move {
 private:
  // 16bit move value          ffffttttttssssss;
  unsigned short moveValue = 0;
  uint8_t flag() const { return moveValue >> 12; };

 public:
  inline uint8_t moveFrom() const {
    return moveValue & MoveCache::fromTileMask;
  };
  inline uint8_t moveTo() const {
    return (moveValue & MoveCache::toTileMask) >> 6;
  };

  inline bool isNull() const { return moveValue == MoveCache::Null; };
  inline bool isCastling() const { return flag() == MoveCache::CastleFlag; };
  inline bool isPawnTwoUp() const {
    return flag() == MoveCache::PawnTwoUpFlag;
  };
  inline bool isEnPassant() const {
    return flag() == MoveCache::EnPassantCaptureFlag;
  };
  inline bool promoteQueen() const {
    return flag() == MoveCache::PromoteToQueenFlag;
  };
  inline bool promoteRook() const {
    return flag() == MoveCache::PromoteToRookFlag;
  };
  inline bool promoteBishop() const {
    return flag() == MoveCache::PromoteToBishopFlag;
  };
  inline bool promoteKnight() const {
    return flag() == MoveCache::PromoteToKnightFlag;
  };
  inline bool isPromotion() const {
    uint8_t fg = flag();
    return (fg == MoveCache::PromoteToQueenFlag ||
            fg == MoveCache::PromoteToRookFlag ||
            fg == MoveCache::PromoteToBishopFlag ||
            fg == MoveCache::PromoteToKnightFlag);
  }
  inline bool isDoublePawnPush() const {
    return flag() == MoveCache::PawnTwoUpFlag;
  }
  inline void setFlag(uint8_t newFlag) {
    // Clear the existing flag bits
    moveValue &= MoveCache::inverseFlagMask;
    // Set the new flag bits
    moveValue |= (newFlag << 12);
  }
  inline void clearMove() { moveValue = MoveCache::Null; };

  Move() { moveValue = MoveCache::Null; };

  Move(uint8_t fromTile, uint8_t toTile) {
    moveValue = (fromTile | (toTile << 6));
  };

  Move(uint8_t fromTile, uint8_t toTile, uint8_t flag) {
    moveValue = (fromTile | toTile << 6 | flag << 12);
  }
};

// lookup cache for colors
struct globalColors {
  // visual colors
  const uint8_t hlightCol = 240;  // White background Black text
  uint8_t wBlackCol = 128;        // Gray background black letter
  uint8_t wWhiteCol = 143;        // Gray background white letter
  uint8_t bWhiteCol = 15;         // Black background white latter
  uint8_t bBlackCol = 8;          // Black background gray letter
  const uint8_t greyLetCol = 8;   // Black background gray letter

  // green previous move
  const uint8_t prevMBlackCol = 32;
  const uint8_t prevMWhiteCol = 47;

  // red check color
  const uint8_t checkBlackCol = 207;  // 64;
  const uint8_t checkWhiteCol = 192;  // 79;
};

globalColors chessColors;

// biggest lookup class
class PreComputedCache {
 private:
  uint8_t rowColValues[8][8];  // each element of [row][col] is a tile
 public:
  uint8_t preComputedRows[64];  // each element is set to which row its on
  uint8_t preComputedCols[64];  // each element is set to which col its on

  static constexpr int directionOffsets[8] = {
      // diagonals
      7,   // [0] up right
      9,   // [1] up left
      -9,  // [2] down right
      -7,  // [3] down left
      // orthogonals
      8,   // [4] up
      -8,  // [5] down
      -1,  // [6] right
      1,   // [7] left
  };

  // pre-compute pseudo legal moves for each tiles
  Move bPawnMoves[64][8];  // 2 max possible moves 2 diagonal takes + promotions
  Move wPawnMoves[64][8];  // 2 max possible moves 2 diagonal takes + promotions
  Move knightMoves[64][8];  // 8 max possible moves
  Move kingMoves[64][8];    // 8 max possible moves

  // directions
  Move bishopMoves[4][64]
                  [7];  // 13 max, 4 directions, 7 moves max in each direction
  Move rookMoves[4][64]
                [7];  // 14 max, 4 directions, 7 moves max in each direction

  // castling
  Move whiteKingSideCastle;
  Move whiteQueenSideCastle;
  Move blackKingSideCastle;
  Move blackQueenSideCastle;

  // returns bitboard ray between from tile to tile
  BitBoard rays[64][64];  // [fromtile][totile]
  BitBoard
      rays1Extra[64][64];  // [fromtile][totile] but 1 extra to preventKingMoves

  // distances between tiles
  unsigned int distances[64][64];  // [fromtile][totile]

  // normal rook tiles
  const uint8_t whiteKingRook = 7;
  const uint8_t whiteQueenRook = 0;
  const uint8_t blackKingRook = 63;
  const uint8_t blackQueenRook = 56;

  // castled rook tiles
  const uint8_t whiteKingRookCastleTo = 5;
  const uint8_t whiteQueenRookCastleTo = 3;
  const uint8_t blackKingRookCastleTo = 61;
  const uint8_t blackQueenRookCastleTo = 59;

  // castled king tiles
  const uint8_t whiteKingCastleTo = 6;
  const uint8_t whiteQueenCastleTo = 2;
  const uint8_t blackKingCastleTo = 62;
  const uint8_t blackQueenCastleTo = 58;

  // queen side vacant tiles - tiles in where it has to be empty to allow
  // castling
  const uint8_t whiteQueenCastleVacant = 1;
  const uint8_t blackQueenCastleVacant = 57;

  std::string startingFen =
      "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";
  std::string notatedTiles[64];  // lookup array for tile notations
  uint8_t notationToTile(std::string &notation) const {
    int fileNum =
        notation[0] - 'a';  // Convert file to a number between 0 and 7
    int rankNum =
        notation[1] - '1';  // Convert rank to a number between 0 and 7
    if (fileNum >= 0 && fileNum <= 7 && rankNum >= 0 && rankNum <= 7) {
      return rowColValues[rankNum][fileNum];
    } else {
      return 0;
    }
  }

  std::string tileToNotation(const uint8_t &tile) const {
    return notatedTiles[tile];
  }

  // piece values in hundy
  static constexpr int pawnValue = 100 * 10;
  static constexpr int knightValue = 300 * 10;
  static constexpr int bishopValue = 300 * 10;
  static constexpr int rookValue = 500 * 10;
  static constexpr int queenValue = 900 * 10;

  // in 1 Million
  static constexpr int evalPositiveInf = 1000000;
  static constexpr int evalNegativeInf = -1000000;

  // in 900K
  static constexpr int evalWhiteWins = 900000;
  static constexpr int evalWhiteLoss = -900000;

  // piece square tables
  int centerPST[64] = {
      -10, 0,  1,  2,  2,  1,  0,  -10,  //
      0,   10, 10, 15, 15, 10, 10, 0,    //
      0,   15, 30, 35, 35, 30, 15, 1,    //
      2,   25, 40, 50, 50, 40, 25, 2,    //
      2,   25, 40, 50, 50, 40, 25, 2,    //  for king endgame
      0,   15, 30, 35, 35, 30, 15, 1,    //
      0,   10, 10, 15, 15, 10, 10, 0,    //
      -10, 0,  1,  2,  2,  1,  0,  -10,  //
  };

  // for ordering moves
  int statOrdPST[64] = {
      0, 0, 1, 1,  1,  1, 0, 0,  //
      0, 1, 2, 2,  2,  2, 1, 0,  //
      1, 4, 9, 6,  6,  9, 4, 1,  //
      2, 8, 8, 10, 10, 8, 8, 2,  //
      2, 8, 8, 10, 10, 8, 8, 2,  //
      1, 4, 9, 6,  6,  9, 4, 1,  //
      0, 1, 2, 2,  2,  2, 1, 0,  //
      0, 0, 1, 1,  1,  1, 0, 0,  //
  };

  int pawnPST[2][64] = {
      0,   0,  0,   0,
      0,   0,  0,   0,  //
      90,  90, 90,  90,
      90,  90, 90,  90,  //
      10,  10, 20,  40,
      40,  20, 10,  10,  //
      0,   0,  20,  40,
      40,  0,  0,   0,  //
      -4,  -2, 20,  40,
      40,  -5, -8,  -8,  //
      -4,  0,  5,   -10,
      0,   -5, 5,   3,  //
      -5,  0,  -10, -30,
      -30, 0,  5,   2,  // push center pawns away from original position
      0,   0,  0,   0,
      0,   0,  0,   0,  //
  };

  int pawnEndPST[2][64] = {
      0,   0,   0,   0,   0,   0,   0,   0,    //
      400, 400, 400, 400, 400, 400, 400, 400,  // pawn near promotion
      80,  80,  80,  80,  80,  80,  80,  80,   //
      30,  30,  30,  20,  20,  30,  30,  30,   //
      20,  20,  20,  20,  20,  20,  20,  20,   //
      15,  15,  15,  15,  15,  15,  15,  15,   //
      0,   0,   0,   0,   0,   0,   0,   0,    //
      0,   0,   0,   0,   0,   0,   0,   0,    //
  };

  int horsePST[2][64] = {
      -90, -30, -30, -30, -30, -30, -30, -90,  // away from corner to center
      -40, -20, 15,  0,   0,   15,  -20, -40,  //
      -30, 0,   20,  20,  20,  20,  0,   -30,  //
      -30, 5,   15,  20,  20,  15,  5,   -30,  //
      -30, 0,   15,  20,  20,  15,  0,   -30,  //
      -30, 5,   20,  5,   5,   20,  5,   -30,  //
      -40, -20, 0,   10,  10,  0,   -20, -40,  //
      -90, -40, -30, -30, -30, -30, -40, -90,  // away from corner to center
  };

  int bishopPST[2][64] = {
      -30, -10, -10, -10, -10, -10, -10, -30,  //
      -10, 15,  0,   0,   0,   0,   15,  -10,  //
      -10, 0,   5,   10,  10,  5,   0,   -10,  //
      -10, 5,   10,  10,  10,  10,  5,   -10,  //
      -10, 5,   10,  10,  10,  10,  5,   -10,  //
      -10, 10,  10,  5,   5,   10,  10,  -10,  //
      -10, 15,  10,  0,   0,   10,  15,  -10,  //
      -30, -20, -20, -10, -10, -20, -20, -30,  //
  };
  int rookPST[2][64] = {
      -5, -5, 0,  0,  0,  0,  -5, -5,  //
      -5, 10, 10, 10, 10, 10, 10, -5,  // eat up pawns behind
      -5, 0,  0,  0,  0,  0,  0,  -5,  //
      -5, 0,  0,  0,  0,  0,  0,  -5,  //
      -5, 0,  0,  0,  0,  0,  0,  -5,  //
      0,  0,  0,  0,  0,  0,  0,  0,   //
      -5, 0,  5,  8,  8,  5,  0,  -5,  //
      -5, -5, -5, 6,  6,  5,  -5, -5,  //
  };

  int kingPST[2][64] = {
      -90, -90, -90, -90, -90, -90, -90, -90,  //
      -90, -90, -90, -90, -90, -90, -90, -90,  //
      -90, -90, -90, -90, -90, -90, -90, -90,  //
      -90, -90, -90, -90, -90, -90, -90, -90,  //
      -60, -60, -60, -60, -60, -60, -60, -60,  //
      -50, -50, -50, -40, -40, -40, -50, -50,  //
      -40, -30, -50, -40, -40, -50, -30, -40,  //
      -5,  5,   -10, -20, -20, -20, 5,   -5,   //
      -10, 7,   5,   -30, -10, -10, 5,   -16,  // early king safety
  };

  BitBoard checkerBB;
  inline bool colorOfSquare(int tile) const {
    return (checkerBB.get() >> tile) & 1;
  }

  inline int value(const uint8_t &piece) const {  // pseudo for move ordering
    uint8_t pieceType = PIECES.type(piece);
    if (pieceType == PIECES.PAWN) {
      return 10;  // simple pawn eval
    } else if (pieceType == PIECES.KNIGHT) {
      return 30;  // simple knight eval
    } else if (pieceType == PIECES.BISHOP) {
      return 32;  // simple bishop eval
    } else if (pieceType == PIECES.ROOK) {
      return 50;  // simple rook eval
    } else if (pieceType == PIECES.QUEEN) {
      return 90;  // simple queen eval
    }
    return 0;
  }

  // two colors // 6 types of PIECES
  uint64_t zobristLookup[64][2][6];
  uint64_t zobristCastling[16];
  uint64_t zobristEnPassant[9];  // 8 columns and 1 for no enpassant
  uint64_t blackTurnZobrist;

  // generates random uint 64number controlled by seed
  inline uint64_t RandUINT64(uint64_t seed) const {
    std::mt19937_64 gen(seed);  // Mersenne Twister algorithm
    return gen();
  }

  bool inBounds(int tile) const { return tile >= 0 && tile < 64; };

  PreComputedCache() {  // very expensive constructor lol
    int dfar = 0;
    // pre compute moves
    whiteKingSideCastle = Move(4, whiteKingCastleTo, MoveCache::CastleFlag);
    whiteQueenSideCastle = Move(4, whiteQueenCastleTo, MoveCache::CastleFlag);
    blackKingSideCastle = Move(60, blackKingCastleTo, MoveCache::CastleFlag);
    blackQueenSideCastle = Move(60, blackQueenCastleTo, MoveCache::CastleFlag);

    checkerBB.set(0x55aa55aa55aa55aa);  // 0xaa55aa55aa55aa55 is just a checker
                                        // board bitbaord all whites set
    // piece square tables

    uint64_t seedIncrement = 31;  // key
    for (int i = 0; i < 64; ++i) {
      int row = (i / 8);
      int col = (i % 8);
      int i2 = ((7 - row) * 8) + (col);
      pawnPST[1][i] = pawnPST[0][i2];
      horsePST[1][i] = horsePST[0][i2];
      bishopPST[1][i] = bishopPST[0][i2];
      kingPST[1][i] = kingPST[0][i2];
      rookPST[1][i] = rookPST[0][i2];
      pawnEndPST[1][i] = pawnEndPST[1][i2];

      // set the zobrist lookup
      for (int t = 0; t < 6; ++t) {
        ++seedIncrement;
        zobristLookup[i][0][t] = RandUINT64(seedIncrement);
        ++seedIncrement;
        zobristLookup[i][1][t] = RandUINT64(seedIncrement);
      }
    }
    // for castling
    for (int i = 0; i < 16; ++i) {
      ++seedIncrement;
      zobristCastling[i] = RandUINT64(seedIncrement);
    }
    // for en passant
    for (int f = 0; f < 9; ++f) {
      ++seedIncrement;
      zobristEnPassant[f] = RandUINT64(seedIncrement);
    }
    // for turn zobrist
    ++seedIncrement;
    blackTurnZobrist = RandUINT64(seedIncrement);

    for (int i = 0; i < 64; ++i) {
      int row = (i / 8);
      int col = (i % 8);
      rowColValues[row][col] = i;
      preComputedRows[i] = row;
      preComputedCols[i] = col;

      // for lookup notations
      std::string notation = "";
      notation += 'a' + col;  // Convert fileNum to letter
      notation += '1' + row;  // Convert rankNum to number
      notatedTiles[i] = notation;

      // white pawns
      if (row < 7) {
        if (row == 6) {
          wPawnMoves[i][4] = Move(i, i + 8, MoveCache::PromoteToQueenFlag);
          wPawnMoves[i][5] = Move(i, i + 8, MoveCache::PromoteToRookFlag);
          wPawnMoves[i][6] = Move(i, i + 8, MoveCache::PromoteToBishopFlag);
          wPawnMoves[i][7] = Move(i, i + 8, MoveCache::PromoteToKnightFlag);
          wPawnMoves[i][0] = Move(i, i + 8);
        } else {
          wPawnMoves[i][0] = Move(i, i + 8);
        }
        if (col < 7) {  // capture
          wPawnMoves[i][3] = Move(i, i + 9);
        }
        if (col > 0) {  // capture
          wPawnMoves[i][2] = Move(i, i + 7);
        }
        if (row == 1) {
          wPawnMoves[i][1] = Move(i, i + 16, MoveCache::PawnTwoUpFlag);
        }
      }

      // black pawns
      if (row > 0) {
        if (row == 1) {
          bPawnMoves[i][4] = Move(i, i - 8, MoveCache::PromoteToQueenFlag);
          bPawnMoves[i][5] = Move(i, i - 8, MoveCache::PromoteToRookFlag);
          bPawnMoves[i][6] = Move(i, i - 8, MoveCache::PromoteToBishopFlag);
          bPawnMoves[i][7] = Move(i, i - 8, MoveCache::PromoteToKnightFlag);
          bPawnMoves[i][0] = Move(i, i - 8);
        } else {
          bPawnMoves[i][0] = Move(i, i - 8);
        }
        if (col < 7) {  // capture
          bPawnMoves[i][3] = Move(i, i - 7);
        }
        if (col > 0) {  // capture
          bPawnMoves[i][2] = Move(i, i - 9);
        }
        if (row == 6) {
          bPawnMoves[i][1] = Move(i, i - 16, MoveCache::PawnTwoUpFlag);
        }
      }

      // knights

      // Knight move: 2 up, 1 left
      if (row < 6 && col > 0) {
        knightMoves[i][0] = Move(i, i + 15);
      }

      // Knight move: 2 up, 1 right
      if (row < 6 && col < 7) {
        knightMoves[i][1] = Move(i, i + 17);
      }

      // Knight move: 1 up, 2 left
      if (row < 7 && col > 1) {
        knightMoves[i][2] = Move(i, i + 6);
      }

      // Knight move: 1 up 2 right
      if (row < 7 && col < 6) {
        knightMoves[i][3] = Move(i, i + 10);
      }

      // Knight move: 1 down, 2 left
      if (row > 0 && col > 1) {
        knightMoves[i][4] = Move(i, i - 10);
      }

      // Knight move: 1 down, 2 right
      if (row > 0 && col < 6) {
        knightMoves[i][5] = Move(i, i - 6);
      }

      // Knight move: 2 down, 1 left
      if (row > 1 && col > 0) {
        knightMoves[i][6] = Move(i, i - 17);
      }

      // Knight move: 2 down, 1 right
      if (row > 1 && col < 7) {
        knightMoves[i][7] = Move(i, i - 15);
      }

      // bishops
      dfar = 0;
      if (col > 0) {
        for (int j = 1; j < 8; ++j) {  // up left col > 0
          int destination = i + (j * directionOffsets[0]);
          int dCol = destination % 8;
          if (inBounds(destination) && dCol >= 0) {
            bishopMoves[0][i][dfar] = Move(i, destination);
            ++dfar;
            if (dCol == 0) {
              break;
            }
          } else {
            break;
          }
        }
      }

      dfar = 0;
      if (col < 7) {
        for (int j = 1; j < 8; ++j) {  // up right col < 7
          int destination = i + (j * directionOffsets[1]);
          int dCol = destination % 8;
          if (inBounds(destination) && dCol <= 7) {
            bishopMoves[1][i][dfar] = Move(i, destination);
            ++dfar;
            if (dCol == 7) {
              break;
            }
          } else {
            break;
          }
        }
      }

      dfar = 0;
      if (col > 0) {
        for (int j = 1; j < 8; ++j) {  // down left col > 0
          int destination = i + (j * directionOffsets[2]);
          int dCol = destination % 8;
          if (inBounds(destination) && dCol >= 0) {
            bishopMoves[2][i][dfar] = Move(i, destination);
            ++dfar;
            if (dCol == 0) {
              break;
            }
          } else {
            break;
          }
        }
      }

      dfar = 0;
      if (col < 7) {
        for (int j = 1; j < 8; ++j) {  // down right col < 7
          int destination = i + (j * directionOffsets[3]);
          int dCol = destination % 8;
          if (inBounds(destination) && dCol <= 7) {
            bishopMoves[3][i][dfar] = Move(i, destination);
            ++dfar;
            if (dCol == 7) {
              break;
            }
          } else {
            break;
          }
        }
      }

      // rooks
      dfar = 0;
      for (int j = 1; j < 8; ++j) {  // up
        int destination = i + (j * directionOffsets[4]);
        if (inBounds(destination)) {
          rookMoves[0][i][dfar] = Move(i, destination);
          ++dfar;
        } else {
          break;
        }
      }
      dfar = 0;
      for (uint8_t j = 1; j < 8; ++j) {  // down
        int destination = i + (j * directionOffsets[5]);
        if (inBounds(destination)) {
          rookMoves[1][i][dfar] = Move(i, destination);
          ++dfar;
        } else {
          break;
        }
      }

      dfar = 0;
      if (col > 0) {
        for (uint8_t j = 1; j < 8; ++j) {  // left
          int destination = i + j * directionOffsets[6];
          int dCol = destination % 8;
          if (inBounds(destination) && dCol >= 0) {
            rookMoves[2][i][dfar] = Move(i, destination);
            ++dfar;
            if (dCol == 0) {  // reached end of left
              break;
            }
          } else {
            break;
          }
        }
      }

      dfar = 0;
      if (col < 7) {
        for (uint8_t j = 1; j < 8; ++j) {  // right
          int destination = i + j * directionOffsets[7];
          int dCol = destination % 8;
          if (inBounds(destination) && dCol <= 7) {
            rookMoves[3][i][dfar] = Move(i, destination);
            ++dfar;
            if (dCol == 7) {  // reached end of right
              break;
            }
          } else {
            break;
          }
        }
      }

      // king
      // kings

      // up
      if (row < 7) {
        kingMoves[i][0] = Move(i, i + 8);
      }

      // down
      if (row > 0) {
        kingMoves[i][1] = Move(i, i - 8);
      }

      // right
      if (col > 0) {
        kingMoves[i][2] = Move(i, i - 1);
      }

      // left
      if (col < 7) {
        kingMoves[i][3] = Move(i, i + 1);
      }

      // top right
      if (col > 0 && row < 7) {
        kingMoves[i][4] = Move(i, i + 7);
      }

      // top left
      if (col < 7 && row < 7) {
        kingMoves[i][5] = Move(i, i + 9);
      }

      // bottom right
      if (col > 0 && row > 0) {
        kingMoves[i][6] = Move(i, i - 9);
      }

      // bottom left
      if (col < 7 && row > 0) {
        kingMoves[i][7] = Move(i, i - 7);
      }
    }

    for (int i = 0; i < 64; ++i) {    // i is fromtile
      for (int j = 0; j < 64; ++j) {  // j is totile

        int rowI = preComputedRows[i];
        int colI = preComputedCols[i];
        int rowJ = preComputedRows[j];
        int colJ = preComputedCols[j];
        distances[i][j] =
            abs(rowI - rowJ) +
            abs(colI - colJ) * 2;  // distance of cols more significant cuz this
                                   // is used for king/pawn evaluation

        bool breakOut = false;
        BitBoard &bb = rays[i][j];
        BitBoard &bb2 = rays1Extra[i][j];
        bb.setSquare(i);
        // bb2.setSquare(i);

        // things between
        // orthos
        if (breakOut == false) {
          for (int d = 0; d < 4; ++d) {
            for (int r = 0; r < 7; ++r) {
              const Move addingMove = rookMoves[d][i][r];
              if (!addingMove.isNull()) {
                if (addingMove.moveTo() == j) {
                  breakOut = true;
                  if (r < 6 && !rookMoves[d][i][r + 1].isNull()) {
                    bb2.setSquare(rookMoves[d][i][r + 1].moveTo());
                  }
                  for (int r2 = r; r2 >= 0; --r2) {
                    bb2.setSquare(rookMoves[d][i][r2].moveTo());
                    bb.setSquare(rookMoves[d][i][r2].moveTo());
                  }
                  break;
                }
              }
            }
            if (breakOut) {
              break;
            }
          }
        }
        // diags
        if (breakOut == false) {
          for (int d = 0; d < 4; ++d) {
            for (int r = 0; r < 7; ++r) {
              const Move addingMove = bishopMoves[d][i][r];
              if (!addingMove.isNull()) {
                if (addingMove.moveTo() == j) {
                  breakOut = true;
                  if (r < 6 && !bishopMoves[d][i][r + 1].isNull()) {
                    bb2.setSquare(bishopMoves[d][i][r + 1].moveTo());
                  }
                  for (int r2 = r; r2 >= 0; --r2) {
                    bb2.setSquare(bishopMoves[d][i][r2].moveTo());
                    bb.setSquare(bishopMoves[d][i][r2].moveTo());
                  }
                  break;
                }
              }
            }
            if (breakOut) {
              break;
            }
          }
        }
        bb2.setSquare(j);
        bb.setSquare(j);
      }
    }
  };
};

// defining big lookup class
const PreComputedCache chessCache;  // uses atleast like 0.9 MB

struct movePair {
  int score = 0;
  Move move;
};

struct moveList {
  movePair moves[218];  // 218 max moves in chess
  uint8_t amt = 0;      // 255 limit

  void addConstMove(const Move &m) {
    if (!m.isNull()) {
      moves[amt].move = m;
      ++amt;
    }
  }
};

// using uint8_t since doesnt need to store more than 255
class pieceList {
 private:
  uint8_t pieceMap[64] = {0};  // to keep track of where PIECES are
 public:
  BitBoard pieceBBoard;      // temporary uneeded
  uint8_t PIECES[10] = {0};  // 10 max possible of the same piece in chess
  uint8_t amt = 0;

  inline void addAtTile(int tile) {
    PIECES[amt] = tile;
    pieceMap[tile] = amt;
    ++amt;
  };

  inline void removeAtTile(int tile) {
    int originalIndex = pieceMap[tile];
    PIECES[originalIndex] = PIECES[amt - 1];
    pieceMap[PIECES[amt - 1]] = originalIndex;

    if (amt > 0) {
      --amt;
    }
  };

  inline void MovePiece(int startSquare, int targetSquare) {
    PIECES[pieceMap[startSquare]] = targetSquare;
    pieceMap[targetSquare] = pieceMap[startSquare];
    // pieceBBoard.setSquare(targetSquare);
    // pieceBBoard.unSetSquare(startSquare);
  };

  inline void clear() {
    // pieceBBoard.clearBoard();
    for (uint8_t i = 0; i < amt; ++i) {
      pieceMap[PIECES[i]] = 0;
    }
    amt = 0;
  }

  pieceList() { clear(); };
};

// optimized stack for chess
class repetitionStack {
 private:
  static const int maxSize = 400;
  uint64_t repetitionHistory[maxSize] = {0};
  int top = 0;

  struct zkeyCount {
    uint64_t zKey = 0;
    uint8_t counter = 0;
  };

 public:
  void push(uint64_t zkey) { repetitionHistory[++top] = zkey; }

  void pop() { repetitionHistory[top--]; }

  bool isThreeFold() const {
    for (int i = 0; i < top; ++i) {
      uint64_t thisKey = repetitionHistory[i];
      uint8_t repCount = 0;
      for (int j = 0; j < i; ++j) {
        // uint64_t jKey = repetitionHistory[j];
        if (thisKey == repetitionHistory[j]) {
          ++repCount;
          if (repCount > 1) {
            return true;
          }
        }
      }
      // std::cout << thisKey << " : " << intToString(repCount) << '\n';
    }
    return false;
  }

  void clear() {
    for (int i = 0; i < top; ++i) {
      repetitionHistory[i] = 0;
    };
    top = 0;
  }
};
// optimized stack for chess
class gameStateStack {
 private:
  static const int maxSize = 400;
  uint32_t gameStateHistory[maxSize] = {0};  // Array to hold game states
  int top;

 public:
  gameStateStack() : top(-1) {}

  bool isEmpty() const { return top == -1; };

  bool isFull() const { return top == maxSize - 1; };

  void push(uint32_t gameState) {
    if (isFull()) {
      std::cout << "gameStack Overflow\n";
      system("PAUSE");
      return;
    }
    gameStateHistory[++top] = gameState;
  }

  uint32_t pop() {
    if (isEmpty()) {
      return 0;  // Returning 0 as error value
    }
    return gameStateHistory[top--];
  }

  uint32_t peek() const {
    if (isEmpty()) {
      return 0;  // Returning 0 as error value
    }
    return gameStateHistory[top];
  }

  void clear() {
    for (int i = 0; i < top; ++i) {
      gameStateHistory[i] = 0;
    };
  }
};

struct searchRes {  // search result
  Move m;           // best move
  int eval = 0;
  int nodes = 0;
  int depth = 0;
  int depthExtended = 0;
  uint8_t mateIn = 0;
};

// uglies structure possible
class Board {
 private:
  // constants
  const unsigned short whiteCastleKingsideMask = 0b1111111111111110;
  const unsigned short whiteCastleQueensideMask = 0b1111111111111101;
  const unsigned short blackCastleKingsideMask = 0b1111111111111011;
  const unsigned short blackCastleQueensideMask = 0b1111111111110111;

  const unsigned short whiteCastleKingsideBit = 0b0000000000000001;
  const unsigned short whiteCastleQueensideBit = 0b0000000000000010;
  const unsigned short blackCastleKingsideBit = 0b0000000000000100;
  const unsigned short blackCastleQueensideBit = 0b0000000000001000;

  const unsigned short castlingMASK = 0b1111;

  const unsigned short whiteCastleMask =
      whiteCastleKingsideMask & whiteCastleQueensideMask;
  const unsigned short blackCastleMask =
      blackCastleKingsideMask & blackCastleQueensideMask;

  // prev moves, only used for display
  Move prevMove;

  // index position of the kings
  uint8_t whiteKing = 0;
  uint8_t blackKing = 0;

  // Total plies (half-moves) played in game
  int plyCount = 0;
  int fiftyMoveCounter = 0;
  int oppTurn = PIECES.BLACK;  // opposite of turn
  int oppTurnIndex = 0;
  int turnIndex = 1;

  // stack like lists for faster move gen, one for black, one for white
  pieceList pawns[2];
  pieceList knights[2];
  pieceList bishops[2];
  pieceList rooks[2];
  pieceList queens[2];

  gameStateStack gameStateHistory;

  repetitionStack repHistory;

  uint32_t currentGameState = 0;
  inline bool whiteKingSideCastle() const {
    return (currentGameState & whiteCastleKingsideBit);
  };
  inline bool whiteQueenSideCastle() const {
    return (currentGameState & whiteCastleQueensideBit);
  };
  inline bool blackKingSideCastle() const {
    return (currentGameState & blackCastleKingsideBit);
  };
  inline bool blackQueenSideCastle() const {
    return (currentGameState & blackCastleQueensideBit);
  };

  // bitboards
  BitBoard allPIECES;
  BitBoard whiteAtks;
  BitBoard blackAtks;
  BitBoard checkRay;
  BitBoard blockRay;
  BitBoard pinMasks[2][8];  // pins possible for each direction
  bool pinExistInPosition = false;

  // board
  unsigned int board[64];

  inline void deleteTile(const uint8_t &index) {
    board[index] = PIECES.EMPTY;
    allPIECES.unSetSquare(index);
  }

  inline uint8_t getEnPassantFile() const {
    return (currentGameState >> 4) & 15;  // 0b1111
  }

  void resetValues() {
    checkRay.clearBoard();
    blockRay.clearBoard();
    allPIECES.clearBoard();
    blackAtks.clearBoard();
    whiteAtks.clearBoard();
    for (int d = 0; d < 8; ++d) {
      pinMasks[0][d].clearBoard();
      pinMasks[1][d].clearBoard();
    }
    currentGameState = 0;
    gameStateHistory.clear();  // clear history
    repHistory.clear();
    for (int i = 0; i < 64; ++i) {
      board[i] = PIECES.EMPTY;
    };
    allPIECES.clearBoard();
    for (int i = 0; i < 2; ++i) {
      pawns[i].clear();
      knights[i].clear();
      bishops[i].clear();
      rooks[i].clear();
      queens[i].clear();
    };
    whiteKing = 0;
    blackKing = 0;
    plyCount = 0;
    turn = PIECES.WHITE;
    oppTurnIndex = 0;
    turnIndex = 1;
    oppTurn = PIECES.BLACK;
  };

  // bools for more efficient access on checks
  bool boolWhiteCheck = false;
  bool boolBlackCheck = false;
  bool genQuiets = true;
  unsigned int checkCount = 0;
  uint64_t zobristKey = 0;

 public:
  int turn = PIECES.WHITE;

  // for move ordering
  // int moveHHistory[2][64][64] = {0};  // heuristic history
  uint16_t ***moveHHistory = allocateMoveHHistory();  // 3d pointer array

  void getpinMasks(BitBoard pmasks[2][8]) const {
    for (int d = 0; d < 8; ++d) {
      pmasks[0][d] = pinMasks[0][d];
      pmasks[1][d] = pinMasks[1][d];
    }
  }

  BitBoard getcray() const { return checkRay; };
  BitBoard getAllPIECES() const { return allPIECES; };
  BitBoard getWhiteAtks() const { return whiteAtks; };
  BitBoard getBlackAtks() const { return blackAtks; };
  pieceList getWPawns() const { return pawns[1]; };
  pieceList getBPawns() const { return pawns[0]; };
  pieceList getWRooks() const { return rooks[1]; };
  pieceList getBRooks() const { return rooks[0]; };

  uint8_t getWhiteKing() const { return whiteKing; };
  uint8_t getBlackKing() const { return blackKing; };
  inline bool whiteInCheck() const { return blackAtks.isSet(whiteKing); }
  inline bool blackInCheck() const { return whiteAtks.isSet(blackKing); }

  bool inCheck() const { return (checkCount != 0); }

  // makes sure PIECES lists and bitboards are in sync with int board
  bool isSynced() {  // returns if board matches bitboards and lists
    int bpawnCount = 0;
    int wpawnCount = 0;
    for (int i = 0; i < 64; ++i) {
      if (board[i] == PIECES.BPAWN) {
        ++bpawnCount;
      } else if (board[i] == PIECES.WPAWN) {
        ++wpawnCount;
      }
      if (board[i] == PIECES.EMPTY && allPIECES.isSet(i)) {  // set but empty ?!
        std::cout << "\nunsync allPIECES1[" << i << "]\n";
        return false;
      } else if (board[i] != PIECES.EMPTY &&
                 !allPIECES.isSet(i)) {  // unset but not empty??
        std::cout << "\nunsync allPIECES2[" << i << "]\n";
        return false;
      }
    }
    for (int i = 0; i < rooks[0].amt; ++i) {
      if (board[rooks[0].PIECES[i]] != PIECES.BROOK) {
        std::cout << "\nunsync list, brook[" << intToString(rooks[0].PIECES[i])
                  << "]\n";
        return false;
      }
    }
    for (int i = 0; i < rooks[1].amt; ++i) {
      if (board[rooks[1].PIECES[i]] != PIECES.WROOK) {
        std::cout << "\nunsync list, wrook[" << intToString(rooks[1].PIECES[i])
                  << "]\n";
        return false;
      }
    }
    //
    if (wpawnCount != pawns[1].amt) {
      std::cout << "\nunsync count wpawns\n";
      return false;
    }
    if (bpawnCount != pawns[0].amt) {
      std::cout << "\nunsync count bpawns\n";
      return false;
    }

    for (int i = 0; i < pawns[0].amt; ++i) {
      if (board[pawns[0].PIECES[i]] != PIECES.BPAWN) {
        std::cout << "\nunsync list, bpawn[" << intToString(pawns[0].PIECES[i])
                  << "]\n";
        return false;
      }
    }
    for (int i = 0; i < pawns[1].amt; ++i) {
      if (board[pawns[1].PIECES[i]] != PIECES.WPAWN) {
        std::cout << "\nunsync list, wpawn[" << intToString(pawns[1].PIECES[i])
                  << "]\n";
        return false;
      }
    }
    return true;
  };

  void orderMoves(moveList &moves) {
    for (int i = 0; i < moves.amt; ++i) {
      movePair &cMPair = moves.moves[i];
      const Move &cMove = cMPair.move;
      const uint8_t &moveTo = cMove.moveTo();
      const uint8_t &moveFrom = cMove.moveFrom();
      const unsigned int &captured = board[moveTo];
      const int &myValue = chessCache.value(board[moveFrom]);

      int turnI = turnIndex;
      // cMPair.score += moveHHistory[turnI][moveFrom][moveTo];

      cMPair.score = chessCache.statOrdPST[moveTo];
      if (turn == PIECES.WHITE) {
        if (blackAtks.isSet(moveTo)) {  // if moves to enemy guarded square
          cMPair.score = -myValue;      // order captures higher
          if (captured != PIECES.EMPTY) {
            cMPair.score +=
                chessCache.value(captured) + 100;  // prioratize captures first
          }
        } else {  // moves to safe/unguarded square
          cMPair.score += chessCache.value(captured);
        }
        if (captured == PIECES.EMPTY) {
          cMPair.score += moveHHistory[1][moveFrom][moveTo];
        }
      } else {
        if (whiteAtks.isSet(moveTo)) {  // if moves to enemy guarded square
          cMPair.score = -myValue;      // order captures higher
          if (captured != PIECES.EMPTY) {
            cMPair.score +=
                chessCache.value(captured) + 100;  // prioratize captures first
          }
        } else {  // moves to safe/unguarded square
          cMPair.score += chessCache.value(captured);
        }
        if (captured == PIECES.EMPTY) {
          cMPair.score -= moveHHistory[0][moveFrom][moveTo];
        }
      }

      if (cMove.isPromotion()) {
        cMPair.score += 1000;
      }
    }
    std::sort(moves.moves, moves.moves + moves.amt,
              [](const movePair &a, const movePair &b) {  // lambda
                return a.score > b.score;  // Sort in descending order of score
              });
  }

  std::string notateMove(Move &m) const {
    if (m.isNull()) {
      return "null";
    }
    int piece = board[m.moveFrom()];
    int captured = board[m.moveTo()];
    if (piece == PIECES.EMPTY) {
      piece = captured;
    }
    std::string moveNotation = PIECES.toUnicode(PIECES.type(piece)) + " ";
    if (captured != PIECES.EMPTY &&
        PIECES.color(captured) != PIECES.color(piece)) {
      moveNotation = moveNotation + chessCache.tileToNotation(m.moveFrom());
      moveNotation = moveNotation + 'x';
    } else {
      if (PIECES.type(piece) != PIECES.PAWN &&
          PIECES.type(piece) != PIECES.KING) {
        moveNotation = moveNotation + chessCache.tileToNotation(m.moveFrom());
      }
    }
    moveNotation = moveNotation + chessCache.tileToNotation(m.moveTo());
    return moveNotation;
  }

  void setupFen(std::string fullFen) {
    resetValues();
    std::string unflippedFen = "";
    std::string fenTurn = "";
    std::string castlingFen = "";
    std::string enPassantTargetSQR = "";
    std::string halfMoveClockFen = "";  // 50 move counter
    std::string fullMoveClockFen = "";
    int spaceCount = 0;
    for (char c : fullFen) {
      if (c == ' ') {
        spaceCount++;
        continue;
      }

      if (spaceCount == 0) {
        unflippedFen += c;
      } else if (spaceCount == 1) {
        fenTurn += tolower(c);
      } else if (spaceCount == 2) {
        castlingFen += c;
      } else if (spaceCount == 3) {
        enPassantTargetSQR += c;
      } else if (spaceCount == 4) {
        halfMoveClockFen += c;
      } else if (spaceCount == 5) {
        fullMoveClockFen += c;
      }
    }

    if (fenTurn == "w") {
      turn = PIECES.WHITE;
      oppTurnIndex = 0;
      turnIndex = 1;
    } else if (fenTurn == "b") {
      turn = PIECES.BLACK;
      oppTurnIndex = 1;
      turnIndex = 0;
    }

    if (enPassantTargetSQR != "-" && enPassantTargetSQR.length() > 0) {
      uint8_t enPassSquare = chessCache.notationToTile(enPassantTargetSQR);
      int file =
          chessCache.preComputedCols[enPassSquare] + 1;  // +1 cuz 0 means none
      currentGameState |= (file << 4);
    }

    if (halfMoveClockFen != "-" && halfMoveClockFen.length() > 0) {
      fiftyMoveCounter = std::stoi(halfMoveClockFen);  // string to int
    }

    for (char c : castlingFen) {
      if (c == 'K') {
        currentGameState |= (1 << 0);
      } else if (c == 'Q') {
        currentGameState |= (1 << 1);
      } else if (c == 'k') {
        currentGameState |= (1 << 2);
      } else if (c == 'q') {
        currentGameState |= (1 << 3);
      }
    }
    gameStateHistory.push(currentGameState);
    repHistory.push(zobristKey);

    std::string fen = invertFen(unflippedFen);
    char index = 63;
    for (int i = 0; i < fen.length(); ++i) {
      char letter = fen[i];
      if (isdigit(letter)) {
        for (char j = 0; j < letter - '0'; ++j) {
          board[index] = PIECES.EMPTY;
          index--;
        }
      } else {
        switch (letter) {
          case 'p':
            pawns[0].addAtTile(index);
            board[index] = PIECES.BPAWN;
            allPIECES.setSquare(index);
            index--;
            break;
          case 'n':
            knights[0].addAtTile(index);
            board[index] = PIECES.BKNIGHT;
            allPIECES.setSquare(index);
            index--;
            break;
          case 'b':
            bishops[0].addAtTile(index);
            board[index] = PIECES.BBISHOP;
            allPIECES.setSquare(index);
            index--;
            break;
          case 'r':
            rooks[0].addAtTile(index);
            board[index] = PIECES.BROOK;
            allPIECES.setSquare(index);
            index--;
            break;
          case 'q':
            queens[0].addAtTile(index);
            board[index] = PIECES.BQUEEN;
            allPIECES.setSquare(index);
            index--;
            break;
          case 'k':
            blackKing = index;
            board[index] = PIECES.BKING;
            allPIECES.setSquare(index);
            index--;
            break;
          case 'P':
            pawns[1].addAtTile(index);
            board[index] = PIECES.WPAWN;
            allPIECES.setSquare(index);
            index--;
            break;
          case 'N':
            knights[1].addAtTile(index);
            board[index] = PIECES.WKNIGHT;
            allPIECES.setSquare(index);
            index--;
            break;
          case 'B':
            bishops[1].addAtTile(index);
            board[index] = PIECES.WBISHOP;
            allPIECES.setSquare(index);
            index--;
            break;
          case 'R':
            rooks[1].addAtTile(index);
            board[index] = PIECES.WROOK;
            allPIECES.setSquare(index);
            index--;
            break;
          case 'Q':
            queens[1].addAtTile(index);
            board[index] = PIECES.WQUEEN;
            allPIECES.setSquare(index);
            index--;
            break;
          case 'K':
            whiteKing = index;
            board[index] = PIECES.WKING;
            allPIECES.setSquare(index);
            index--;
            break;
        }
      }
    }

    // setup atk moves
    generatePseudoLegals();
  };

  void display(bool whiteSide, BitBoard &highlights, std::string line1 = " ",
               std::string line2 = " ", std::string line3 = " ",
               std::string line4 = " ") {
    for (int i = 0; i < 64; ++i) {
      int i2 = i;
      int row = chessCache.preComputedRows[i2];
      int col = chessCache.preComputedCols[i2];
      if (whiteSide) {
        i2 = ((7 - row) * 8) + (col);
      }
      int row2 = chessCache.preComputedRows[i2];
      if (chessCache.preComputedCols[i] == 0) {
        setTxtColor(chessColors.greyLetCol);
        std::cout << (row2 + 1) << '|';
      }

      if (highlights.isSet(i2)) {
        setTxtColor(chessColors.hlightCol);
      } else {
        int pieceColor = PIECES.color(board[i2]) == 0 ? 0 : 1;
        if (chessCache.colorOfSquare(i2)) {
          setTxtColor(pieceColor == 0 ? chessColors.wWhiteCol
                                      : chessColors.wBlackCol);
        } else {
          setTxtColor(pieceColor == 0 ? chessColors.bWhiteCol
                                      : chessColors.bBlackCol);
        };

        if (!prevMove.isNull() &&
            (prevMove.moveFrom() == i2 || prevMove.moveTo() == i2)) {
          if (PIECES.color(board[prevMove.moveTo()]) == PIECES.WHITE) {
            setTxtColor(chessColors.prevMWhiteCol);
          } else {
            setTxtColor(chessColors.prevMBlackCol);
          }
        }

        if (checkCount != 0) {
          if (i2 == whiteKing && checkRay.isSet(i2)) {
            setTxtColor(chessColors.checkBlackCol);
          }
          if (i2 == blackKing && checkRay.isSet(i2)) {
            setTxtColor(chessColors.checkWhiteCol);
          }
        }

        if (blockRay.isSet(i2)) {
          if (PIECES.color(board[i2]) == PIECES.WHITE) {
            setTxtColor(chessColors.checkBlackCol);
          } else {
            setTxtColor(chessColors.checkWhiteCol);
          }
        }
      }
      std::cout << PIECES.toUnicode(PIECES.type(board[i2])) << " ";
      if ((i + 1) % 8 == 0) {
        setTxtColor(chessColors.greyLetCol);
        if (chessCache.preComputedRows[i] == 0) {
          std::cout << "  ply: " << plyCount;
          std::cout << ", ";
          if (turn) {  // if blacks turn
            std::cout << " turn: b,  ";
          } else {
            std::cout << " turn: w,  ";
          }
          std::cout << (whiteKingSideCastle() ? "K" : "-");
          std::cout << (whiteQueenSideCastle() ? "Q" : "-");
          std::cout << (blackKingSideCastle() ? "k" : "-");
          std::cout << (blackQueenSideCastle() ? "q" : "-");
          std::cout << ",  FiftyMoveCounter: " << fiftyMoveCounter;
          // std::cout << ",  ( ";
          // printUint32Binary(currentGameState);
          // std::cout << ")";
        } else if (chessCache.preComputedRows[i] == 1) {
          std::cout << line1;
        } else if (chessCache.preComputedRows[i] == 2) {
          std::cout << line2;
        } else if (chessCache.preComputedRows[i] == 3) {
          std::cout << line3;
        } else if (chessCache.preComputedRows[i] == 4) {
          std::cout << line4;
        } else if (chessCache.preComputedRows[i] == 6) {
          std::cout << "  zKey: " << zobristKey;
        } else if (chessCache.preComputedRows[i] == 7) {
          std::cout << "  hEval: " << heuristicEval();
        }
        setTxtColor(15);
        std::cout << '\n';
      }
    }
    setTxtColor(chessColors.greyLetCol);
    std::cout << "  a b c d e f g h\n";
    setTxtColor(15);
  };

  void makeTurn() {  // swaps turns
    oppTurn = turn;
    if (turn) {  // if blacks turn
      oppTurnIndex = 0;
      turnIndex = 1;
      turn = PIECES.WHITE;
    } else {
      oppTurnIndex = 1;
      turnIndex = 0;
      turn = PIECES.BLACK;
    }
  };

  bool isCapture(const Move &M) const {
    return allPIECES.isSet(M.moveTo());
    // return board[m.moveTo()];
    // return (board[m.moveTo()] != PIECES.EMPTY);
  };

  void addLegal(moveList &ML, const Move &M) const {
    if (genQuiets == false &&
        !isCapture(M)) {  // remove if not capture in captures only
      return;
    }
    // if empty capture or captures enemy
    const unsigned int mto = M.moveTo();
    if (!blockRay.isEmpty()) {
      if (!blockRay.isSet(mto)) {
        return;
      }
    }
    if (pinExistInPosition) {
      const unsigned int mFrom = M.moveFrom();
      for (int d = 0; d < 8; ++d) {
        if (pinMasks[oppTurnIndex][d].isSet(
                mFrom)) {  // if piece moving is pinned
          if (!pinMasks[oppTurnIndex][d].isSet(
                  mto)) {  // restricts movement to only the pinmask
            return;
          }
        };
      }
    }
    if (board[mto] == PIECES.EMPTY || turn != PIECES.color(board[mto])) {
      if (mto != whiteKing && mto != blackKing) {  // and not king captures
        ML.addConstMove(M);
      }
    }
  };

  void addPLegal(moveList &ML, const Move &m) const {
    if (genQuiets == false &&
        !isCapture(m)) {  // remove if not capture in captures only
      return;
    }

    const unsigned int mto = m.moveTo();
    if (board[mto] == PIECES.EMPTY || turn != PIECES.color(board[mto])) {
      if (mto != whiteKing && mto != blackKing) {  // and not king captures
        ML.addConstMove(m);
      }
    }
  };

  // manually written/hard coded for less looping, so its a bit faster
  void generatePawnMoves(moveList &m) const {
    if (turn == PIECES.WHITE) {
      for (int i = 0; i < pawns[1].amt; ++i) {
        int pieceIndex = pawns[1].PIECES[i];

        if (genQuiets) {
          const Move pawnPush = chessCache.wPawnMoves[pieceIndex][0];
          if (!pawnPush.isNull() && !allPIECES.isSet(pawnPush.moveTo())) {
            if (chessCache.preComputedRows[pieceIndex] == 6) {
              addLegal(m, chessCache.wPawnMoves[pieceIndex][4]);  // Queen
              addLegal(m, chessCache.wPawnMoves[pieceIndex][5]);  // Rook
              addLegal(m, chessCache.wPawnMoves[pieceIndex][6]);  // Bishop
              addLegal(m, chessCache.wPawnMoves[pieceIndex][7]);  // Knight
            } else {
              addLegal(m, pawnPush);  // Single Push
            }
            if (!allPIECES.isSet(
                    chessCache.wPawnMoves[pieceIndex][1].moveTo())) {
              addLegal(m, chessCache.wPawnMoves[pieceIndex][1]);  // Double Push
            }
          }
        }

        Move addingMove = chessCache.wPawnMoves[pieceIndex][2];
        uint8_t enPFile = getEnPassantFile();
        if (!addingMove.isNull()) {
          if (isCapture(addingMove)) {
            if (chessCache.preComputedRows[pieceIndex] == 6) {
              addingMove.setFlag(MoveCache::PromoteToQueenFlag);
              addLegal(m, addingMove);  // Promote
              addingMove.setFlag(MoveCache::PromoteToRookFlag);
              addLegal(m, addingMove);  // Promote
              addingMove.setFlag(MoveCache::PromoteToBishopFlag);
              addLegal(m, addingMove);  // Promote
              addingMove.setFlag(MoveCache::PromoteToKnightFlag);
              addLegal(m, addingMove);  // Promote
            } else {
              addLegal(m, addingMove);  // Diagonal Capture
            }
          } else if (enPFile != 0 &&
                     chessCache.preComputedCols[addingMove.moveTo()] ==
                         enPFile - 1 &&
                     chessCache.preComputedRows[addingMove.moveFrom()] ==
                         4  // en passant row
          ) {
            addingMove.setFlag(MoveCache::EnPassantCaptureFlag);
            m.addConstMove(addingMove);  // en passant capture
          }
        }
        Move addingMove2 = chessCache.wPawnMoves[pieceIndex][3];
        uint8_t enPFile2 = getEnPassantFile();
        if (!addingMove2.isNull()) {
          if (isCapture(addingMove2)) {
            if (chessCache.preComputedRows[pieceIndex] == 6) {
              addingMove2.setFlag(MoveCache::PromoteToQueenFlag);
              addLegal(m, addingMove2);  // Promote
              addingMove2.setFlag(MoveCache::PromoteToRookFlag);
              addLegal(m, addingMove2);  // Promote
              addingMove2.setFlag(MoveCache::PromoteToBishopFlag);
              addLegal(m, addingMove2);  // Promote
              addingMove2.setFlag(MoveCache::PromoteToKnightFlag);
              addLegal(m, addingMove2);  // Promote
            } else {
              addLegal(m, addingMove2);  // Diagonal Capture
            }
          } else if (enPFile2 != 0 &&
                     chessCache.preComputedCols[addingMove2.moveTo()] ==
                         enPFile2 - 1 &&
                     chessCache.preComputedRows[addingMove2.moveFrom()] ==
                         4  // en passant row
          ) {
            addingMove2.setFlag(MoveCache::EnPassantCaptureFlag);
            m.addConstMove(addingMove2);  // en passant capture
          }
        }
      }
    } else {  // if black turn
      for (int i = 0; i < pawns[0].amt; ++i) {
        int pieceIndex = pawns[0].PIECES[i];
        const Move pawnPush = chessCache.bPawnMoves[pieceIndex][0];
        if (!pawnPush.isNull() && !allPIECES.isSet(pawnPush.moveTo())) {
          if (chessCache.preComputedRows[pieceIndex] == 1) {
            addLegal(m, chessCache.bPawnMoves[pieceIndex][4]);  // Queen
            addLegal(m, chessCache.bPawnMoves[pieceIndex][5]);  // Rook
            addLegal(m, chessCache.bPawnMoves[pieceIndex][6]);  // Bishop
            addLegal(m, chessCache.bPawnMoves[pieceIndex][7]);  // Knight
          } else {
            addLegal(m, pawnPush);  // Single Push
          }
          if (!allPIECES.isSet(chessCache.bPawnMoves[pieceIndex][1].moveTo())) {
            addLegal(m, chessCache.bPawnMoves[pieceIndex][1]);
          }
        }
        Move addingMove = chessCache.bPawnMoves[pieceIndex][2];
        if (!addingMove.isNull()) {
          uint8_t enPFile = getEnPassantFile();
          if (isCapture(addingMove)) {
            if (chessCache.preComputedRows[pieceIndex] == 1) {
              addingMove.setFlag(MoveCache::PromoteToQueenFlag);
              addLegal(m, addingMove);  // Promote
              addingMove.setFlag(MoveCache::PromoteToRookFlag);
              addLegal(m, addingMove);  // Promote
              addingMove.setFlag(MoveCache::PromoteToBishopFlag);
              addLegal(m, addingMove);  // Promote
              addingMove.setFlag(MoveCache::PromoteToKnightFlag);
              addLegal(m, addingMove);  // Promote
            } else {
              addLegal(m, addingMove);  // Diagonal Capture
            }
          } else if (enPFile != 0 &&
                     chessCache.preComputedCols[addingMove.moveTo()] ==
                         enPFile - 1 &&
                     chessCache.preComputedRows[addingMove.moveFrom()] == 3) {
            addingMove.setFlag(MoveCache::EnPassantCaptureFlag);
            m.addConstMove(addingMove);  // en passant capture
          }
        }
        Move addingMove2 = chessCache.bPawnMoves[pieceIndex][3];
        if (!addingMove2.isNull()) {
          uint8_t enPFile2 = getEnPassantFile();
          if (isCapture(addingMove2)) {
            if (chessCache.preComputedRows[pieceIndex] == 1) {
              addingMove2.setFlag(MoveCache::PromoteToQueenFlag);
              addLegal(m, addingMove2);  // Promote
              addingMove2.setFlag(MoveCache::PromoteToRookFlag);
              addLegal(m, addingMove2);  // Promote
              addingMove2.setFlag(MoveCache::PromoteToBishopFlag);
              addLegal(m, addingMove2);  // Promote
              addingMove2.setFlag(MoveCache::PromoteToKnightFlag);
              addLegal(m, addingMove2);  // Promote
            } else {
              addLegal(m, addingMove2);  // Diagonal Capture
            }
          } else if (enPFile2 != 0 &&
                     chessCache.preComputedCols[addingMove2.moveTo()] ==
                         enPFile2 - 1 &&
                     chessCache.preComputedRows[addingMove2.moveFrom()] == 3) {
            addingMove2.setFlag(MoveCache::EnPassantCaptureFlag);
            m.addConstMove(addingMove2);  // en passant capture
          }
        }
      }
    }
  };

  // manually written/hard coded for less looping, so its a bit faster
  void generateKnightMoves(moveList &m) const {
    if (turn == PIECES.WHITE) {
      for (int i = 0; i < knights[1].amt; ++i) {
        for (int j = 0; j < 8; ++j) {
          addLegal(m, chessCache.knightMoves[knights[1].PIECES[i]][j]);
        }
      }
    } else {
      for (int i = 0; i < knights[0].amt; ++i) {
        for (int j = 0; j < 8; ++j) {
          addLegal(m, chessCache.knightMoves[knights[0].PIECES[i]][j]);
        }
      }
    }
  };

  // manually written/hard coded for less looping, so its a bit faster
  void generateBishopMoves(moveList &m) const {
    if (turn == PIECES.WHITE) {
      for (int i = 0; i < bishops[1].amt; ++i) {
        // for each direction
        for (int d = 0; d < 4; ++d) {
          for (int r = 0; r < 7; ++r) {
            const Move addingMove =
                chessCache.bishopMoves[d][bishops[1].PIECES[i]][r];
            addLegal(m, addingMove);
            if (allPIECES.isSet(addingMove.moveTo())) {
              break;
            }
          }
        }
      }

      for (int i = 0; i < queens[1].amt; ++i) {
        // for each direction
        for (int d = 0; d < 4; ++d) {
          for (int r = 0; r < 7; ++r) {
            const Move addingMove =
                chessCache.bishopMoves[d][queens[1].PIECES[i]][r];
            addLegal(m, addingMove);
            if (allPIECES.isSet(addingMove.moveTo())) {
              break;
            }
          }
        }
      }
    } else {
      for (int i = 0; i < bishops[0].amt; ++i) {
        // for each direction
        for (int d = 0; d < 4; ++d) {
          for (int r = 0; r < 7; ++r) {
            const Move addingMove =
                chessCache.bishopMoves[d][bishops[0].PIECES[i]][r];
            addLegal(m, addingMove);
            if (allPIECES.isSet(addingMove.moveTo())) {
              break;
            }
          }
        }
      }

      for (int i = 0; i < queens[0].amt; ++i) {
        // for each direction
        for (int d = 0; d < 4; ++d) {
          for (int r = 0; r < 7; ++r) {
            const Move addingMove =
                chessCache.bishopMoves[d][queens[0].PIECES[i]][r];
            addLegal(m, addingMove);
            if (allPIECES.isSet(addingMove.moveTo())) {
              break;
            }
          }
        }
      }
    }
  };

  // manually written/hard coded for less looping, so its a bit faster
  void generateRookMoves(moveList &m) const {
    if (turn == PIECES.WHITE) {
      for (int i = 0; i < rooks[1].amt; ++i) {
        // for each direction
        for (int d = 0; d < 4; ++d) {
          for (int r = 0; r < 7; ++r) {
            const Move addingMove =
                chessCache.rookMoves[d][rooks[1].PIECES[i]][r];
            addLegal(m, addingMove);
            if (allPIECES.isSet(addingMove.moveTo())) {
              break;
            }
          }
        }
      }

      for (int i = 0; i < queens[1].amt; ++i) {
        // for each direction
        for (int d = 0; d < 4; ++d) {
          for (int r = 0; r < 7; ++r) {
            const Move addingMove =
                chessCache.rookMoves[d][queens[1].PIECES[i]][r];
            addLegal(m, addingMove);
            if (allPIECES.isSet(addingMove.moveTo())) {
              break;
            }
          }
        }
      }
    } else {
      for (int i = 0; i < rooks[0].amt; ++i) {
        // for each direction
        for (int d = 0; d < 4; ++d) {
          for (int r = 0; r < 7; ++r) {
            const Move addingMove =
                chessCache.rookMoves[d][rooks[0].PIECES[i]][r];
            addLegal(m, addingMove);
            if (allPIECES.isSet(addingMove.moveTo())) {
              break;
            }
          }
        }
      }

      for (int i = 0; i < queens[0].amt; ++i) {
        // for each direction
        for (int d = 0; d < 4; ++d) {
          for (int r = 0; r < 7; ++r) {
            const Move addingMove =
                chessCache.rookMoves[d][queens[0].PIECES[i]][r];
            addLegal(m, addingMove);
            if (allPIECES.isSet(addingMove.moveTo())) {
              break;
            }
          }
        }
      }
    }
  };

  // manually written/hard coded for less looping, so its a bit faster
  void generateKingMoves(moveList &m) const {
    if (turn == PIECES.WHITE) {
      for (int j = 0; j < 8; ++j) {
        const Move addingMove = chessCache.kingMoves[whiteKing][j];
        if (!blackAtks.isSet(addingMove.moveTo()) &&
            !checkRay.isSet(addingMove.moveTo())) {
          addPLegal(m, addingMove);
        }
      }
      if (!whiteInCheck()) {  // if not in check allow castling
        if (whiteKingSideCastle()) {
          if (board[chessCache.whiteKingCastleTo] == PIECES.EMPTY &&
              board[chessCache.whiteKingRookCastleTo] == PIECES.EMPTY &&
              !blackAtks.isSet(chessCache.whiteKingCastleTo) &&
              !blackAtks.isSet(
                  chessCache.whiteKingRookCastleTo)) {  // and not attacked
            m.addConstMove(chessCache.whiteKingSideCastle);
          }
        }
        if (whiteQueenSideCastle()) {
          if (board[chessCache.whiteQueenCastleTo] == PIECES.EMPTY &&
              board[chessCache.whiteQueenRookCastleTo] == PIECES.EMPTY &&
              board[chessCache.whiteQueenCastleVacant] == PIECES.EMPTY &&
              !blackAtks.isSet(chessCache.whiteQueenCastleTo) &&
              !blackAtks.isSet(
                  chessCache.whiteQueenRookCastleTo)) {  // and not attacked
            m.addConstMove(chessCache.whiteQueenSideCastle);
          }
        }
      }
    } else {
      for (int j = 0; j < 8; ++j) {
        const Move addingMove = chessCache.kingMoves[blackKing][j];
        if (!whiteAtks.isSet(addingMove.moveTo()) &&
            !checkRay.isSet(addingMove.moveTo())) {
          addPLegal(m, addingMove);
        }
      }
      if (!blackInCheck()) {  // if not in check allow castling
        if (blackKingSideCastle()) {
          if (board[chessCache.blackKingCastleTo] == PIECES.EMPTY &&
              board[chessCache.blackKingRookCastleTo] == PIECES.EMPTY &&
              !whiteAtks.isSet(chessCache.blackKingCastleTo) &&
              !whiteAtks.isSet(
                  chessCache.blackKingRookCastleTo)) {  // and not attacked
            m.addConstMove(chessCache.blackKingSideCastle);
          }
        }
        if (blackQueenSideCastle()) {
          if (board[chessCache.blackQueenCastleTo] == PIECES.EMPTY &&
              board[chessCache.blackQueenRookCastleTo] == PIECES.EMPTY &&
              board[chessCache.blackQueenCastleVacant] == PIECES.EMPTY &&
              !whiteAtks.isSet(chessCache.blackQueenCastleTo) &&
              !whiteAtks.isSet(
                  chessCache.blackQueenRookCastleTo)) {  // and not attacked
            m.addConstMove(chessCache.blackQueenSideCastle);
          }
        }
      }
    }
  };

  // manually written/hard coded for less looping, so its a bit faster
  void genPseudoPawnMoves() {
    for (int i = 0; i < pawns[1].amt; ++i) {
      int pieceIndex = pawns[1].PIECES[i];
      const Move addingMove = chessCache.wPawnMoves[pieceIndex][2];
      if (!addingMove.isNull()) {
        whiteAtks.setSquare(addingMove.moveTo());
        if (addingMove.moveTo() == blackKing) {
          blockRay.setSquare(pieceIndex);
          checkRay.setSquare(blackKing);
          ++checkCount;
        }
      }
      const Move addingMove2 = chessCache.wPawnMoves[pieceIndex][3];
      if (!addingMove2.isNull()) {
        whiteAtks.setSquare(addingMove2.moveTo());
        if (addingMove2.moveTo() == blackKing) {
          blockRay.setSquare(pieceIndex);
          checkRay.setSquare(blackKing);
          ++checkCount;
        }
      }
    }
    for (int i = 0; i < pawns[0].amt; ++i) {
      int pieceIndex = pawns[0].PIECES[i];
      const Move addingMove = chessCache.bPawnMoves[pieceIndex][2];
      if (!addingMove.isNull()) {
        blackAtks.setSquare(addingMove.moveTo());
        if (addingMove.moveTo() == whiteKing) {
          blockRay.setSquare(pieceIndex);
          checkRay.setSquare(whiteKing);
          ++checkCount;
        }
      }
      const Move addingMove2 = chessCache.bPawnMoves[pieceIndex][3];
      if (!addingMove2.isNull()) {
        blackAtks.setSquare(addingMove2.moveTo());
        if (addingMove2.moveTo() == whiteKing) {
          blockRay.setSquare(pieceIndex);
          checkRay.setSquare(whiteKing);
          ++checkCount;
        }
      }
    }
  };

  // manually written/hard coded for less looping, so its a bit faster
  void genPseudoKnightMoves() {
    // if (turn == PIECES.WHITE) {
    for (int i = 0; i < knights[1].amt; ++i) {
      for (int j = 0; j < 8; ++j) {
        const Move addingMove = chessCache.knightMoves[knights[1].PIECES[i]][j];
        if (!addingMove.isNull()) {
          whiteAtks.setSquare(addingMove.moveTo());
          if (addingMove.moveTo() == blackKing) {
            blockRay.setSquare(knights[1].PIECES[i]);
            checkRay.setSquare(blackKing);
            ++checkCount;
          }
        }
      }
    }
    //} else {
    for (int i = 0; i < knights[0].amt; ++i) {
      for (int j = 0; j < 8; ++j) {
        const Move addingMove = chessCache.knightMoves[knights[0].PIECES[i]][j];
        if (!addingMove.isNull()) {
          blackAtks.setSquare(addingMove.moveTo());
          if (addingMove.moveTo() == whiteKing) {
            blockRay.setSquare(knights[0].PIECES[i]);
            checkRay.setSquare(whiteKing);
            ++checkCount;
          }
        }
      }
    }
    //}
  };

  // manually written/hard coded for less looping, so its a bit faster
  void genPseudoBishopMoves() {
    // if (turn == PIECES.WHITE) {
    for (int i = 0; i < bishops[1].amt; ++i) {
      // for each direction
      for (int d = 0; d < 4; ++d) {
        for (int r = 0; r < 7; ++r) {
          const Move addingMove =
              chessCache.bishopMoves[d][bishops[1].PIECES[i]][r];
          if (!addingMove.isNull()) {
            whiteAtks.setSquare(addingMove.moveTo());
            if (addingMove.moveTo() == blackKing) {
              checkRay =
                  chessCache.rays1Extra[addingMove.moveFrom()][blackKing];
              blockRay = chessCache.rays[addingMove.moveFrom()][blackKing];
              ++checkCount;
            }
            if (allPIECES.isSet(addingMove.moveTo())) {
              if (r < 6) {
                for (int r2 = r + 1; r2 < 7; ++r2) {
                  const Move &continueMove =
                      chessCache.bishopMoves[d][bishops[1].PIECES[i]][r2];
                  if (!continueMove.isNull()) {
                    uint8_t moveTo = continueMove.moveTo();
                    if (allPIECES.isSet(moveTo)) {
                      if (moveTo == blackKing) {
                        pinMasks[1][d] =
                            chessCache.rays[addingMove.moveFrom()][blackKing];
                        pinExistInPosition = true;
                      }
                      break;
                    }
                  }
                }
              }
              break;
            }
          }
        }
      }
    }

    for (int i = 0; i < queens[1].amt; ++i) {
      // for each direction
      for (int d = 0; d < 4; ++d) {
        for (int r = 0; r < 7; ++r) {
          const Move addingMove =
              chessCache.bishopMoves[d][queens[1].PIECES[i]][r];
          if (!addingMove.isNull()) {
            whiteAtks.setSquare(addingMove.moveTo());
            if (addingMove.moveTo() == blackKing) {
              checkRay =
                  chessCache.rays1Extra[addingMove.moveFrom()][blackKing];
              blockRay = chessCache.rays[addingMove.moveFrom()][blackKing];
              ++checkCount;
            }
            if (allPIECES.isSet(addingMove.moveTo())) {
              if (r < 6) {
                for (int r2 = r + 1; r2 < 7; ++r2) {
                  const Move &continueMove =
                      chessCache.bishopMoves[d][queens[1].PIECES[i]][r2];
                  if (!continueMove.isNull()) {
                    uint8_t moveTo = continueMove.moveTo();
                    if (allPIECES.isSet(moveTo)) {
                      if (moveTo == blackKing) {
                        pinMasks[1][d] =
                            chessCache.rays[addingMove.moveFrom()][blackKing];
                        pinExistInPosition = true;
                      }
                      break;
                    }
                  }
                }
              }
              break;
            }
          }
        }
      }
    }
    //} else {
    for (int i = 0; i < bishops[0].amt; ++i) {
      // for each direction
      for (int d = 0; d < 4; ++d) {
        for (int r = 0; r < 7; ++r) {
          const Move addingMove =
              chessCache.bishopMoves[d][bishops[0].PIECES[i]][r];
          if (!addingMove.isNull()) {
            blackAtks.setSquare(addingMove.moveTo());
            if (addingMove.moveTo() == whiteKing) {
              checkRay =
                  chessCache.rays1Extra[addingMove.moveFrom()][whiteKing];
              blockRay = chessCache.rays[addingMove.moveFrom()][whiteKing];
              ++checkCount;
            }
            if (allPIECES.isSet(addingMove.moveTo())) {
              if (r < 6) {
                for (int r2 = r + 1; r2 < 7; ++r2) {
                  const Move &continueMove =
                      chessCache.bishopMoves[d][bishops[0].PIECES[i]][r2];
                  if (!continueMove.isNull()) {
                    uint8_t moveTo = continueMove.moveTo();
                    if (allPIECES.isSet(moveTo)) {
                      if (moveTo == whiteKing) {
                        pinMasks[0][d] =
                            chessCache.rays[addingMove.moveFrom()][whiteKing];
                        pinExistInPosition = true;
                      }
                      break;
                    }
                  }
                }
              }
              break;
            }
          }
        }
      }
    }

    for (int i = 0; i < queens[0].amt; ++i) {
      // for each direction
      for (int d = 0; d < 4; ++d) {
        for (int r = 0; r < 7; ++r) {
          const Move addingMove =
              chessCache.bishopMoves[d][queens[0].PIECES[i]][r];
          if (!addingMove.isNull()) {
            blackAtks.setSquare(addingMove.moveTo());
            if (addingMove.moveTo() == whiteKing) {
              checkRay =
                  chessCache.rays1Extra[addingMove.moveFrom()][whiteKing];
              blockRay = chessCache.rays[addingMove.moveFrom()][whiteKing];
              ++checkCount;
            }
            if (allPIECES.isSet(addingMove.moveTo())) {
              if (r < 6) {
                for (int r2 = r + 1; r2 < 7; ++r2) {
                  const Move &continueMove =
                      chessCache.bishopMoves[d][queens[0].PIECES[i]][r2];
                  if (!continueMove.isNull()) {
                    uint8_t moveTo = continueMove.moveTo();
                    if (allPIECES.isSet(moveTo)) {
                      if (moveTo == whiteKing) {
                        pinMasks[0][d] =
                            chessCache.rays[addingMove.moveFrom()][whiteKing];
                        pinExistInPosition = true;
                      }
                      break;
                    }
                  }
                }
              }
              break;
            }
          }
        }
      }
    }
    //}
  };

  // manually written/hard coded for less looping, so its a bit faster
  void genPseudoRookMoves() {
    // if (turn == PIECES.WHITE) {
    for (int i = 0; i < rooks[1].amt; ++i) {
      // for each direction
      for (int d = 0; d < 4; ++d) {
        for (int r = 0; r < 7; ++r) {
          const Move addingMove =
              chessCache.rookMoves[d][rooks[1].PIECES[i]][r];
          if (!addingMove.isNull()) {
            whiteAtks.setSquare(addingMove.moveTo());
            if (addingMove.moveTo() == blackKing) {
              checkRay =
                  chessCache.rays1Extra[addingMove.moveFrom()][blackKing];
              blockRay = chessCache.rays[addingMove.moveFrom()][blackKing];
              ++checkCount;
            }
            if (allPIECES.isSet(addingMove.moveTo())) {
              // pin masks
              if (r < 6) {
                for (int r2 = r + 1; r2 < 7; ++r2) {
                  const Move &continueMove =
                      chessCache.rookMoves[d][rooks[1].PIECES[i]][r2];
                  if (!continueMove.isNull()) {
                    uint8_t moveTo = continueMove.moveTo();
                    if (allPIECES.isSet(moveTo)) {
                      if (moveTo == blackKing) {
                        pinMasks[1][d + 4] =
                            chessCache.rays[addingMove.moveFrom()][blackKing];
                        pinExistInPosition = true;
                      }
                      break;
                    }
                  }
                }
              }
              break;
            }
          }
        }
      }
    }

    for (int i = 0; i < queens[1].amt; ++i) {
      // for each direction
      for (int d = 0; d < 4; ++d) {
        for (int r = 0; r < 7; ++r) {
          const Move addingMove =
              chessCache.rookMoves[d][queens[1].PIECES[i]][r];
          if (!addingMove.isNull()) {
            whiteAtks.setSquare(addingMove.moveTo());
            if (addingMove.moveTo() == blackKing) {
              checkRay =
                  chessCache.rays1Extra[addingMove.moveFrom()][blackKing];
              blockRay = chessCache.rays[addingMove.moveFrom()][blackKing];
              ++checkCount;
            }
            if (allPIECES.isSet(addingMove.moveTo())) {
              // pin masks
              if (r < 6) {
                for (int r2 = r + 1; r2 < 7; ++r2) {
                  const Move &continueMove =
                      chessCache.rookMoves[d][queens[1].PIECES[i]][r2];
                  if (!continueMove.isNull()) {
                    uint8_t moveTo = continueMove.moveTo();
                    if (allPIECES.isSet(moveTo)) {
                      if (moveTo == blackKing) {
                        pinMasks[1][d + 4] =
                            chessCache.rays[addingMove.moveFrom()][blackKing];
                        pinExistInPosition = true;
                      }
                      break;
                    }
                  }
                }
              }
              break;
            }
          }
        }
      }
    }
    //} else {
    for (int i = 0; i < rooks[0].amt; ++i) {
      // for each direction
      for (int d = 0; d < 4; ++d) {
        for (int r = 0; r < 7; ++r) {
          const Move addingMove =
              chessCache.rookMoves[d][rooks[0].PIECES[i]][r];
          if (!addingMove.isNull()) {
            blackAtks.setSquare(addingMove.moveTo());
            if (addingMove.moveTo() == whiteKing) {
              checkRay =
                  chessCache.rays1Extra[addingMove.moveFrom()][whiteKing];
              blockRay = chessCache.rays[addingMove.moveFrom()][whiteKing];
              ++checkCount;
            }
            if (allPIECES.isSet(addingMove.moveTo())) {
              // pin masks
              if (r < 6) {
                for (int r2 = r + 1; r2 < 7; ++r2) {
                  const Move &continueMove =
                      chessCache.rookMoves[d][rooks[0].PIECES[i]][r2];
                  if (!continueMove.isNull()) {
                    uint8_t moveTo = continueMove.moveTo();
                    if (allPIECES.isSet(moveTo)) {
                      if (moveTo == whiteKing) {
                        pinMasks[0][d + 4] =
                            chessCache.rays[addingMove.moveFrom()][whiteKing];
                        pinExistInPosition = true;
                      }
                      break;
                    }
                  }
                }
              }
              break;
            }
          }
        }
      }
    }

    for (int i = 0; i < queens[0].amt; ++i) {
      // for each direction
      for (int d = 0; d < 4; ++d) {
        for (int r = 0; r < 7; ++r) {
          const Move addingMove =
              chessCache.rookMoves[d][queens[0].PIECES[i]][r];
          if (!addingMove.isNull()) {
            blackAtks.setSquare(addingMove.moveTo());
            if (addingMove.moveTo() == whiteKing) {
              checkRay =
                  chessCache.rays1Extra[addingMove.moveFrom()][whiteKing];
              blockRay = chessCache.rays[addingMove.moveFrom()][whiteKing];
              ++checkCount;
            }
            if (allPIECES.isSet(addingMove.moveTo())) {
              // pin masks
              if (r < 6) {
                for (int r2 = r + 1; r2 < 7; ++r2) {
                  const Move &continueMove =
                      chessCache.rookMoves[d][queens[0].PIECES[i]][r2];
                  if (!continueMove.isNull()) {
                    uint8_t moveTo = continueMove.moveTo();
                    if (allPIECES.isSet(moveTo)) {
                      if (moveTo == whiteKing) {
                        pinMasks[0][d + 4] =
                            chessCache.rays[addingMove.moveFrom()][whiteKing];
                        pinExistInPosition = true;
                      }
                      break;
                    }
                  }
                }
              }
              break;
            }
          }
        }
      }
    }
    //}
  };

  // manually written/hard coded for less looping, so its a bit faster
  void genPseudoKingMoves() {
    for (int j = 0; j < 8; ++j) {
      const Move addingMove = chessCache.kingMoves[whiteKing][j];
      if (!addingMove.isNull()) {
        whiteAtks.setSquare(addingMove.moveTo());
      }
    }
    for (int j = 0; j < 8; ++j) {
      const Move addingMove = chessCache.kingMoves[blackKing][j];
      if (!addingMove.isNull()) {
        blackAtks.setSquare(addingMove.moveTo());
      }
    }
  };

  uint64_t generateZKey() {
    uint64_t zobristKey = 0;
    for (int i = 0; i < 64; ++i) {
      uint8_t pieceType = PIECES.type(board[i]);
      if (pieceType != PIECES.EMPTY) {  // if not 0/EMPTY
        // xor together
        int pieceCol = PIECES.color(board[i]);
        if (pieceCol == PIECES.WHITE) {
          pieceCol = 0;
        } else {
          pieceCol = 1;
        }
        zobristKey ^=
            chessCache.zobristLookup[i][pieceCol]
                                    [pieceType - 1];  // -1 cuz EMPTY is ignored
                                                      // in zobrist lookup
      }
    }
    if (turn == PIECES.BLACK) {
      zobristKey ^= chessCache.blackTurnZobrist;
    }
    chessCache.zobristEnPassant[getEnPassantFile()];
    zobristKey ^= chessCache.zobristCastling[currentGameState & castlingMASK];
    return zobristKey;
  }

  void generatePseudoLegals() {
    // clear pins
    if (pinExistInPosition) {
      for (int d = 0; d < 8; ++d) {
        pinMasks[0][d].clearBoard();
        pinMasks[1][d].clearBoard();
      }
      pinExistInPosition = false;
    }
    // clear checks
    checkCount = 0;
    checkRay.clearBoard();
    blockRay.clearBoard();

    boolWhiteCheck = whiteInCheck();
    boolBlackCheck = blackInCheck();

    // clear attacks
    whiteAtks.clearBoard();
    blackAtks.clearBoard();

    // update
    genPseudoPawnMoves();
    genPseudoKnightMoves();
    genPseudoBishopMoves();
    genPseudoRookMoves();
    genPseudoKingMoves();
    zobristKey = generateZKey();
  }

  void generateMoves(moveList &moves, bool includeQuiets) {
    if (fiftyMoveCounter > 50 || repHistory.isThreeFold()) {
      return;
    }
    genQuiets = includeQuiets;
    if (checkCount < 2) {
      generatePawnMoves(moves);
      generateKnightMoves(moves);
      generateBishopMoves(moves);
      generateRookMoves(moves);
    }
    generateKingMoves(moves);
  };

  void makeMove(const Move &m) {
    uint8_t newCastleState = (currentGameState & 15);  // 15 = 0b1111
    currentGameState = 0;

    uint8_t startSquare = m.moveFrom();
    uint8_t targetSquare = m.moveTo();

    prevMove = m;

    // moving
    if (board[startSquare] == PIECES.BPAWN) {
      pawns[0].MovePiece(startSquare, targetSquare);
    } else if (board[startSquare] == PIECES.WPAWN) {
      pawns[1].MovePiece(startSquare, targetSquare);
    } else if (board[startSquare] == PIECES.BKNIGHT) {
      knights[0].MovePiece(startSquare, targetSquare);
    } else if (board[startSquare] == PIECES.WKNIGHT) {
      knights[1].MovePiece(startSquare, targetSquare);
    } else if (board[startSquare] == PIECES.WBISHOP) {
      bishops[1].MovePiece(startSquare, targetSquare);
    } else if (board[startSquare] == PIECES.BBISHOP) {
      bishops[0].MovePiece(startSquare, targetSquare);
    } else if (board[startSquare] == PIECES.WROOK) {
      rooks[1].MovePiece(startSquare, targetSquare);
      if (startSquare ==
          chessCache.whiteKingRook) {  // remove king side castling
        newCastleState &= whiteCastleKingsideMask;
      } else if (startSquare ==
                 chessCache.whiteQueenRook) {  // remove queen side castling
        newCastleState &= whiteCastleQueensideMask;
      };
    } else if (board[startSquare] == PIECES.BROOK) {
      rooks[0].MovePiece(startSquare, targetSquare);
      if (startSquare ==
          chessCache.blackKingRook) {  // remove king side castling
        newCastleState &= blackCastleKingsideMask;
      } else if (startSquare ==
                 chessCache.blackQueenRook) {  // remove queen side castling
        newCastleState &= blackCastleQueensideMask;
      };
    } else if (board[startSquare] == PIECES.WQUEEN) {
      queens[1].MovePiece(startSquare, targetSquare);
    } else if (board[startSquare] == PIECES.BQUEEN) {
      queens[0].MovePiece(startSquare, targetSquare);
    } else if (startSquare == whiteKing) {
      whiteKing = targetSquare;
      newCastleState &= whiteCastleMask;
    } else if (startSquare == blackKing) {
      blackKing = targetSquare;
      newCastleState &= blackCastleMask;
    }

    // Pawn has moved two forwards, mark file with en-passant flag
    if (m.isPawnTwoUp()) {
      int file =
          chessCache.preComputedCols[startSquare] + 1;  // +1 cuz 0 means none
      currentGameState |= (file << 4);
    }

    // castling
    if (m.isCastling()) {
      if (targetSquare ==
          chessCache.whiteKingCastleTo) {  // white king side castle
        board[chessCache.whiteKingRookCastleTo] = PIECES.WROOK;
        deleteTile(chessCache.whiteKingRook);
        rooks[turnIndex].MovePiece(chessCache.whiteKingRook,
                                   chessCache.whiteKingRookCastleTo);
        allPIECES.setSquare(chessCache.whiteKingRookCastleTo);

      } else if (targetSquare ==
                 chessCache.whiteQueenCastleTo) {  // white queen side castle
        board[chessCache.whiteQueenRookCastleTo] = PIECES.WROOK;
        deleteTile(chessCache.whiteQueenRook);
        rooks[turnIndex].MovePiece(chessCache.whiteQueenRook,
                                   chessCache.whiteQueenRookCastleTo);
        allPIECES.setSquare(chessCache.whiteQueenRookCastleTo);

      } else if (targetSquare ==
                 chessCache.blackKingCastleTo) {  // black king side castle
        board[chessCache.blackKingRookCastleTo] = PIECES.BROOK;
        deleteTile(chessCache.blackKingRook);
        rooks[turnIndex].MovePiece(chessCache.blackKingRook,
                                   chessCache.blackKingRookCastleTo);
        allPIECES.setSquare(chessCache.blackKingRookCastleTo);

      } else if (targetSquare ==
                 chessCache.blackQueenCastleTo) {  // black king side castle
        board[chessCache.blackQueenRookCastleTo] = PIECES.BROOK;
        deleteTile(chessCache.blackQueenRook);
        rooks[turnIndex].MovePiece(chessCache.blackQueenRook,
                                   chessCache.blackQueenRookCastleTo);
        allPIECES.setSquare(chessCache.blackQueenRookCastleTo);
      }
    }

    // promotion
    if (m.isPromotion()) {
      pawns[turnIndex].removeAtTile(
          targetSquare);  // remove at target square where move piece was used
      if (m.promoteQueen()) {
        board[startSquare] = PIECES.QUEEN | turn;
        queens[turnIndex].addAtTile(targetSquare);
      } else if (m.promoteRook()) {
        board[startSquare] = PIECES.ROOK | turn;
        rooks[turnIndex].addAtTile(targetSquare);
      } else if (m.promoteBishop()) {
        board[startSquare] = PIECES.BISHOP | turn;
        bishops[turnIndex].addAtTile(targetSquare);
      } else if (m.promoteKnight()) {
        board[startSquare] = PIECES.KNIGHT | turn;
        knights[turnIndex].addAtTile(targetSquare);
      }
    };

    // en passant
    if (m.isEnPassant()) {
      if (turn == PIECES.WHITE) {
        int epSquare = targetSquare - 8;
        pawns[oppTurnIndex].removeAtTile(epSquare);
        deleteTile(epSquare);
      } else {
        int epSquare = targetSquare + 8;
        pawns[oppTurnIndex].removeAtTile(epSquare);
        deleteTile(epSquare);
      }
    }

    // captures
    if (board[targetSquare] == PIECES.BPAWN) {
      pawns[0].removeAtTile(targetSquare);
    } else if (board[targetSquare] == PIECES.WPAWN) {
      pawns[1].removeAtTile(targetSquare);
    } else if (board[targetSquare] == PIECES.BKNIGHT) {
      knights[0].removeAtTile(targetSquare);
    } else if (board[targetSquare] == PIECES.WKNIGHT) {
      knights[1].removeAtTile(targetSquare);
    } else if (board[targetSquare] == PIECES.WBISHOP) {
      bishops[1].removeAtTile(targetSquare);
    } else if (board[targetSquare] == PIECES.BBISHOP) {
      bishops[0].removeAtTile(targetSquare);
    } else if (board[targetSquare] == PIECES.WROOK) {
      rooks[1].removeAtTile(targetSquare);
      if (targetSquare ==
          chessCache.whiteKingRook) {  // remove king side castling
        newCastleState &= whiteCastleKingsideMask;
      } else if (targetSquare ==
                 chessCache.whiteQueenRook) {  // remove queen side castling
        newCastleState &= whiteCastleQueensideMask;
      };
    } else if (board[targetSquare] == PIECES.BROOK) {
      rooks[0].removeAtTile(targetSquare);
      if (targetSquare ==
          chessCache.blackKingRook) {  // remove king side castling
        newCastleState &= blackCastleKingsideMask;
      } else if (targetSquare ==
                 chessCache.blackQueenRook) {  // remove queen side castling
        newCastleState &= blackCastleQueensideMask;
      };
    } else if (board[targetSquare] == PIECES.WQUEEN) {
      queens[1].removeAtTile(targetSquare);
    } else if (board[targetSquare] == PIECES.BQUEEN) {
      queens[0].removeAtTile(targetSquare);
    }

    allPIECES.setSquare(targetSquare);
    allPIECES.unSetSquare(startSquare);

    ++plyCount;
    ++fiftyMoveCounter;

    if (PIECES.type(board[startSquare]) == PIECES.PAWN ||
        board[targetSquare] != PIECES.EMPTY || m.isEnPassant()) {
      fiftyMoveCounter = 0;
    }

    currentGameState |= newCastleState;              // castling
    currentGameState |= (board[targetSquare] << 8);  // capture
    currentGameState |= (fiftyMoveCounter << 16);    // fifty move counter

    gameStateHistory.push(currentGameState);
    repHistory.push(zobristKey);

    board[targetSquare] = board[startSquare];
    board[startSquare] = PIECES.EMPTY;
    makeTurn();

    generatePseudoLegals();
  };

  void unMakeMove(const Move &m) {
    --plyCount;
    uint8_t startSquare = m.moveFrom();
    uint8_t targetSquare = m.moveTo();

    uint8_t capturedPiece = (currentGameState >> 8) & 63;
    prevMove.clearMove();

    // moving
    if (board[targetSquare] == PIECES.BPAWN) {
      pawns[0].MovePiece(targetSquare, startSquare);
    } else if (board[targetSquare] == PIECES.WPAWN) {
      pawns[1].MovePiece(targetSquare, startSquare);
    } else if (board[targetSquare] == PIECES.BKNIGHT) {
      knights[0].MovePiece(targetSquare, startSquare);
    } else if (board[targetSquare] == PIECES.WKNIGHT) {
      knights[1].MovePiece(targetSquare, startSquare);
    } else if (board[targetSquare] == PIECES.WBISHOP) {
      bishops[1].MovePiece(targetSquare, startSquare);
    } else if (board[targetSquare] == PIECES.BBISHOP) {
      bishops[0].MovePiece(targetSquare, startSquare);
    } else if (board[targetSquare] == PIECES.WROOK) {
      rooks[1].MovePiece(targetSquare, startSquare);
    } else if (board[targetSquare] == PIECES.BROOK) {
      rooks[0].MovePiece(targetSquare, startSquare);
    } else if (board[targetSquare] == PIECES.WQUEEN) {
      queens[1].MovePiece(targetSquare, startSquare);
    } else if (board[targetSquare] == PIECES.BQUEEN) {
      queens[0].MovePiece(targetSquare, startSquare);
    } else if (targetSquare == whiteKing) {
      whiteKing = startSquare;
    } else if (targetSquare == blackKing) {
      blackKing = startSquare;
    }

    // castling
    if (m.isCastling()) {
      if (targetSquare ==
          chessCache.whiteKingCastleTo) {  // white king side castle
        board[chessCache.whiteKingRook] = PIECES.WROOK;
        rooks[oppTurnIndex].MovePiece(chessCache.whiteKingRookCastleTo,
                                      chessCache.whiteKingRook);
        deleteTile(chessCache.whiteKingRookCastleTo);
        allPIECES.setSquare(chessCache.whiteKingRook);

      } else if (targetSquare ==
                 chessCache.whiteQueenCastleTo) {  // white queen side castle
        board[chessCache.whiteQueenRook] = PIECES.WROOK;
        rooks[oppTurnIndex].MovePiece(chessCache.whiteQueenRookCastleTo,
                                      chessCache.whiteQueenRook);
        deleteTile(chessCache.whiteQueenRookCastleTo);
        allPIECES.setSquare(chessCache.whiteQueenRook);

      } else if (targetSquare ==
                 chessCache.blackKingCastleTo) {  // black king side castle
        board[chessCache.blackKingRook] = PIECES.BROOK;
        rooks[oppTurnIndex].MovePiece(chessCache.blackKingRookCastleTo,
                                      chessCache.blackKingRook);
        deleteTile(chessCache.blackKingRookCastleTo);
        allPIECES.setSquare(chessCache.blackKingRook);

      } else if (targetSquare ==
                 chessCache.blackQueenCastleTo) {  // black king side castle
        board[chessCache.blackQueenRook] = PIECES.BROOK;
        rooks[oppTurnIndex].MovePiece(chessCache.blackQueenRookCastleTo,
                                      chessCache.blackQueenRook);
        deleteTile(chessCache.blackQueenRookCastleTo);
        allPIECES.setSquare(chessCache.blackQueenRook);
      }
    }

    // promotion
    if (m.isPromotion()) {
      board[targetSquare] = PIECES.PAWN | oppTurn;
      if (m.promoteQueen()) {
        queens[oppTurnIndex].removeAtTile(targetSquare);
      } else if (m.promoteRook()) {
        rooks[oppTurnIndex].removeAtTile(targetSquare);
      } else if (m.promoteBishop()) {
        bishops[oppTurnIndex].removeAtTile(targetSquare);
      } else if (m.promoteKnight()) {
        knights[oppTurnIndex].removeAtTile(targetSquare);
      }
      pawns[oppTurnIndex].addAtTile(startSquare);
    }

    // un en passant
    if (m.isEnPassant()) {
      if (turn == PIECES.WHITE) {
        int epSquare = targetSquare + 8;
        pawns[turnIndex].addAtTile(epSquare);
        allPIECES.setSquare(epSquare);
        board[epSquare] = PIECES.WPAWN;
      } else {
        int epSquare = targetSquare - 8;
        pawns[turnIndex].addAtTile(epSquare);
        allPIECES.setSquare(epSquare);
        board[epSquare] = PIECES.BPAWN;
      }
    }

    // uncaptures
    allPIECES.setSquare(startSquare);
    if (capturedPiece == PIECES.EMPTY) {
      allPIECES.unSetSquare(targetSquare);
    } else if (capturedPiece == PIECES.BPAWN) {
      pawns[0].addAtTile(targetSquare);
    } else if (capturedPiece == PIECES.WPAWN) {
      pawns[1].addAtTile(targetSquare);
    } else if (capturedPiece == PIECES.BKNIGHT) {
      knights[0].addAtTile(targetSquare);
    } else if (capturedPiece == PIECES.WKNIGHT) {
      knights[1].addAtTile(targetSquare);
    } else if (capturedPiece == PIECES.BBISHOP) {
      bishops[0].addAtTile(targetSquare);
    } else if (capturedPiece == PIECES.WBISHOP) {
      bishops[1].addAtTile(targetSquare);
    } else if (capturedPiece == PIECES.BROOK) {
      rooks[0].addAtTile(targetSquare);
    } else if (capturedPiece == PIECES.WROOK) {
      rooks[1].addAtTile(targetSquare);
    } else if (capturedPiece == PIECES.BQUEEN) {
      queens[0].addAtTile(targetSquare);
    } else if (capturedPiece == PIECES.WQUEEN) {
      queens[1].addAtTile(targetSquare);
    }

    board[startSquare] = board[targetSquare];
    board[targetSquare] = capturedPiece;

    gameStateHistory.pop();  // removes current state from history
    repHistory.pop();
    currentGameState =
        gameStateHistory
            .peek();  // sets current state to previous state in history

    fiftyMoveCounter = (currentGameState >> 16);

    makeTurn();
    generatePseudoLegals();
  }

  int heuristicEval() {
    int eval = 0;

    // material scores
    eval += pawns[1].amt * PreComputedCache::pawnValue;
    eval += knights[1].amt * PreComputedCache::knightValue;
    eval += bishops[1].amt * PreComputedCache::bishopValue;
    eval += rooks[1].amt * PreComputedCache::rookValue;
    eval += queens[1].amt * PreComputedCache::queenValue;

    eval -= pawns[0].amt * PreComputedCache::pawnValue;
    eval -= knights[0].amt * PreComputedCache::knightValue;
    eval -= bishops[0].amt * PreComputedCache::bishopValue;
    eval -= rooks[0].amt * PreComputedCache::rookValue;
    eval -= queens[0].amt * PreComputedCache::queenValue;

    // guard and threats scores
    eval += whiteAtks.populationCountBAND(allPIECES.get());
    eval -= blackAtks.populationCountBAND(allPIECES.get());
    if (plyCount > 8) {  // agressive
      eval += whiteAtks.populationCountBAND(allPIECES.get()) * 10;
      eval -= blackAtks.populationCountBAND(allPIECES.get()) * 10;
    }

    // mobility scores
    uint8_t whitepop = whiteAtks.populationCount();
    uint8_t blackpop = blackAtks.populationCount();
    eval += whitepop;
    eval -= blackpop;

    // imbalance PIECES scores
    // bishop pairs
    if (bishops[1].amt >= 2) {
      eval += 60;
    }
    if (bishops[0].amt >= 2) {
      eval -= 60;
    }

    int materialBig = rooks[1].amt * 2 + rooks[0].amt * 2 +   // 8
                      bishops[1].amt + bishops[0].amt +       // 4
                      knights[1].amt + knights[0].amt +       // 4
                      queens[1].amt * 5 + queens[0].amt * 5;  // 10
    // should sum to 26

    if ((pawns[1].amt + pawns[0].amt) == 0) {
      if (materialBig < 2) {
        return 0;
      };
    }

    // piece square tables
    if (materialBig <= 15) {  // endgame determin
      for (int i = 0; i < pawns[0].amt; ++i) {
        eval -= chessCache.pawnEndPST[0][pawns[0].PIECES[i]];
        eval +=
            chessCache
                .distances[blackKing][pawns[0].PIECES[i]];  // less value if far
                                                            // away from king
      }
      for (int i = 0; i < pawns[1].amt; ++i) {
        eval += chessCache.pawnEndPST[1][pawns[1].PIECES[i]];
        eval -=
            chessCache
                .distances[whiteKing][pawns[1].PIECES[i]];  // less value if far
                                                            // away from king
      }
      eval += chessCache.centerPST[whiteKing];
      eval -= chessCache.centerPST[blackKing];
    } else {  // normal
      for (int i = 0; i < pawns[0].amt; ++i) {
        eval -= chessCache.pawnPST[0][pawns[0].PIECES[i]];
      }
      for (int i = 0; i < pawns[1].amt; ++i) {
        eval += chessCache.pawnPST[1][pawns[1].PIECES[i]];
      }
      eval += chessCache.kingPST[1][whiteKing];
      eval -= chessCache.kingPST[0][blackKing];
    }

    //
    for (int i = 0; i < knights[0].amt; ++i) {
      eval -= chessCache.horsePST[0][knights[0].PIECES[i]];
    }
    for (int i = 0; i < knights[1].amt; ++i) {
      eval += chessCache.horsePST[1][knights[1].PIECES[i]];
    }
    //
    for (int i = 0; i < bishops[0].amt; ++i) {
      eval -= chessCache.bishopPST[0][bishops[0].PIECES[i]];
    }
    for (int i = 0; i < bishops[1].amt; ++i) {
      eval += chessCache.bishopPST[1][bishops[1].PIECES[i]];
    }
    //
    for (int i = 0; i < rooks[0].amt; ++i) {
      eval -= chessCache.rookPST[0][rooks[0].PIECES[i]];
    }
    for (int i = 0; i < rooks[1].amt; ++i) {
      eval += chessCache.rookPST[1][rooks[1].PIECES[i]];
    }
    return eval;
  };

  Move search_BestMove;
  int search_Nodes = 0;
  int search_Depth = 0;
  int search_ExtendedDepth = 0;  // depth reached with extensions
  std::chrono::steady_clock::time_point search_start =
      std::chrono::steady_clock::now();

  searchRes oSearch(int lockedDepth) {
    int beta = PreComputedCache::evalPositiveInf;
    int alpha = PreComputedCache::evalNegativeInf;

    searchRes result;

    int depthReached = 1;
    result.eval = 0;
    search_Nodes = 0;

    if (lockedDepth != 0) {  // if locked depth is set
      depthReached = lockedDepth;
      search_BestMove.clearMove();
      result.eval = alphaBeta(lockedDepth, 0, alpha, beta, 0);
    } else {
      search_start = std::chrono::steady_clock::now();
      for (int depth = 1; depth < 100; ++depth) {  // iterative deepening
        depthReached = depth;
        search_BestMove.clearMove();
        result.eval = alphaBeta(depth, 0, alpha, beta, 0);

        setTxtColor(chessColors.greyLetCol);
        // std::cout << '.';

        std::cout << "depth: " << depth << ", " << notateMove(search_BestMove)
                  << ", eval: " << result.eval << '\n';

        if (abs(result.eval) >= chessCache.evalWhiteWins) {
          result.mateIn = depth;
          break;
        }
        auto endt = std::chrono::steady_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(
                            endt - search_start)
                            .count();
        if (duration > 4000) { // think time
          break;
        }
      }
    }

    // system("PAUSE");
    result.m = search_BestMove;
    result.nodes = search_Nodes;
    result.depth = depthReached;
    result.depthExtended = search_ExtendedDepth;
    return result;
  };

  // recursion
  int alphaBeta(int depth, int plyFromRoot, int alpha, int beta,
                int numExtensions) {
    /*if (plyFromRoot > 0) {
        // mating seq already found
        int newAlpha = -PreComputedCache::evalWhiteWins - plyFromRoot;
        int newBeta = PreComputedCache::evalWhiteWins + plyFromRoot;
        if (newAlpha > alpha) {
            alpha = newAlpha;
        }
        if (newBeta < beta) {
            beta = newBeta;
        }

        if (alpha >= beta){
            return alpha;
        }
    }*/
    if (depth == 0) {  // leaf
      search_ExtendedDepth = plyFromRoot;
      return heuristicEval();
      // return quiesce(alpha, beta);
    }
    moveList genMoves;
    generateMoves(genMoves, true);

    if (genMoves.amt == 0) {
      if (repHistory.isThreeFold()) {
        return 0;
      }
      if (whiteInCheck()) {  // white checkmated
        return PreComputedCache::evalWhiteLoss - depth;
      } else if (blackInCheck()) {
        return PreComputedCache::evalWhiteWins + depth;
      }
      return 0;  // stalemate
    }

    orderMoves(genMoves);
    Move currentBestMove;

    if (turnIndex) {  // white turn
      int maxEval = PreComputedCache::evalNegativeInf;
      for (uint8_t i = 0; i < genMoves.amt; ++i) {
        Move genMove = genMoves.moves[i].move;
        makeMove(genMove);
        int extension =
            numExtensions < 8 && inCheck() ? 1 : 0;  // check extension
        if ((genMoves.moves[i].score < -10 || i > 20) &&
            depth > 1) {  // late move reduction
          extension = -1;
        }
        int score = alphaBeta(depth - 1 + extension, plyFromRoot + 1, alpha,
                              beta, numExtensions + extension);
        unMakeMove(genMove);
        ++search_Nodes;
        if (score > maxEval) {
          currentBestMove = genMove;
          if (plyFromRoot == 0) {
            search_BestMove = genMove;
          }
          maxEval = score;
        }
        if (score >= beta) {  // fail hard beta-cutoff
          moveHHistory[1][genMove.moveFrom()][genMove.moveTo()] =
              depth * depth * 4;
          return beta;
        }
        if (score > alpha) {  // found new best move in this position
          alpha = score;
        }
      }
      return maxEval;

    } else {  // black turn

      int minEval = PreComputedCache::evalPositiveInf;
      for (uint8_t i = 0; i < genMoves.amt; ++i) {
        Move genMove = genMoves.moves[i].move;
        makeMove(genMove);
        int extension =
            numExtensions < 8 && inCheck() ? 1 : 0;  // check extension
        if ((genMoves.moves[i].score < -10 || i > 20) &&
            depth > 1) {  // late move reduction
          extension = -1;
        }
        int score = alphaBeta(depth - 1 + extension, plyFromRoot + 1, alpha,
                              beta, numExtensions + extension);
        unMakeMove(genMove);
        ++search_Nodes;
        if (score < minEval) {
          currentBestMove = genMove;
          if (plyFromRoot == 0) {
            search_BestMove = genMove;
          }
          minEval = score;
        }
        if (score <= alpha) {  // fail hard alpha-cutoff
          moveHHistory[0][genMove.moveFrom()][genMove.moveTo()] =
              depth * depth * 4;
          return alpha;
        }
        if (score < beta) {  // found new best move in this position
          beta = score;
        }
      }
      return minEval;
    }
  }

  Board() { setupFen(chessCache.startingFen); };

  ~Board() { deallocateMoveHHistory(moveHHistory); }

  Board(std::string fen) { setupFen(fen); };
};

// global just for performance testing
BitBoard cBoardHLight;
int PERFT(Board &chessBoard, int depth, int &depthCheck) {
  if (depth == 0) {  // leaf node
    return 1;
  }
  int newData = 0;

  moveList genMoves;
  chessBoard.generateMoves(genMoves, true);
  chessBoard.orderMoves(genMoves);
  for (uint8_t i = 0; i < genMoves.amt; ++i) {
    if (depthCheck == depth) {
      std::cout << '\n' << chessBoard.notateMove(genMoves.moves[i].move);
    }
    chessBoard.makeMove(genMoves.moves[i].move);
    int branchData = PERFT(chessBoard, depth - 1, depthCheck);
    if (depthCheck == depth) {
      std::cout << ": " << branchData;
      std::cout << ", score: " << genMoves.moves[i].score;
    }
    newData += branchData;
    chessBoard.unMakeMove(genMoves.moves[i].move);
  }
  return newData;
}

void perftTest(Board &chessBoard) {
  int finalDepth = 0;
  std::cout << "\nstarting PREFT\ndepth: ";
  std::cin >> finalDepth;
  auto startstart = std::chrono::steady_clock::now();
  for (int d = 0; d <= finalDepth; d++) {
    auto start = std::chrono::steady_clock::now();
    int numPos = PERFT(chessBoard, d, finalDepth);
    auto endt = std::chrono::steady_clock::now();
    auto duration =
        std::chrono::duration_cast<std::chrono::milliseconds>(endt - start)
            .count();
    std::cout << '\n' << d << " ply, ";
    std::cout << " " << numPos << " nodes,  ";
    std::cout << " " << duration << " ms";
  }
  auto endendt = std::chrono::steady_clock::now();
  auto totalDuraction = std::chrono::duration_cast<std::chrono::milliseconds>(
                            endendt - startstart)
                            .count();
  std::cout << "\ntotal time: " << totalDuraction << "ms\n\n";
  system("PAUSE");
};

std::string askFen() {
  std::cin.ignore();
  std::cout << "Enter FEN: ";
  std::string line;
  char ch;
  while ((ch = std::cin.get()) != '\n') {
    line += ch;
  }
  return line;
}

void EvalBar(int eval, Board &chessBoard) {
  // eval bar
  for (int i = 0; i < 64; ++i) {
    int heval = 31 - (eval / 100);
    if (heval > 63) {  // clamp
      heval = 63;
    } else if (heval < 1) {
      heval = 1;
    }

    if (i > heval) {
      setTxtColor(chessColors.bWhiteCol);
    } else {
      setTxtColor(chessColors.bBlackCol);
    }
    if (i == 31 || i == 32) {
      std::cout << '|';
    } else {
      if (i == heval) {
        std::cout << '>';
      } else if (i == heval + 1) {
        std::cout << '<';
      } else {
        std::cout << '=';
      }
    }
  }
  std::cout << "\n\n";
}

void startGame(Board &chessBoard) {
  BitBoard cBoardHLight;
  Move previousMoves[400];
  std::string input = " ";
  std::string aiTxt = " ";
  int aiEval = 0;

  bool blackSide = false;
  int chessBoardPly = 0;  // amt of chess moves
  int moveFrom = 0;
  int moveTo = 0;

  bool whiteAI = false;
  bool blackAI = false;
  int waiLockedDepth = 0;
  int baiLockedDepth = 0;

  do {
    if (!chessBoard.isSynced()) {
      system("PAUSE");
    }
    system("CLS");
    cBoardHLight.clearBoard();

    EvalBar(aiEval, chessBoard);
    chessBoard.display(blackSide, cBoardHLight, aiTxt);

    if ((chessBoard.turn == PIECES.BLACK && blackAI) ||
        (chessBoard.turn == PIECES.WHITE && whiteAI)) {
      auto start = std::chrono::steady_clock::now();
      searchRes minMaxResult;
      if (chessBoard.turn == PIECES.WHITE) {
        minMaxResult = chessBoard.oSearch(waiLockedDepth);
      } else {
        minMaxResult = chessBoard.oSearch(baiLockedDepth);
      }
      aiEval = minMaxResult.eval;
      std::string evalString = intToString(minMaxResult.eval);
      if (minMaxResult.mateIn != 0) {
        evalString = "MATE #" + intToString(minMaxResult.mateIn);
      }
      if (!minMaxResult.m.isNull()) {
        chessBoard.makeMove(minMaxResult.m);
        auto endt = std::chrono::steady_clock::now();
        auto duration =
            std::chrono::duration_cast<std::chrono::milliseconds>(endt - start)
                .count();
        previousMoves[chessBoardPly] = minMaxResult.m;
        ++chessBoardPly;
        aiTxt = "  EVAL: " + evalString +
                ",  nodes: " + intToString(minMaxResult.nodes) +
                ",  time: " + intToString(static_cast<int>(duration)) + "ms" +
                ",  depth: " + intToString(minMaxResult.depth) + "/" +
                intToString(minMaxResult.depthExtended);
        continue;
      } else {
        aiTxt = "[NO MOVE GENERATED] EVAL: " + evalString +
                ",  nodes: " + intToString(minMaxResult.nodes) +
                ",  depth: " + intToString(minMaxResult.depth) + "/" +
                intToString(minMaxResult.depthExtended);
      }
    } else {
      aiEval = chessBoard.heuristicEval();
    }

    std::cout << "input: ";
    std::cin >> input;

    toLowercase(input);
    if (input == "help") {
      std::cout
          << "\n -> COMMANDS <-\n"
          << "\"exit\" - exit current game\n"
          << "\"undo\" - undoes a move\n"
          << "\"flip\" - flips board\n"
          << "\"wai\" - toggle white AI\n"
          << "\"bai\" - toggle black AI\n"
          << "\"f\" - enter custom fen\n"
          << "\"reset\" - resets fen to original\n"
          << "\"baidepth\" - set black ai depth\n"
          << "\"waidepth\" - set white ai depth\n"
          << "\n"
          << "\"cray\" - highlights check ray bitboard\n"
          << "\"PIECES\" - highlights all PIECES bitboard\n"
          << "\"watks\" - highlights all white attacks bitboard\n"
          << "\"batks\" - highlights all black attacks bitboard\n"
          << "\"pins\" - highlights all the pins\n"
          << "\"kings\"  - highlights both kings\n"
          << "\"checkers\" - highights all white tiles\n"  // to test
                                                           // colorOfSquare func
          << "\n\n";
      system("PAUSE");
    } else if (input == "undo") {
      if (chessBoardPly > 0) {
        --chessBoardPly;
        chessBoard.unMakeMove(previousMoves[chessBoardPly]);
      }
      whiteAI = false;
      blackAI = false;
    } else if (input == "flip") {
      blackSide = !blackSide;
    } else if (input == "wai") {
      whiteAI = !whiteAI;
    } else if (input == "bai") {
      blackAI = !blackAI;
    } else if (input == "f") {
      chessBoard.setupFen(askFen());
      std::cout << "\n\n";
    } else if (input == "reset") {
      chessBoard.setupFen(chessCache.startingFen);
    } else if (input == "baidepth") {
      std::cout << "depth: ";
      std::cin >> baiLockedDepth;
    } else if (input == "waidepth") {
      std::cout << "depth: ";
      std::cin >> waiLockedDepth;
    } else if (input == "cray") {
      BitBoard bb = chessBoard.getcray();
      chessBoard.display(blackSide, bb,
                         "  popCount: " + intToString(bb.populationCount()));
      system("PAUSE");
    } else if (input == "checkers") {
      BitBoard bb;
      for (int i = 0; i < 64; ++i) {
        if (chessCache.colorOfSquare(i)) {
          bb.setSquare(i);
        }
      }
      chessBoard.display(blackSide, bb,
                         "  popCount: " + intToString(bb.populationCount()));
      system("PAUSE");
    } else if (input == "pins") {
      BitBoard pinMasks[2][8];
      BitBoard bb;
      chessBoard.getpinMasks(pinMasks);
      for (int t = 0; t < 2; ++t) {
        for (int i = 0; i < 8; ++i) {
          bb.set(bb.get() | pinMasks[t][i].get());
          if (!pinMasks[t][i].isEmpty()) {
            chessBoard.display(
                blackSide, bb,
                "  popCount: " + intToString(bb.populationCount()));
          }
        }
      }
      system("PAUSE");
    } else if (input == "PIECES") {
      BitBoard bb = chessBoard.getAllPIECES();
      chessBoard.display(blackSide, bb,
                         "  popCount: " + intToString(bb.populationCount()));
      system("PAUSE");
    } else if (input == "watks") {
      BitBoard bb = chessBoard.getWhiteAtks();
      chessBoard.display(blackSide, bb,
                         "  popCount: " + intToString(bb.populationCount()));
      system("PAUSE");
    } else if (input == "batks") {
      BitBoard bb = chessBoard.getBlackAtks();
      chessBoard.display(blackSide, bb,
                         "  popCount: " + intToString(bb.populationCount()));
      system("PAUSE");
    } else if (input == "kings") {
      BitBoard bb;
      bb.setSquare(chessBoard.getWhiteKing());
      bb.setSquare(chessBoard.getBlackKing());
      chessBoard.display(blackSide, bb);
      system("PAUSE");
    } else if (input == "exit") {
      // exits
      break;
    } else {
      moveFrom = chessCache.notationToTile(input);
      cBoardHLight.setSquare(moveFrom);

      moveList genMoves;
      chessBoard.generateMoves(genMoves, true);
      for (int i = 0; i < genMoves.amt; ++i) {
        if (genMoves.moves[i].move.moveFrom() == moveFrom) {
          cBoardHLight.setSquare(genMoves.moves[i].move.moveTo());
        }
      }

      system("CLS");
      EvalBar(aiEval, chessBoard);
      chessBoard.display(blackSide, cBoardHLight,
                         "  #moves: " + intToString(genMoves.amt));

      std::cout << "moveTo: ";
      std::cin >> input;
      moveTo = chessCache.notationToTile(input);

      moveList Promotes;
      bool isPromotion = false;
      for (int i = 0; i < genMoves.amt; ++i) {
        if (genMoves.moves[i].move.moveFrom() == moveFrom &&
            genMoves.moves[i].move.moveTo() == moveTo) {
          if (genMoves.moves[i].move.isPromotion()) {
            Promotes.addConstMove(genMoves.moves[i].move);
            isPromotion = true;
          }
        }
      }
      if (isPromotion) {
        std::cout << "\npromote to: \n"
                  << "[q] " << PIECES.toUnicode(PIECES.QUEEN) << " Queen\n"
                  << "[r] " << PIECES.toUnicode(PIECES.ROOK) << " Rook\n"
                  << "[b] " << PIECES.toUnicode(PIECES.BISHOP) << " Bishop\n"
                  << "[n] " << PIECES.toUnicode(PIECES.KNIGHT) << " Knight\n";
        std::cin >> input;
        toLowercase(input);
        if (input == "r") {
          chessBoard.makeMove(Promotes.moves[1].move);
          previousMoves[chessBoardPly] = Promotes.moves[1].move;
        } else if (input == "b") {
          chessBoard.makeMove(Promotes.moves[2].move);
          previousMoves[chessBoardPly] = Promotes.moves[2].move;
        } else if (input == "n") {
          chessBoard.makeMove(Promotes.moves[3].move);
          previousMoves[chessBoardPly] = Promotes.moves[3].move;
        } else {  // queen
          chessBoard.makeMove(Promotes.moves[0].move);
          previousMoves[chessBoardPly] = Promotes.moves[0].move;
        }
        ++chessBoardPly;
      } else {
        for (int i = 0; i < genMoves.amt; ++i) {
          if (genMoves.moves[i].move.moveFrom() == moveFrom &&
              genMoves.moves[i].move.moveTo() == moveTo) {
            chessBoard.makeMove(genMoves.moves[i].move);
            previousMoves[chessBoardPly] = genMoves.moves[i].move;
            ++chessBoardPly;
            break;
          }
        }
      }
    }
  } while (true);
}

void runAITest(Board &chessBoard) {
  auto start = std::chrono::steady_clock::now();

  searchRes minMaxResult;
  chessBoard.setupFen(
      "5K2/8/7P/P7/7r/5k2/8/8 w - - 0 1");  // tied game rook vs two pawns // 0<
  if (chessBoard.turn == PIECES.WHITE) {
    minMaxResult = chessBoard.oSearch(8);
  } else {
    minMaxResult = chessBoard.oSearch(8);
  }
  std::cout << "\n- endgame\n  eval: " << minMaxResult.eval;
  std::cout << "\n  nodes: " << minMaxResult.nodes;

  chessBoard.setupFen(
      "2q1nk1r/4Rp2/1ppp1P2/6Pp/3p1B2/3P3P/PPP1Q3/6K1 w");  // mate in 5
  if (chessBoard.turn == PIECES.WHITE) {
    minMaxResult = chessBoard.oSearch(6);
  } else {
    minMaxResult = chessBoard.oSearch(6);
  }
  std::cout << "\n\n- mate in 5\n  eval: " << minMaxResult.eval;
  std::cout << "\n  nodes: " << minMaxResult.nodes;

  chessBoard.setupFen(
      "6r1/p3p1rk/1p1pPp1p/q3n2R/4P3/3BR2P/PPP2QP1/7K w");  // another mate in 5
  if (chessBoard.turn == PIECES.WHITE) {
    minMaxResult = chessBoard.oSearch(7);
  } else {
    minMaxResult = chessBoard.oSearch(7);
  }
  std::cout << "\n\n- harder mate in 5\n  eval: " << minMaxResult.eval;
  std::cout << "\n  nodes: " << minMaxResult.nodes;

  chessBoard.setupFen(
      "r1bqk2r/ppppbppp/2n2n2/3Np3/2P5/5NP1/PP1PPPBP/R1BQK2R w KQkq - 5 6");  // english cpawn opening // +0.6
  if (chessBoard.turn == PIECES.WHITE) {
    minMaxResult = chessBoard.oSearch(6);
  } else {
    minMaxResult = chessBoard.oSearch(6);
  }
  std::cout << "\n\n- english Opening\n  eval: " << minMaxResult.eval;
  std::cout << "\n  nodes: " << minMaxResult.nodes;

  chessBoard.setupFen(
      "rn1qk2r/p1p2ppp/bp2pn2/3p4/1bPP4/1P3NP1/P2BPPBP/RN1QK2R b KQkq - 3 "
      "4");  // queens indian // + 0.2
  if (chessBoard.turn == PIECES.WHITE) {
    minMaxResult = chessBoard.oSearch(6);
  } else {
    minMaxResult = chessBoard.oSearch(6);
  }
  std::cout << "\n\n- queens Indian\n  eval: " << minMaxResult.eval;
  std::cout << "\n  nodes: " << minMaxResult.nodes;

  chessBoard.setupFen(
      "2rr2k1/p2qbppp/5n2/2pB4/P1P2B2/6P1/4PP1P/1R1Q1RK1 b - - 0 19");  // queens
                                                                        // indian
                                                                        // middle
                                                                        // game
                                                                        // //
                                                                        // +0.3
  if (chessBoard.turn == PIECES.WHITE) {
    minMaxResult = chessBoard.oSearch(6);
  } else {
    minMaxResult = chessBoard.oSearch(6);
  }
  std::cout << "\n\n- queens Indian Middle\n  eval: " << minMaxResult.eval;
  std::cout << "\n  nodes: " << minMaxResult.nodes;

  chessBoard.setupFen(
      "3nk2r/3q2pp/Q3b3/2R1Pp2/3p4/5N2/1p3PPP/1B4K1 w k - 1 24");  // queens
                                                                   // gambit
                                                                   // middle
                                                                   // game //
                                                                   // +0.9
  if (chessBoard.turn == PIECES.WHITE) {
    minMaxResult = chessBoard.oSearch(6);
  } else {
    minMaxResult = chessBoard.oSearch(6);
  }
  std::cout << "\n\n- queens gambit Middle\n  eval: " << minMaxResult.eval;
  std::cout << "\n  nodes: " << minMaxResult.nodes;

  auto endt = std::chrono::steady_clock::now();
  auto duration =
      std::chrono::duration_cast<std::chrono::milliseconds>(endt - start)
          .count();
  std::cout << "\n\n - total time taken: " << intToString(duration) << '\n';
  system("PAUSE");
}

int main() {
  Board chessBoard;
  std::string input = " ";
  allowEmojis();
  do {
    system("CLS");
    std::cout << " " << PIECES.toUnicode(PIECES.PAWN) << " CHESS MENU "
              << PIECES.toUnicode(PIECES.PAWN);
    setTxtColor(chessColors.greyLetCol);
    std::cout << " V7.4";  // VERSION ~1400 elo
    setTxtColor(15);
    std::cout << "\n______________________\n";
    std::cout << "\n[p] Play";
    setTxtColor(chessColors.greyLetCol);
    std::cout << " (input \"help\" for commands) ";
    setTxtColor(15);
    std::cout << "\n[v] Visuals"
              << "\n[t] Performance Test"
              << "\n[a] AI Test"
              << "\n[f] Custom Fen";
    setTxtColor(chessColors.greyLetCol);
    std::cout << " (starting position) ";
    setTxtColor(15);
    std::cout << "\n[e] Exit" << '\n';

    std::cin >> input;
    toLowercase(input);

    if (input == "p") {
      startGame(chessBoard);
    } else if (input == "v") {
      std::cout << "[1] [standard]\n";
      std::cout << "[2] blue vs red [no checkers]\n";
      std::cout << "[3] test all colors\n";
      std::cin >> input;
      if (input == "1") {
        chessColors.wBlackCol = 128;  // Gray background black letter
        chessColors.wWhiteCol = 143;  // Gray background white letter
        chessColors.bWhiteCol = 15;   // Black background white latter
        chessColors.bBlackCol = 8;    // Black background gray letter
      } else if (input == "2") {
        chessColors.wBlackCol = 12;
        chessColors.wWhiteCol = 11;
        chessColors.bWhiteCol = 11;
        chessColors.bBlackCol = 12;
      } else if (input == "3") {
        for (int i = 0; i < 16; ++i) {
          for (int x = 0; x < 16; ++x) {
            int color = i * 16 + x;
            setTxtColor(color);
            std::cout << color;
            if (color <= 9) {
              std::cout << "  ";
            } else if (color <= 99) {
              std::cout << " ";
            }
            setTxtColor(15);
          }
          std::cout << '\n';
        }
        system("PAUSE");
      }
    } else if (input == "t") {
      // perft test
      perftTest(chessBoard);
    } else if (input == "a") {
      runAITest(chessBoard);
    } else if (input == "f") {
      chessBoard.setupFen(askFen());
      std::cout << "\n\n";
    } else if (input == "e") {
      // exit
      break;
    } else {
      std::cout << '"' << input << '"' << " invalid input;\n";
      system("PAUSE");
    };
  } while (input != "e");
  return 0;
}

// VISUALS!!
void allowEmojis() {
  // Sets Encoding to UTF8 - for chess PIECES ASCII
  SetConsoleOutputCP(CP_UTF8);  // for visuals
}

void setTxtColor(int colorValue) {
  HANDLE hConsole;
  hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
  SetConsoleTextAttribute(hConsole, colorValue);
}

std::string intToString(int num) {
  if (num == 0) return "0";
  return num < 0 ? "-" + std::to_string(-num) : std::to_string(num);
}

void toLowercase(std::string &input) {
  for (char &c : input) {
    if (c >= 'A' && c <= 'Z') {
      c += 32;  // Convert uppercase letter to lowercase
    }
  }
}

void printUint32Binary(uint32_t num) {
  for (int i = sizeof(num) * 8 - 1; i >= 0; --i) {
    std::cout << ((num >> i) & 1);
    if (i % 4 == 0) std::cout << " ";
  }
}

// just reverses/like/this --> sesrever/ekil/siht
std::string invertFen(const std::string STR) {
  std::string reversedStr = "";
  std::string word = "";
  for (char c : STR) {
    if (c != '/') {
      word += c;
    } else {
      for (int i = static_cast<int>(word.length()) - 1; i >= 0; i--) {
        reversedStr += word[i];
      }
      reversedStr += '/';
      word = "";
    }
  }
  for (int i = static_cast<int>(word.length()) - 1; i >= 0; i--) {
    reversedStr += word[i];
  }
  return reversedStr;
}

// butterfly arrays
// Function to allocate memory for moveHHistory array
uint16_t ***allocateMoveHHistory() {
  uint16_t ***moveHHistory = new uint16_t *
      *[2];  // Allocate memory for the first dimension (white or black)
  for (int i = 0; i < 2; ++i) {
    moveHHistory[i] =
        new uint16_t *[64];  // Allocate memory for the second dimension
    for (int j = 0; j < 64; ++j) {
      moveHHistory[i][j] =
          new uint16_t[64];  // Allocate memory for the third dimension
      // Initialize elements to zero
      for (int k = 0; k < 64; ++k) {
        moveHHistory[i][j][k] = 0;
      }
    }
  }
  return moveHHistory;
}

// Function to deallocate memory for moveHHistory array
void deallocateMoveHHistory(uint16_t ***moveHHistory) {
  for (int i = 0; i < 2; ++i) {
    for (int j = 0; j < 64; ++j) {
      delete[] moveHHistory[i][j];  // Deallocate memory for the third dimension
    }
    delete[] moveHHistory[i];  // Deallocate memory for the second dimension
  }
  delete[] moveHHistory;  // Deallocate memory for the first dimension
}

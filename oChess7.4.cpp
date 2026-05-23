/*=====================================
Programmer: Airzy T
Assignment: FINAL PROJECT CHESS

Description (I.P.O):
    Input:
        - menu driven program
          enter letter inside [here] to select options

          // inside playing
          * chess is turn based and white always moves first
          example input: e2 e4
                      or e2 enter e4 - to see moves beforehand

    Process:
        // move generation
        - every possible move is calculated beforehand and stored in big arrays
          for fast access (caches)
        - convert inputs into a number corresponding to the tile in the 8x8 board
          e.g. a1 -> 0, h8 -> 63
        - find and generate moves using input by looking it up in the caches

        // bot algorithm
        - uses alpha-beta pruning (recursive) to play ahead and find best moves
        - everything is optimized at the binary level; moves and pieces stored
          smaller than an integer

    Output:
        // move generation
        - displays all legal moves as highlights
        // board
        - top bar shows an evaluation bar
        - uses unicode chess piece characters

Assumptions:
    - user is expected to enter valid inputs
    - public fields used instead of getters/setters for faster lookup
    - ++i pre-increment used for minor speed benefit
    - minimized branching; risks more memory for speed
    - needs at least 0.9 - 2 MB for pseudo-legal move storage

Sources:
    https://bitwisecmd.com/
    https://www.rapidtables.com/convert/number/decimal-to-binary.html
    https://lichess.org/analysis
    https://lichess.org/editor
    https://www.chessprogramming.org/
    https://www.youtube.com/@chessprogramming591
    https://github.com/SebLague/Chess-Coding-Adventure
=======================================*/

#include <algorithm>
#include <chrono>
#include <iostream>
#include <random>
#include <string>
#include <windows.h>

void setTxtColor(int colorValue);
void printUint32Binary(uint32_t num);
void toLowercase(std::string &input);
void allowEmojis();

uint16_t ***allocateMoveHHistory();
void deallocateMoveHHistory(uint16_t ***moveHHistory);

std::string intToString(int num);
std::string invertFen(const std::string STR);

// 1ULL = 00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000001
class BitBoard {
 private:
  uint64_t bitBoard = 0;

 public:
  inline void setSquare(uint8_t square)   { bitBoard |=  (1ULL << square); }
  inline void unSetSquare(uint8_t square) { bitBoard &= ~(1ULL << square); }
  inline bool isSet(uint8_t square) const { return (bitBoard >> square) & 1; }
  inline void clearBoard()                { bitBoard = 0; }
  inline uint64_t get() const             { return bitBoard; }
  inline void set(uint64_t bb)            { bitBoard = bb; }
  inline bool isEmpty() const             { return bitBoard == 0; }

  // Population count (Hamming weight)
  inline int populationCount() const {
    uint64_t x = bitBoard;
    x = x - ((x >> 1) & 0x5555555555555555ULL);
    x = (x & 0x3333333333333333ULL) + ((x >> 2) & 0x3333333333333333ULL);
    x = (x + (x >> 4)) & 0x0F0F0F0F0F0F0F0FULL;
    x = x + (x >> 8);
    x = x + (x >> 16);
    x = x + (x >> 32);
    return x & 0x7F;
  }

  inline int populationCountBAND(uint64_t x2) const {
    uint64_t x = bitBoard & x2;
    x = x - ((x >> 1) & 0x5555555555555555ULL);
    x = (x & 0x3333333333333333ULL) + ((x >> 2) & 0x3333333333333333ULL);
    x = (x + (x >> 4)) & 0x0F0F0F0F0F0F0F0FULL;
    x = x + (x >> 8);
    x = x + (x >> 16);
    x = x + (x >> 32);
    return x & 0x7F;
  }

  BitBoard() {}
  BitBoard(uint64_t &newbb) { bitBoard = newbb; }
};

class PieceCache {
 private:
  static constexpr uint8_t TYPE_MASK  = 0b0111;
  static constexpr uint8_t COLOR_MASK = 0b1000;

  const std::string KING_UNICODE   = "\xE2\x99\x9A";
  const std::string QUEEN_UNICODE  = "\xE2\x99\x9B";
  const std::string ROOK_UNICODE   = "\xE2\x99\x9C";
  const std::string BISHOP_UNICODE = "\xE2\x99\x9D";
  const std::string KNIGHT_UNICODE = "\xE2\x99\x9E";
  const std::string PAWN_UNICODE   = "\xE2\x99\x99";
  const std::string EMPTY_STRING   = " ";
  const std::string UNKNOWN_TYPE   = "?";

 public:
  // Piece types — last 3 bits
  const uint8_t EMPTY  = 0;
  const uint8_t PAWN   = 1;
  const uint8_t KNIGHT = 2;
  const uint8_t BISHOP = 3;
  const uint8_t ROOK   = 4;
  const uint8_t QUEEN  = 5;
  const uint8_t KING   = 6;

  // Colors — bit 4
  const uint8_t WHITE = 0;
  const uint8_t BLACK = 8;

  // Predefined pieces
  const uint8_t WPAWN   = PAWN   | WHITE;
  const uint8_t WKNIGHT = KNIGHT | WHITE;
  const uint8_t WBISHOP = BISHOP | WHITE;
  const uint8_t WROOK   = ROOK   | WHITE;
  const uint8_t WQUEEN  = QUEEN  | WHITE;
  const uint8_t WKING   = KING   | WHITE;

  const uint8_t BPAWN   = PAWN   | BLACK;
  const uint8_t BKNIGHT = KNIGHT | BLACK;
  const uint8_t BBISHOP = BISHOP | BLACK;
  const uint8_t BROOK   = ROOK   | BLACK;
  const uint8_t BQUEEN  = QUEEN  | BLACK;
  const uint8_t BKING   = KING   | BLACK;

  inline uint8_t type(const uint8_t piece)  const { return piece & TYPE_MASK; }
  inline uint8_t color(const uint8_t piece) const { return piece & COLOR_MASK; }

  const std::string toUnicode(const uint8_t pieceType) const {
    switch (pieceType) {
      case 0: return EMPTY_STRING;
      case 1: return PAWN_UNICODE;
      case 2: return KNIGHT_UNICODE;
      case 3: return BISHOP_UNICODE;
      case 4: return ROOK_UNICODE;
      case 5: return QUEEN_UNICODE;
      case 6: return KING_UNICODE;
      default: return UNKNOWN_TYPE;
    }
  }
};

const PieceCache PIECES;

struct MoveCache {
  static constexpr unsigned short fromTileMask      = 0b0000000000111111;
  static constexpr unsigned short toTileMask        = 0b0000111111000000;
  static constexpr unsigned short flagMask          = 0b1111000000000000;
  static constexpr unsigned short inverseFlagMask   = 0b0000111111111111;

  static constexpr uint8_t NoFlag               = 0b0000;
  static constexpr uint8_t EnPassantCaptureFlag = 0b0001;
  static constexpr uint8_t CastleFlag           = 0b0010;
  static constexpr uint8_t PawnTwoUpFlag        = 0b0011;
  static constexpr uint8_t PromoteToQueenFlag   = 0b0100;
  static constexpr uint8_t PromoteToKnightFlag  = 0b0101;
  static constexpr uint8_t PromoteToRookFlag    = 0b0110;
  static constexpr uint8_t PromoteToBishopFlag  = 0b0111;

  static constexpr unsigned short Null = 0;
};

// Compact 16-bit move: ffffttttttssssss
// Bits  0-5:  start square
// Bits  6-11: target square
// Bits 12-15: flag
class Move {
 private:
  unsigned short moveValue = 0;
  uint8_t flag() const { return moveValue >> 12; }

 public:
  inline uint8_t moveFrom() const { return moveValue & MoveCache::fromTileMask; }
  inline uint8_t moveTo()   const { return (moveValue & MoveCache::toTileMask) >> 6; }

  inline bool isNull()         const { return moveValue == MoveCache::Null; }
  inline bool isCastling()     const { return flag() == MoveCache::CastleFlag; }
  inline bool isPawnTwoUp()    const { return flag() == MoveCache::PawnTwoUpFlag; }
  inline bool isEnPassant()    const { return flag() == MoveCache::EnPassantCaptureFlag; }
  inline bool promoteQueen()   const { return flag() == MoveCache::PromoteToQueenFlag; }
  inline bool promoteRook()    const { return flag() == MoveCache::PromoteToRookFlag; }
  inline bool promoteBishop()  const { return flag() == MoveCache::PromoteToBishopFlag; }
  inline bool promoteKnight()  const { return flag() == MoveCache::PromoteToKnightFlag; }
  inline bool isDoublePawnPush() const { return flag() == MoveCache::PawnTwoUpFlag; }

  inline bool isPromotion() const {
    uint8_t fg = flag();
    return fg == MoveCache::PromoteToQueenFlag  ||
           fg == MoveCache::PromoteToRookFlag   ||
           fg == MoveCache::PromoteToBishopFlag ||
           fg == MoveCache::PromoteToKnightFlag;
  }

  inline void setFlag(uint8_t newFlag) {
    moveValue &= MoveCache::inverseFlagMask;
    moveValue |= (newFlag << 12);
  }

  inline void clearMove() { moveValue = MoveCache::Null; }

  Move() { moveValue = MoveCache::Null; }
  Move(uint8_t fromTile, uint8_t toTile)              { moveValue = fromTile | (toTile << 6); }
  Move(uint8_t fromTile, uint8_t toTile, uint8_t flag){ moveValue = fromTile | (toTile << 6) | (flag << 12); }
};

struct globalColors {
  const uint8_t hlightCol    = 240;
  uint8_t wBlackCol          = 128;
  uint8_t wWhiteCol          = 143;
  uint8_t bWhiteCol          = 15;
  uint8_t bBlackCol          = 8;
  const uint8_t greyLetCol   = 8;

  const uint8_t prevMBlackCol = 32;
  const uint8_t prevMWhiteCol = 47;

  const uint8_t checkBlackCol = 207;
  const uint8_t checkWhiteCol = 192;
};

globalColors chessColors;

class PreComputedCache {
 private:
  uint8_t rowColValues[8][8];

 public:
  uint8_t preComputedRows[64];
  uint8_t preComputedCols[64];

  static constexpr int directionOffsets[8] = {
       7,   // [0] up-right  (diagonal)
       9,   // [1] up-left   (diagonal)
      -9,   // [2] down-right(diagonal)
      -7,   // [3] down-left (diagonal)
       8,   // [4] up        (orthogonal)
      -8,   // [5] down      (orthogonal)
      -1,   // [6] right     (orthogonal)
       1,   // [7] left      (orthogonal)
  };

  Move bPawnMoves[64][8];
  Move wPawnMoves[64][8];
  Move knightMoves[64][8];
  Move kingMoves[64][8];

  Move bishopMoves[4][64][7];
  Move rookMoves[4][64][7];

  Move whiteKingSideCastle;
  Move whiteQueenSideCastle;
  Move blackKingSideCastle;
  Move blackQueenSideCastle;

  BitBoard rays[64][64];
  BitBoard rays1Extra[64][64];

  unsigned int distances[64][64];

  const uint8_t whiteKingRook  = 7;
  const uint8_t whiteQueenRook = 0;
  const uint8_t blackKingRook  = 63;
  const uint8_t blackQueenRook = 56;

  const uint8_t whiteKingRookCastleTo  = 5;
  const uint8_t whiteQueenRookCastleTo = 3;
  const uint8_t blackKingRookCastleTo  = 61;
  const uint8_t blackQueenRookCastleTo = 59;

  const uint8_t whiteKingCastleTo  = 6;
  const uint8_t whiteQueenCastleTo = 2;
  const uint8_t blackKingCastleTo  = 62;
  const uint8_t blackQueenCastleTo = 58;

  const uint8_t whiteQueenCastleVacant = 1;
  const uint8_t blackQueenCastleVacant = 57;

  std::string startingFen = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";
  std::string notatedTiles[64];

  uint8_t notationToTile(std::string &notation) const {
    int fileNum = notation[0] - 'a';
    int rankNum = notation[1] - '1';
    if (fileNum >= 0 && fileNum <= 7 && rankNum >= 0 && rankNum <= 7)
      return rowColValues[rankNum][fileNum];
    return 0;
  }

  std::string tileToNotation(const uint8_t &tile) const { return notatedTiles[tile]; }

  static constexpr int pawnValue   = 100 * 10;
  static constexpr int knightValue = 300 * 10;
  static constexpr int bishopValue = 300 * 10;
  static constexpr int rookValue   = 500 * 10;
  static constexpr int queenValue  = 900 * 10;

  static constexpr int evalPositiveInf =  1000000;
  static constexpr int evalNegativeInf = -1000000;
  static constexpr int evalWhiteWins   =  900000;
  static constexpr int evalWhiteLoss   = -900000;

  int centerPST[64] = {
      -10, 0,  1,  2,  2,  1,  0,  -10,
        0,10, 10, 15, 15, 10, 10,    0,
        0,15, 30, 35, 35, 30, 15,    1,
        2,25, 40, 50, 50, 40, 25,    2,
        2,25, 40, 50, 50, 40, 25,    2,
        0,15, 30, 35, 35, 30, 15,    1,
        0,10, 10, 15, 15, 10, 10,    0,
      -10, 0,  1,  2,  2,  1,  0,  -10,
  };

  int statOrdPST[64] = {
      0, 0, 1, 1,  1,  1, 0, 0,
      0, 1, 2, 2,  2,  2, 1, 0,
      1, 4, 9, 6,  6,  9, 4, 1,
      2, 8, 8,10, 10,  8, 8, 2,
      2, 8, 8,10, 10,  8, 8, 2,
      1, 4, 9, 6,  6,  9, 4, 1,
      0, 1, 2, 2,  2,  2, 1, 0,
      0, 0, 1, 1,  1,  1, 0, 0,
  };

  int pawnPST[2][64] = {
        0,   0,   0,   0,   0,   0,   0,   0,
       90,  90,  90,  90,  90,  90,  90,  90,
       10,  10,  20,  40,  40,  20,  10,  10,
        0,   0,  20,  40,  40,   0,   0,   0,
       -4,  -2,  20,  40,  40,  -5,  -8,  -8,
       -4,   0,   5, -10,   0,  -5,   5,   3,
       -5,   0, -10, -30, -30,   0,   5,   2,
        0,   0,   0,   0,   0,   0,   0,   0,
  };

  int pawnEndPST[2][64] = {
        0,   0,   0,   0,   0,   0,   0,   0,
      400, 400, 400, 400, 400, 400, 400, 400,
       80,  80,  80,  80,  80,  80,  80,  80,
       30,  30,  30,  20,  20,  30,  30,  30,
       20,  20,  20,  20,  20,  20,  20,  20,
       15,  15,  15,  15,  15,  15,  15,  15,
        0,   0,   0,   0,   0,   0,   0,   0,
        0,   0,   0,   0,   0,   0,   0,   0,
  };

  int horsePST[2][64] = {
      -90, -30, -30, -30, -30, -30, -30, -90,
      -40, -20,  15,   0,   0,  15, -20, -40,
      -30,   0,  20,  20,  20,  20,   0, -30,
      -30,   5,  15,  20,  20,  15,   5, -30,
      -30,   0,  15,  20,  20,  15,   0, -30,
      -30,   5,  20,   5,   5,  20,   5, -30,
      -40, -20,   0,  10,  10,   0, -20, -40,
      -90, -40, -30, -30, -30, -30, -40, -90,
  };

  int bishopPST[2][64] = {
      -30, -10, -10, -10, -10, -10, -10, -30,
      -10,  15,   0,   0,   0,   0,  15, -10,
      -10,   0,   5,  10,  10,   5,   0, -10,
      -10,   5,  10,  10,  10,  10,   5, -10,
      -10,   5,  10,  10,  10,  10,   5, -10,
      -10,  10,  10,   5,   5,  10,  10, -10,
      -10,  15,  10,   0,   0,  10,  15, -10,
      -30, -20, -20, -10, -10, -20, -20, -30,
  };

  int rookPST[2][64] = {
       -5,  -5,   0,   0,   0,   0,  -5,  -5,
       -5,  10,  10,  10,  10,  10,  10,  -5,
       -5,   0,   0,   0,   0,   0,   0,  -5,
       -5,   0,   0,   0,   0,   0,   0,  -5,
       -5,   0,   0,   0,   0,   0,   0,  -5,
        0,   0,   0,   0,   0,   0,   0,   0,
       -5,   0,   5,   8,   8,   5,   0,  -5,
       -5,  -5,  -5,   6,   6,   5,  -5,  -5,
  };

  int kingPST[2][64] = {
      -90, -90, -90, -90, -90, -90, -90, -90,
      -90, -90, -90, -90, -90, -90, -90, -90,
      -90, -90, -90, -90, -90, -90, -90, -90,
      -90, -90, -90, -90, -90, -90, -90, -90,
      -60, -60, -60, -60, -60, -60, -60, -60,
      -50, -50, -50, -40, -40, -40, -50, -50,
      -40, -30, -50, -40, -40, -50, -30, -40,
       -5,   5, -10, -20, -20, -20,   5,  -5,
      -10,   7,   5, -30, -10, -10,   5, -16,
  };

  BitBoard checkerBB;
  inline bool colorOfSquare(int tile) const { return (checkerBB.get() >> tile) & 1; }

  inline int value(const uint8_t &piece) const {
    switch (PIECES.type(piece)) {
      case 1: return 10;
      case 2: return 30;
      case 3: return 32;
      case 4: return 50;
      case 5: return 90;
      default: return 0;
    }
  }

  uint64_t zobristLookup[64][2][6];
  uint64_t zobristCastling[16];
  uint64_t zobristEnPassant[9];
  uint64_t blackTurnZobrist;

  inline uint64_t RandUINT64(uint64_t seed) const {
    std::mt19937_64 gen(seed);
    return gen();
  }

  bool inBounds(int tile) const { return tile >= 0 && tile < 64; }

  PreComputedCache() {
    whiteKingSideCastle  = Move(4, whiteKingCastleTo,  MoveCache::CastleFlag);
    whiteQueenSideCastle = Move(4, whiteQueenCastleTo, MoveCache::CastleFlag);
    blackKingSideCastle  = Move(60, blackKingCastleTo,  MoveCache::CastleFlag);
    blackQueenSideCastle = Move(60, blackQueenCastleTo, MoveCache::CastleFlag);

    checkerBB.set(0x55aa55aa55aa55aa);

    uint64_t seedIncrement = 31;
    for (int i = 0; i < 64; ++i) {
      int row = i / 8;
      int col = i % 8;
      int i2  = ((7 - row) * 8) + col;
      pawnPST[1][i]    = pawnPST[0][i2];
      horsePST[1][i]   = horsePST[0][i2];
      bishopPST[1][i]  = bishopPST[0][i2];
      kingPST[1][i]    = kingPST[0][i2];
      rookPST[1][i]    = rookPST[0][i2];
      pawnEndPST[1][i] = pawnEndPST[0][i2];

      for (int t = 0; t < 6; ++t) {
        zobristLookup[i][0][t] = RandUINT64(++seedIncrement);
        zobristLookup[i][1][t] = RandUINT64(++seedIncrement);
      }
    }
    for (int i = 0; i < 16; ++i) zobristCastling[i]  = RandUINT64(++seedIncrement);
    for (int f = 0; f <  9; ++f) zobristEnPassant[f]  = RandUINT64(++seedIncrement);
    blackTurnZobrist = RandUINT64(++seedIncrement);

    for (int i = 0; i < 64; ++i) {
      int row = i / 8;
      int col = i % 8;
      rowColValues[row][col] = i;
      preComputedRows[i] = row;
      preComputedCols[i] = col;

      std::string notation = "";
      notation += 'a' + col;
      notation += '1' + row;
      notatedTiles[i] = notation;

      // White pawns
      if (row < 7) {
        if (row == 6) {
          wPawnMoves[i][4] = Move(i, i + 8, MoveCache::PromoteToQueenFlag);
          wPawnMoves[i][5] = Move(i, i + 8, MoveCache::PromoteToRookFlag);
          wPawnMoves[i][6] = Move(i, i + 8, MoveCache::PromoteToBishopFlag);
          wPawnMoves[i][7] = Move(i, i + 8, MoveCache::PromoteToKnightFlag);
        }
        wPawnMoves[i][0] = Move(i, i + 8);
        if (col < 7) wPawnMoves[i][3] = Move(i, i + 9);
        if (col > 0) wPawnMoves[i][2] = Move(i, i + 7);
        if (row == 1) wPawnMoves[i][1] = Move(i, i + 16, MoveCache::PawnTwoUpFlag);
      }

      // Black pawns
      if (row > 0) {
        if (row == 1) {
          bPawnMoves[i][4] = Move(i, i - 8, MoveCache::PromoteToQueenFlag);
          bPawnMoves[i][5] = Move(i, i - 8, MoveCache::PromoteToRookFlag);
          bPawnMoves[i][6] = Move(i, i - 8, MoveCache::PromoteToBishopFlag);
          bPawnMoves[i][7] = Move(i, i - 8, MoveCache::PromoteToKnightFlag);
        }
        bPawnMoves[i][0] = Move(i, i - 8);
        if (col < 7) bPawnMoves[i][3] = Move(i, i - 7);
        if (col > 0) bPawnMoves[i][2] = Move(i, i - 9);
        if (row == 6) bPawnMoves[i][1] = Move(i, i - 16, MoveCache::PawnTwoUpFlag);
      }

      // Knights
      if (row < 6 && col > 0) knightMoves[i][0] = Move(i, i + 15);
      if (row < 6 && col < 7) knightMoves[i][1] = Move(i, i + 17);
      if (row < 7 && col > 1) knightMoves[i][2] = Move(i, i + 6);
      if (row < 7 && col < 6) knightMoves[i][3] = Move(i, i + 10);
      if (row > 0 && col > 1) knightMoves[i][4] = Move(i, i - 10);
      if (row > 0 && col < 6) knightMoves[i][5] = Move(i, i - 6);
      if (row > 1 && col > 0) knightMoves[i][6] = Move(i, i - 17);
      if (row > 1 && col < 7) knightMoves[i][7] = Move(i, i - 15);

      // Bishops
      if (col > 0) {
        int dfar = 0;
        for (int j = 1; j < 8; ++j) {
          int dest = i + j * directionOffsets[0];
          int dCol = dest % 8;
          if (!inBounds(dest) || dCol < 0) break;
          bishopMoves[0][i][dfar++] = Move(i, dest);
          if (dCol == 0) break;
        }
      }
      if (col < 7) {
        int dfar = 0;
        for (int j = 1; j < 8; ++j) {
          int dest = i + j * directionOffsets[1];
          int dCol = dest % 8;
          if (!inBounds(dest) || dCol > 7) break;
          bishopMoves[1][i][dfar++] = Move(i, dest);
          if (dCol == 7) break;
        }
      }
      if (col > 0) {
        int dfar = 0;
        for (int j = 1; j < 8; ++j) {
          int dest = i + j * directionOffsets[2];
          int dCol = dest % 8;
          if (!inBounds(dest) || dCol < 0) break;
          bishopMoves[2][i][dfar++] = Move(i, dest);
          if (dCol == 0) break;
        }
      }
      if (col < 7) {
        int dfar = 0;
        for (int j = 1; j < 8; ++j) {
          int dest = i + j * directionOffsets[3];
          int dCol = dest % 8;
          if (!inBounds(dest) || dCol > 7) break;
          bishopMoves[3][i][dfar++] = Move(i, dest);
          if (dCol == 7) break;
        }
      }

      // Rooks
      {
        int dfar = 0;
        for (int j = 1; j < 8; ++j) {
          int dest = i + j * directionOffsets[4];
          if (!inBounds(dest)) break;
          rookMoves[0][i][dfar++] = Move(i, dest);
        }
      }
      {
        int dfar = 0;
        for (int j = 1; j < 8; ++j) {
          int dest = i + j * directionOffsets[5];
          if (!inBounds(dest)) break;
          rookMoves[1][i][dfar++] = Move(i, dest);
        }
      }
      if (col > 0) {
        int dfar = 0;
        for (int j = 1; j < 8; ++j) {
          int dest = i + j * directionOffsets[6];
          int dCol = dest % 8;
          if (!inBounds(dest) || dCol < 0) break;
          rookMoves[2][i][dfar++] = Move(i, dest);
          if (dCol == 0) break;
        }
      }
      if (col < 7) {
        int dfar = 0;
        for (int j = 1; j < 8; ++j) {
          int dest = i + j * directionOffsets[7];
          int dCol = dest % 8;
          if (!inBounds(dest) || dCol > 7) break;
          rookMoves[3][i][dfar++] = Move(i, dest);
          if (dCol == 7) break;
        }
      }

      // King
      if (row < 7) kingMoves[i][0] = Move(i, i + 8);
      if (row > 0) kingMoves[i][1] = Move(i, i - 8);
      if (col > 0) kingMoves[i][2] = Move(i, i - 1);
      if (col < 7) kingMoves[i][3] = Move(i, i + 1);
      if (col > 0 && row < 7) kingMoves[i][4] = Move(i, i + 7);
      if (col < 7 && row < 7) kingMoves[i][5] = Move(i, i + 9);
      if (col > 0 && row > 0) kingMoves[i][6] = Move(i, i - 9);
      if (col < 7 && row > 0) kingMoves[i][7] = Move(i, i - 7);
    }

    // Ray tables
    for (int i = 0; i < 64; ++i) {
      for (int j = 0; j < 64; ++j) {
        distances[i][j] = abs(preComputedRows[i] - preComputedRows[j]) +
                          abs(preComputedCols[i] - preComputedCols[j]) * 2;

        BitBoard &bb  = rays[i][j];
        BitBoard &bb2 = rays1Extra[i][j];
        bb.setSquare(i);

        bool found = false;
        for (int d = 0; d < 4 && !found; ++d) {
          for (int r = 0; r < 7; ++r) {
            const Move &m = rookMoves[d][i][r];
            if (m.isNull()) break;
            if (m.moveTo() == j) {
              found = true;
              if (r < 6 && !rookMoves[d][i][r + 1].isNull())
                bb2.setSquare(rookMoves[d][i][r + 1].moveTo());
              for (int r2 = r; r2 >= 0; --r2) {
                bb.setSquare(rookMoves[d][i][r2].moveTo());
                bb2.setSquare(rookMoves[d][i][r2].moveTo());
              }
              break;
            }
          }
        }
        for (int d = 0; d < 4 && !found; ++d) {
          for (int r = 0; r < 7; ++r) {
            const Move &m = bishopMoves[d][i][r];
            if (m.isNull()) break;
            if (m.moveTo() == j) {
              found = true;
              if (r < 6 && !bishopMoves[d][i][r + 1].isNull())
                bb2.setSquare(bishopMoves[d][i][r + 1].moveTo());
              for (int r2 = r; r2 >= 0; --r2) {
                bb.setSquare(bishopMoves[d][i][r2].moveTo());
                bb2.setSquare(bishopMoves[d][i][r2].moveTo());
              }
              break;
            }
          }
        }
        bb.setSquare(j);
        bb2.setSquare(j);
      }
    }
  }
};

const PreComputedCache chessCache;

struct movePair {
  int score = 0;
  Move move;
};

struct moveList {
  movePair moves[218];
  uint8_t  amt = 0;

  void addConstMove(const Move &m) {
    if (!m.isNull()) {
      moves[amt].move = m;
      ++amt;
    }
  }
};

class pieceList {
 private:
  uint8_t pieceMap[64] = {0};
 public:
  uint8_t PIECES[10] = {0};
  uint8_t amt = 0;

  inline void addAtTile(int tile) {
    PIECES[amt] = tile;
    pieceMap[tile] = amt;
    ++amt;
  }

  inline void removeAtTile(int tile) {
    int idx = pieceMap[tile];
    PIECES[idx] = PIECES[amt - 1];
    pieceMap[PIECES[amt - 1]] = idx;
    if (amt > 0) --amt;
  }

  inline void MovePiece(int from, int to) {
    PIECES[pieceMap[from]] = to;
    pieceMap[to] = pieceMap[from];
  }

  inline void clear() {
    for (uint8_t i = 0; i < amt; ++i) pieceMap[PIECES[i]] = 0;
    amt = 0;
  }

  pieceList() { clear(); }
};

class repetitionStack {
 private:
  static const int maxSize = 400;
  uint64_t repetitionHistory[maxSize] = {0};
  int top = 0;

 public:
  void push(uint64_t zkey) { repetitionHistory[++top] = zkey; }
  void pop()               { --top; }  // was a no-op bug: repetitionHistory[top--]

  bool isThreeFold() const {
    for (int i = 0; i < top; ++i) {
      uint64_t key = repetitionHistory[i];
      uint8_t count = 0;
      for (int j = 0; j < i; ++j) {
        if (key == repetitionHistory[j] && ++count > 1) return true;
      }
    }
    return false;
  }

  void clear() {
    for (int i = 0; i < top; ++i) repetitionHistory[i] = 0;
    top = 0;
  }
};

class gameStateStack {
 private:
  static const int maxSize = 400;
  uint32_t gameStateHistory[maxSize] = {0};
  int top;

 public:
  gameStateStack() : top(-1) {}

  bool isEmpty() const { return top == -1; }
  bool isFull()  const { return top == maxSize - 1; }

  void push(uint32_t gameState) {
    if (isFull()) { std::cout << "gameStack Overflow\n"; system("PAUSE"); return; }
    gameStateHistory[++top] = gameState;
  }

  uint32_t pop() {
    if (isEmpty()) return 0;
    return gameStateHistory[top--];
  }

  uint32_t peek() const {
    if (isEmpty()) return 0;
    return gameStateHistory[top];
  }

  void clear() {
    for (int i = 0; i < top; ++i) gameStateHistory[i] = 0;
  }
};

struct searchRes {
  Move    m;
  int     eval           = 0;
  int     nodes          = 0;
  int     depth          = 0;
  int     depthExtended  = 0;
  uint8_t mateIn         = 0;
};

class Board {
 private:
  const unsigned short whiteCastleKingsideMask  = 0b1111111111111110;
  const unsigned short whiteCastleQueensideMask = 0b1111111111111101;
  const unsigned short blackCastleKingsideMask  = 0b1111111111111011;
  const unsigned short blackCastleQueensideMask = 0b1111111111110111;

  const unsigned short whiteCastleKingsideBit   = 0b0000000000000001;
  const unsigned short whiteCastleQueensideBit  = 0b0000000000000010;
  const unsigned short blackCastleKingsideBit   = 0b0000000000000100;
  const unsigned short blackCastleQueensideBit  = 0b0000000000001000;

  const unsigned short castlingMASK   = 0b1111;
  const unsigned short whiteCastleMask = whiteCastleKingsideMask & whiteCastleQueensideMask;
  const unsigned short blackCastleMask = blackCastleKingsideMask & blackCastleQueensideMask;

  Move prevMove;

  uint8_t whiteKing = 0;
  uint8_t blackKing = 0;

  int plyCount        = 0;
  int fiftyMoveCounter = 0;
  int oppTurn         = PIECES.BLACK;
  int oppTurnIndex    = 0;
  int turnIndex       = 1;

  pieceList pawns[2];
  pieceList knights[2];
  pieceList bishops[2];
  pieceList rooks[2];
  pieceList queens[2];

  gameStateStack gameStateHistory;
  repetitionStack repHistory;

  uint32_t currentGameState = 0;
  inline bool whiteKingSideCastle()  const { return currentGameState & whiteCastleKingsideBit; }
  inline bool whiteQueenSideCastle() const { return currentGameState & whiteCastleQueensideBit; }
  inline bool blackKingSideCastle()  const { return currentGameState & blackCastleKingsideBit; }
  inline bool blackQueenSideCastle() const { return currentGameState & blackCastleQueensideBit; }

  BitBoard allPIECES;
  BitBoard whiteAtks;
  BitBoard blackAtks;
  BitBoard checkRay;
  BitBoard blockRay;
  BitBoard pinMasks[2][8];
  bool pinExistInPosition = false;

  unsigned int board[64];

  inline void deleteTile(const uint8_t &index) {
    board[index] = PIECES.EMPTY;
    allPIECES.unSetSquare(index);
  }

  inline uint8_t getEnPassantFile() const { return (currentGameState >> 4) & 15; }

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
    gameStateHistory.clear();
    repHistory.clear();
    for (int i = 0; i < 64; ++i) board[i] = PIECES.EMPTY;
    for (int i = 0; i < 2; ++i) {
      pawns[i].clear();
      knights[i].clear();
      bishops[i].clear();
      rooks[i].clear();
      queens[i].clear();
    }
    whiteKing    = 0;
    blackKing    = 0;
    plyCount     = 0;
    turn         = PIECES.WHITE;
    oppTurnIndex = 0;
    turnIndex    = 1;
    oppTurn      = PIECES.BLACK;
  }

  bool genQuiets  = true;
  unsigned int checkCount = 0;
  uint64_t zobristKey = 0;

 public:
  int turn = PIECES.WHITE;

  uint16_t ***moveHHistory = allocateMoveHHistory();

  void getpinMasks(BitBoard pmasks[2][8]) const {
    for (int d = 0; d < 8; ++d) {
      pmasks[0][d] = pinMasks[0][d];
      pmasks[1][d] = pinMasks[1][d];
    }
  }

  BitBoard getcray()         const { return checkRay; }
  BitBoard getAllPIECES()     const { return allPIECES; }
  BitBoard getWhiteAtks()    const { return whiteAtks; }
  BitBoard getBlackAtks()    const { return blackAtks; }
  pieceList getWPawns()      const { return pawns[1]; }
  pieceList getBPawns()      const { return pawns[0]; }
  pieceList getWRooks()      const { return rooks[1]; }
  pieceList getBRooks()      const { return rooks[0]; }
  uint8_t getWhiteKing()     const { return whiteKing; }
  uint8_t getBlackKing()     const { return blackKing; }

  inline bool whiteInCheck() const { return blackAtks.isSet(whiteKing); }
  inline bool blackInCheck() const { return whiteAtks.isSet(blackKing); }
  bool inCheck()             const { return checkCount != 0; }

  bool isSynced() {
    int bpawnCount = 0, wpawnCount = 0;
    for (int i = 0; i < 64; ++i) {
      if (board[i] == PIECES.BPAWN) ++bpawnCount;
      else if (board[i] == PIECES.WPAWN) ++wpawnCount;
      if (board[i] == PIECES.EMPTY && allPIECES.isSet(i)) {
        std::cout << "\nunsync allPIECES1[" << i << "]\n"; return false;
      } else if (board[i] != PIECES.EMPTY && !allPIECES.isSet(i)) {
        std::cout << "\nunsync allPIECES2[" << i << "]\n"; return false;
      }
    }
    for (int i = 0; i < rooks[0].amt; ++i) {
      if (board[rooks[0].PIECES[i]] != PIECES.BROOK) {
        std::cout << "\nunsync list, brook[" << intToString(rooks[0].PIECES[i]) << "]\n"; return false;
      }
    }
    for (int i = 0; i < rooks[1].amt; ++i) {
      if (board[rooks[1].PIECES[i]] != PIECES.WROOK) {
        std::cout << "\nunsync list, wrook[" << intToString(rooks[1].PIECES[i]) << "]\n"; return false;
      }
    }
    if (wpawnCount != pawns[1].amt) { std::cout << "\nunsync count wpawns\n"; return false; }
    if (bpawnCount != pawns[0].amt) { std::cout << "\nunsync count bpawns\n"; return false; }
    for (int i = 0; i < pawns[0].amt; ++i) {
      if (board[pawns[0].PIECES[i]] != PIECES.BPAWN) {
        std::cout << "\nunsync list, bpawn[" << intToString(pawns[0].PIECES[i]) << "]\n"; return false;
      }
    }
    for (int i = 0; i < pawns[1].amt; ++i) {
      if (board[pawns[1].PIECES[i]] != PIECES.WPAWN) {
        std::cout << "\nunsync list, wpawn[" << intToString(pawns[1].PIECES[i]) << "]\n"; return false;
      }
    }
    return true;
  }

  void orderMoves(moveList &moves) {
    for (int i = 0; i < moves.amt; ++i) {
      movePair &cMPair = moves.moves[i];
      const Move &cMove = cMPair.move;
      const uint8_t moveFrom  = cMove.moveFrom();
      const uint8_t moveTo    = cMove.moveTo();
      const unsigned int captured = board[moveTo];
      const int myValue = chessCache.value(board[moveFrom]);

      cMPair.score = chessCache.statOrdPST[moveTo];

      if (turn == PIECES.WHITE) {
        if (blackAtks.isSet(moveTo)) {
          cMPair.score = -myValue;
          if (captured != PIECES.EMPTY) cMPair.score += chessCache.value(captured) + 100;
        } else {
          cMPair.score += chessCache.value(captured);
        }
        if (captured == PIECES.EMPTY) cMPair.score += moveHHistory[1][moveFrom][moveTo];
      } else {
        if (whiteAtks.isSet(moveTo)) {
          cMPair.score = -myValue;
          if (captured != PIECES.EMPTY) cMPair.score += chessCache.value(captured) + 100;
        } else {
          cMPair.score += chessCache.value(captured);
        }
        if (captured == PIECES.EMPTY) cMPair.score -= moveHHistory[0][moveFrom][moveTo];
      }

      if (cMove.isPromotion()) cMPair.score += 1000;
    }
    std::sort(moves.moves, moves.moves + moves.amt,
              [](const movePair &a, const movePair &b) { return a.score > b.score; });
  }

  std::string notateMove(Move &m) const {
    if (m.isNull()) return "null";
    int piece    = board[m.moveFrom()];
    int captured = board[m.moveTo()];
    if (piece == PIECES.EMPTY) piece = captured;
    std::string s = PIECES.toUnicode(PIECES.type(piece)) + " ";
    if (captured != PIECES.EMPTY && PIECES.color(captured) != PIECES.color(piece)) {
      s += chessCache.tileToNotation(m.moveFrom()) + 'x';
    } else if (PIECES.type(piece) != PIECES.PAWN && PIECES.type(piece) != PIECES.KING) {
      s += chessCache.tileToNotation(m.moveFrom());
    }
    s += chessCache.tileToNotation(m.moveTo());
    return s;
  }

  void setupFen(std::string fullFen) {
    resetValues();
    std::string unflippedFen, fenTurn, castlingFen, enPassantTargetSQR,
                halfMoveClockFen, fullMoveClockFen;
    int spaceCount = 0;
    for (char c : fullFen) {
      if (c == ' ') { ++spaceCount; continue; }
      switch (spaceCount) {
        case 0: unflippedFen      += c; break;
        case 1: fenTurn           += tolower(c); break;
        case 2: castlingFen       += c; break;
        case 3: enPassantTargetSQR += c; break;
        case 4: halfMoveClockFen  += c; break;
        case 5: fullMoveClockFen  += c; break;
      }
    }

    if (fenTurn == "w") {
      turn = PIECES.WHITE; oppTurnIndex = 0; turnIndex = 1;
    } else if (fenTurn == "b") {
      turn = PIECES.BLACK; oppTurnIndex = 1; turnIndex = 0;
    }

    if (enPassantTargetSQR != "-" && !enPassantTargetSQR.empty()) {
      uint8_t sq = chessCache.notationToTile(enPassantTargetSQR);
      currentGameState |= ((chessCache.preComputedCols[sq] + 1) << 4);
    }

    if (halfMoveClockFen != "-" && !halfMoveClockFen.empty())
      fiftyMoveCounter = std::stoi(halfMoveClockFen);

    for (char c : castlingFen) {
      if      (c == 'K') currentGameState |= (1 << 0);
      else if (c == 'Q') currentGameState |= (1 << 1);
      else if (c == 'k') currentGameState |= (1 << 2);
      else if (c == 'q') currentGameState |= (1 << 3);
    }
    gameStateHistory.push(currentGameState);
    repHistory.push(zobristKey);

    std::string fen = invertFen(unflippedFen);
    char index = 63;
    for (char letter : fen) {
      if (isdigit(letter)) {
        for (char j = 0; j < letter - '0'; ++j) { board[index] = PIECES.EMPTY; --index; }
      } else {
        switch (letter) {
          case 'p': pawns[0].addAtTile(index);   board[index] = PIECES.BPAWN;   allPIECES.setSquare(index--); break;
          case 'n': knights[0].addAtTile(index);  board[index] = PIECES.BKNIGHT; allPIECES.setSquare(index--); break;
          case 'b': bishops[0].addAtTile(index);  board[index] = PIECES.BBISHOP; allPIECES.setSquare(index--); break;
          case 'r': rooks[0].addAtTile(index);    board[index] = PIECES.BROOK;   allPIECES.setSquare(index--); break;
          case 'q': queens[0].addAtTile(index);   board[index] = PIECES.BQUEEN;  allPIECES.setSquare(index--); break;
          case 'k': blackKing = index;             board[index] = PIECES.BKING;   allPIECES.setSquare(index--); break;
          case 'P': pawns[1].addAtTile(index);    board[index] = PIECES.WPAWN;   allPIECES.setSquare(index--); break;
          case 'N': knights[1].addAtTile(index);   board[index] = PIECES.WKNIGHT; allPIECES.setSquare(index--); break;
          case 'B': bishops[1].addAtTile(index);   board[index] = PIECES.WBISHOP; allPIECES.setSquare(index--); break;
          case 'R': rooks[1].addAtTile(index);     board[index] = PIECES.WROOK;   allPIECES.setSquare(index--); break;
          case 'Q': queens[1].addAtTile(index);    board[index] = PIECES.WQUEEN;  allPIECES.setSquare(index--); break;
          case 'K': whiteKing = index;              board[index] = PIECES.WKING;   allPIECES.setSquare(index--); break;
        }
      }
    }
    generatePseudoLegals();
  }

  void display(bool whiteSide, BitBoard &highlights,
               std::string line1 = " ", std::string line2 = " ",
               std::string line3 = " ", std::string line4 = " ") {
    for (int i = 0; i < 64; ++i) {
      int i2  = i;
      int row = chessCache.preComputedRows[i];
      int col = chessCache.preComputedCols[i];
      if (whiteSide) i2 = ((7 - row) * 8) + col;
      int row2 = chessCache.preComputedRows[i2];

      if (col == 0) {
        setTxtColor(chessColors.greyLetCol);
        std::cout << (row2 + 1) << '|';
      }

      if (highlights.isSet(i2)) {
        setTxtColor(chessColors.hlightCol);
      } else {
        int pieceColor = PIECES.color(board[i2]) == 0 ? 0 : 1;
        if (chessCache.colorOfSquare(i2)) {
          setTxtColor(pieceColor == 0 ? chessColors.wWhiteCol : chessColors.wBlackCol);
        } else {
          setTxtColor(pieceColor == 0 ? chessColors.bWhiteCol : chessColors.bBlackCol);
        }
        if (!prevMove.isNull() && (prevMove.moveFrom() == i2 || prevMove.moveTo() == i2)) {
          setTxtColor(PIECES.color(board[prevMove.moveTo()]) == PIECES.WHITE
                          ? chessColors.prevMWhiteCol : chessColors.prevMBlackCol);
        }
        if (checkCount != 0) {
          if (i2 == whiteKing && checkRay.isSet(i2)) setTxtColor(chessColors.checkBlackCol);
          if (i2 == blackKing && checkRay.isSet(i2)) setTxtColor(chessColors.checkWhiteCol);
        }
        if (blockRay.isSet(i2)) {
          setTxtColor(PIECES.color(board[i2]) == PIECES.WHITE
                          ? chessColors.checkBlackCol : chessColors.checkWhiteCol);
        }
      }

      std::cout << PIECES.toUnicode(PIECES.type(board[i2])) << " ";
      if ((i + 1) % 8 == 0) {
        setTxtColor(chessColors.greyLetCol);
        switch (chessCache.preComputedRows[i]) {
          case 0:
            std::cout << "  ply: " << plyCount << ", "
                      << (turn ? " turn: b,  " : " turn: w,  ")
                      << (whiteKingSideCastle()  ? "K" : "-")
                      << (whiteQueenSideCastle() ? "Q" : "-")
                      << (blackKingSideCastle()  ? "k" : "-")
                      << (blackQueenSideCastle() ? "q" : "-")
                      << ",  FiftyMoveCounter: " << fiftyMoveCounter;
            break;
          case 1: std::cout << line1; break;
          case 2: std::cout << line2; break;
          case 3: std::cout << line3; break;
          case 4: std::cout << line4; break;
          case 6: std::cout << "  zKey: " << zobristKey; break;
          case 7: std::cout << "  hEval: " << heuristicEval(); break;
        }
        setTxtColor(15);
        std::cout << '\n';
      }
    }
    setTxtColor(chessColors.greyLetCol);
    std::cout << "  a b c d e f g h\n";
    setTxtColor(15);
  }

  void makeTurn() {
    oppTurn = turn;
    if (turn) { oppTurnIndex = 0; turnIndex = 1; turn = PIECES.WHITE; }
    else      { oppTurnIndex = 1; turnIndex = 0; turn = PIECES.BLACK; }
  }

  bool isCapture(const Move &M) const { return allPIECES.isSet(M.moveTo()); }

  void addLegal(moveList &ML, const Move &M) const {
    if (!genQuiets && !isCapture(M)) return;
    const unsigned int mto = M.moveTo();
    if (!blockRay.isEmpty() && !blockRay.isSet(mto)) return;
    if (pinExistInPosition) {
      const unsigned int mFrom = M.moveFrom();
      for (int d = 0; d < 8; ++d) {
        if (pinMasks[oppTurnIndex][d].isSet(mFrom) && !pinMasks[oppTurnIndex][d].isSet(mto))
          return;
      }
    }
    if ((board[mto] == PIECES.EMPTY || turn != PIECES.color(board[mto])) &&
        mto != whiteKing && mto != blackKing) {
      ML.addConstMove(M);
    }
  }

  void addPLegal(moveList &ML, const Move &m) const {
    if (!genQuiets && !isCapture(m)) return;
    const unsigned int mto = m.moveTo();
    if ((board[mto] == PIECES.EMPTY || turn != PIECES.color(board[mto])) &&
        mto != whiteKing && mto != blackKing) {
      ML.addConstMove(m);
    }
  }

  void generatePawnMoves(moveList &m) const {
    if (turn == PIECES.WHITE) {
      for (int i = 0; i < pawns[1].amt; ++i) {
        int pi = pawns[1].PIECES[i];
        if (genQuiets) {
          const Move push = chessCache.wPawnMoves[pi][0];
          if (!push.isNull() && !allPIECES.isSet(push.moveTo())) {
            if (chessCache.preComputedRows[pi] == 6) {
              addLegal(m, chessCache.wPawnMoves[pi][4]);
              addLegal(m, chessCache.wPawnMoves[pi][5]);
              addLegal(m, chessCache.wPawnMoves[pi][6]);
              addLegal(m, chessCache.wPawnMoves[pi][7]);
            } else {
              addLegal(m, push);
            }
            if (!allPIECES.isSet(chessCache.wPawnMoves[pi][1].moveTo()))
              addLegal(m, chessCache.wPawnMoves[pi][1]);
          }
        }

        uint8_t enPFile = getEnPassantFile();
        for (int slot : {2, 3}) {
          Move cap = chessCache.wPawnMoves[pi][slot];
          if (cap.isNull()) continue;
          if (isCapture(cap)) {
            if (chessCache.preComputedRows[pi] == 6) {
              cap.setFlag(MoveCache::PromoteToQueenFlag);  addLegal(m, cap);
              cap.setFlag(MoveCache::PromoteToRookFlag);   addLegal(m, cap);
              cap.setFlag(MoveCache::PromoteToBishopFlag); addLegal(m, cap);
              cap.setFlag(MoveCache::PromoteToKnightFlag); addLegal(m, cap);
            } else {
              addLegal(m, cap);
            }
          } else if (enPFile != 0 &&
                     chessCache.preComputedCols[cap.moveTo()] == enPFile - 1 &&
                     chessCache.preComputedRows[cap.moveFrom()] == 4) {
            cap.setFlag(MoveCache::EnPassantCaptureFlag);
            m.addConstMove(cap);
          }
        }
      }
    } else {
      for (int i = 0; i < pawns[0].amt; ++i) {
        int pi = pawns[0].PIECES[i];
        const Move push = chessCache.bPawnMoves[pi][0];
        if (!push.isNull() && !allPIECES.isSet(push.moveTo())) {
          if (chessCache.preComputedRows[pi] == 1) {
            addLegal(m, chessCache.bPawnMoves[pi][4]);
            addLegal(m, chessCache.bPawnMoves[pi][5]);
            addLegal(m, chessCache.bPawnMoves[pi][6]);
            addLegal(m, chessCache.bPawnMoves[pi][7]);
          } else {
            addLegal(m, push);
          }
          if (!allPIECES.isSet(chessCache.bPawnMoves[pi][1].moveTo()))
            addLegal(m, chessCache.bPawnMoves[pi][1]);
        }

        uint8_t enPFile = getEnPassantFile();
        for (int slot : {2, 3}) {
          Move cap = chessCache.bPawnMoves[pi][slot];
          if (cap.isNull()) continue;
          if (isCapture(cap)) {
            if (chessCache.preComputedRows[pi] == 1) {
              cap.setFlag(MoveCache::PromoteToQueenFlag);  addLegal(m, cap);
              cap.setFlag(MoveCache::PromoteToRookFlag);   addLegal(m, cap);
              cap.setFlag(MoveCache::PromoteToBishopFlag); addLegal(m, cap);
              cap.setFlag(MoveCache::PromoteToKnightFlag); addLegal(m, cap);
            } else {
              addLegal(m, cap);
            }
          } else if (enPFile != 0 &&
                     chessCache.preComputedCols[cap.moveTo()] == enPFile - 1 &&
                     chessCache.preComputedRows[cap.moveFrom()] == 3) {
            cap.setFlag(MoveCache::EnPassantCaptureFlag);
            m.addConstMove(cap);
          }
        }
      }
    }
  }

  void generateKnightMoves(moveList &m) const {
    int side = (turn == PIECES.WHITE) ? 1 : 0;
    for (int i = 0; i < knights[side].amt; ++i)
      for (int j = 0; j < 8; ++j)
        addLegal(m, chessCache.knightMoves[knights[side].PIECES[i]][j]);
  }

  void generateBishopMoves(moveList &m) const {
    int side = (turn == PIECES.WHITE) ? 1 : 0;
    for (int *lists[] = {nullptr}; side >= 0; --side) {  // unused trick — just unroll:
    (void)lists;
    break; }
    // Bishops
    for (int s : {(turn == PIECES.WHITE) ? 1 : 0}) {
      for (int pi = 0; pi < bishops[s].amt; ++pi) {
        for (int d = 0; d < 4; ++d) {
          for (int r = 0; r < 7; ++r) {
            const Move &mv = chessCache.bishopMoves[d][bishops[s].PIECES[pi]][r];
            addLegal(m, mv);
            if (allPIECES.isSet(mv.moveTo())) break;
          }
        }
      }
      for (int qi = 0; qi < queens[s].amt; ++qi) {
        for (int d = 0; d < 4; ++d) {
          for (int r = 0; r < 7; ++r) {
            const Move &mv = chessCache.bishopMoves[d][queens[s].PIECES[qi]][r];
            addLegal(m, mv);
            if (allPIECES.isSet(mv.moveTo())) break;
          }
        }
      }
    }
  }

  void generateRookMoves(moveList &m) const {
    for (int s : {(turn == PIECES.WHITE) ? 1 : 0}) {
      for (int ri = 0; ri < rooks[s].amt; ++ri) {
        for (int d = 0; d < 4; ++d) {
          for (int r = 0; r < 7; ++r) {
            const Move &mv = chessCache.rookMoves[d][rooks[s].PIECES[ri]][r];
            addLegal(m, mv);
            if (allPIECES.isSet(mv.moveTo())) break;
          }
        }
      }
      for (int qi = 0; qi < queens[s].amt; ++qi) {
        for (int d = 0; d < 4; ++d) {
          for (int r = 0; r < 7; ++r) {
            const Move &mv = chessCache.rookMoves[d][queens[s].PIECES[qi]][r];
            addLegal(m, mv);
            if (allPIECES.isSet(mv.moveTo())) break;
          }
        }
      }
    }
  }

  void generateKingMoves(moveList &m) const {
    if (turn == PIECES.WHITE) {
      for (int j = 0; j < 8; ++j) {
        const Move &mv = chessCache.kingMoves[whiteKing][j];
        if (!blackAtks.isSet(mv.moveTo()) && !checkRay.isSet(mv.moveTo()))
          addPLegal(m, mv);
      }
      if (!whiteInCheck()) {
        if (whiteKingSideCastle() &&
            board[chessCache.whiteKingCastleTo]     == PIECES.EMPTY &&
            board[chessCache.whiteKingRookCastleTo] == PIECES.EMPTY &&
            !blackAtks.isSet(chessCache.whiteKingCastleTo) &&
            !blackAtks.isSet(chessCache.whiteKingRookCastleTo))
          m.addConstMove(chessCache.whiteKingSideCastle);

        if (whiteQueenSideCastle() &&
            board[chessCache.whiteQueenCastleTo]     == PIECES.EMPTY &&
            board[chessCache.whiteQueenRookCastleTo] == PIECES.EMPTY &&
            board[chessCache.whiteQueenCastleVacant] == PIECES.EMPTY &&
            !blackAtks.isSet(chessCache.whiteQueenCastleTo) &&
            !blackAtks.isSet(chessCache.whiteQueenRookCastleTo))
          m.addConstMove(chessCache.whiteQueenSideCastle);
      }
    } else {
      for (int j = 0; j < 8; ++j) {
        const Move &mv = chessCache.kingMoves[blackKing][j];
        if (!whiteAtks.isSet(mv.moveTo()) && !checkRay.isSet(mv.moveTo()))
          addPLegal(m, mv);
      }
      if (!blackInCheck()) {
        if (blackKingSideCastle() &&
            board[chessCache.blackKingCastleTo]     == PIECES.EMPTY &&
            board[chessCache.blackKingRookCastleTo] == PIECES.EMPTY &&
            !whiteAtks.isSet(chessCache.blackKingCastleTo) &&
            !whiteAtks.isSet(chessCache.blackKingRookCastleTo))
          m.addConstMove(chessCache.blackKingSideCastle);

        if (blackQueenSideCastle() &&
            board[chessCache.blackQueenCastleTo]     == PIECES.EMPTY &&
            board[chessCache.blackQueenRookCastleTo] == PIECES.EMPTY &&
            board[chessCache.blackQueenCastleVacant] == PIECES.EMPTY &&
            !whiteAtks.isSet(chessCache.blackQueenCastleTo) &&
            !whiteAtks.isSet(chessCache.blackQueenRookCastleTo))
          m.addConstMove(chessCache.blackQueenSideCastle);
      }
    }
  }

  // ---- Pseudo-legal attack generation ----

  void genPseudoPawnMoves() {
    for (int i = 0; i < pawns[1].amt; ++i) {
      int pi = pawns[1].PIECES[i];
      for (int slot : {2, 3}) {
        const Move &mv = chessCache.wPawnMoves[pi][slot];
        if (!mv.isNull()) {
          whiteAtks.setSquare(mv.moveTo());
          if (mv.moveTo() == blackKing) { blockRay.setSquare(pi); checkRay.setSquare(blackKing); ++checkCount; }
        }
      }
    }
    for (int i = 0; i < pawns[0].amt; ++i) {
      int pi = pawns[0].PIECES[i];
      for (int slot : {2, 3}) {
        const Move &mv = chessCache.bPawnMoves[pi][slot];
        if (!mv.isNull()) {
          blackAtks.setSquare(mv.moveTo());
          if (mv.moveTo() == whiteKing) { blockRay.setSquare(pi); checkRay.setSquare(whiteKing); ++checkCount; }
        }
      }
    }
  }

  void genPseudoKnightMoves() {
    for (int i = 0; i < knights[1].amt; ++i)
      for (int j = 0; j < 8; ++j) {
        const Move &mv = chessCache.knightMoves[knights[1].PIECES[i]][j];
        if (!mv.isNull()) {
          whiteAtks.setSquare(mv.moveTo());
          if (mv.moveTo() == blackKing) { blockRay.setSquare(knights[1].PIECES[i]); checkRay.setSquare(blackKing); ++checkCount; }
        }
      }
    for (int i = 0; i < knights[0].amt; ++i)
      for (int j = 0; j < 8; ++j) {
        const Move &mv = chessCache.knightMoves[knights[0].PIECES[i]][j];
        if (!mv.isNull()) {
          blackAtks.setSquare(mv.moveTo());
          if (mv.moveTo() == whiteKing) { blockRay.setSquare(knights[0].PIECES[i]); checkRay.setSquare(whiteKing); ++checkCount; }
        }
      }
  }

  // Shared logic for sliding piece pseudo-legal generation
  void genPseudoSliding(bool isBishop, int side,
                        BitBoard &myAtks, BitBoard &oppAtks,
                        uint8_t myKing, uint8_t oppKing,
                        int pinDirOffset) {
    auto &list = isBishop ? bishops[side] : rooks[side];
    auto &qlist = queens[side];
    auto &movTable = isBishop ? chessCache.bishopMoves : chessCache.rookMoves;

    auto process = [&](int pieceIdx) {
      for (int d = 0; d < 4; ++d) {
        for (int r = 0; r < 7; ++r) {
          const Move &mv = movTable[d][pieceIdx][r];
          if (mv.isNull()) break;
          myAtks.setSquare(mv.moveTo());
          if (mv.moveTo() == oppKing) {
            checkRay = chessCache.rays1Extra[mv.moveFrom()][oppKing];
            blockRay = chessCache.rays[mv.moveFrom()][oppKing];
            ++checkCount;
          }
          if (allPIECES.isSet(mv.moveTo())) {
            for (int r2 = r + 1; r2 < 7; ++r2) {
              const Move &cont = movTable[d][pieceIdx][r2];
              if (cont.isNull()) break;
              if (allPIECES.isSet(cont.moveTo())) {
                if (cont.moveTo() == oppKing) {
                  pinMasks[side][d + pinDirOffset] = chessCache.rays[mv.moveFrom()][oppKing];
                  pinExistInPosition = true;
                }
                break;
              }
            }
            break;
          }
        }
      }
    };

    for (int i = 0; i < list.amt;  ++i) process(list.PIECES[i]);
    for (int i = 0; i < qlist.amt; ++i) process(qlist.PIECES[i]);
  }

  void genPseudoBishopMoves() {
    genPseudoSliding(true, 1, whiteAtks, blackAtks, whiteKing, blackKing, 0);
    genPseudoSliding(true, 0, blackAtks, whiteAtks, blackKing, whiteKing, 0);
  }

  void genPseudoRookMoves() {
    genPseudoSliding(false, 1, whiteAtks, blackAtks, whiteKing, blackKing, 4);
    genPseudoSliding(false, 0, blackAtks, whiteAtks, blackKing, whiteKing, 4);
  }

  void genPseudoKingMoves() {
    for (int j = 0; j < 8; ++j) {
      const Move &wm = chessCache.kingMoves[whiteKing][j];
      if (!wm.isNull()) whiteAtks.setSquare(wm.moveTo());
      const Move &bm = chessCache.kingMoves[blackKing][j];
      if (!bm.isNull()) blackAtks.setSquare(bm.moveTo());
    }
  }

  uint64_t generateZKey() {
    uint64_t key = 0;
    for (int i = 0; i < 64; ++i) {
      uint8_t pt = PIECES.type(board[i]);
      if (pt != PIECES.EMPTY) {
        int pc = (PIECES.color(board[i]) == PIECES.WHITE) ? 0 : 1;
        key ^= chessCache.zobristLookup[i][pc][pt - 1];
      }
    }
    if (turn == PIECES.BLACK) key ^= chessCache.blackTurnZobrist;
    key ^= chessCache.zobristEnPassant[getEnPassantFile()];
    key ^= chessCache.zobristCastling[currentGameState & castlingMASK];
    return key;
  }

  void generatePseudoLegals() {
    if (pinExistInPosition) {
      for (int d = 0; d < 8; ++d) { pinMasks[0][d].clearBoard(); pinMasks[1][d].clearBoard(); }
      pinExistInPosition = false;
    }
    checkCount = 0;
    checkRay.clearBoard();
    blockRay.clearBoard();
    whiteAtks.clearBoard();
    blackAtks.clearBoard();

    genPseudoPawnMoves();
    genPseudoKnightMoves();
    genPseudoBishopMoves();
    genPseudoRookMoves();
    genPseudoKingMoves();
    zobristKey = generateZKey();
  }

  void generateMoves(moveList &moves, bool includeQuiets) {
    if (fiftyMoveCounter > 50 || repHistory.isThreeFold()) return;
    genQuiets = includeQuiets;
    if (checkCount < 2) {
      generatePawnMoves(moves);
      generateKnightMoves(moves);
      generateBishopMoves(moves);
      generateRookMoves(moves);
    }
    generateKingMoves(moves);
  }

  void makeMove(const Move &m) {
    uint8_t newCastleState = currentGameState & 15;
    currentGameState = 0;

    uint8_t from = m.moveFrom();
    uint8_t to   = m.moveTo();
    prevMove = m;

    // Move piece in list
    if      (board[from] == PIECES.BPAWN)   pawns[0].MovePiece(from, to);
    else if (board[from] == PIECES.WPAWN)   pawns[1].MovePiece(from, to);
    else if (board[from] == PIECES.BKNIGHT) knights[0].MovePiece(from, to);
    else if (board[from] == PIECES.WKNIGHT) knights[1].MovePiece(from, to);
    else if (board[from] == PIECES.WBISHOP) bishops[1].MovePiece(from, to);
    else if (board[from] == PIECES.BBISHOP) bishops[0].MovePiece(from, to);
    else if (board[from] == PIECES.WROOK) {
      rooks[1].MovePiece(from, to);
      if      (from == chessCache.whiteKingRook)  newCastleState &= whiteCastleKingsideMask;
      else if (from == chessCache.whiteQueenRook) newCastleState &= whiteCastleQueensideMask;
    } else if (board[from] == PIECES.BROOK) {
      rooks[0].MovePiece(from, to);
      if      (from == chessCache.blackKingRook)  newCastleState &= blackCastleKingsideMask;
      else if (from == chessCache.blackQueenRook) newCastleState &= blackCastleQueensideMask;
    } else if (board[from] == PIECES.WQUEEN) queens[1].MovePiece(from, to);
    else if (board[from] == PIECES.BQUEEN)  queens[0].MovePiece(from, to);
    else if (from == whiteKing) { whiteKing = to; newCastleState &= whiteCastleMask; }
    else if (from == blackKing) { blackKing = to; newCastleState &= blackCastleMask; }

    if (m.isPawnTwoUp())
      currentGameState |= ((chessCache.preComputedCols[from] + 1) << 4);

    if (m.isCastling()) {
      if (to == chessCache.whiteKingCastleTo) {
        board[chessCache.whiteKingRookCastleTo] = PIECES.WROOK;
        deleteTile(chessCache.whiteKingRook);
        rooks[turnIndex].MovePiece(chessCache.whiteKingRook, chessCache.whiteKingRookCastleTo);
        allPIECES.setSquare(chessCache.whiteKingRookCastleTo);
      } else if (to == chessCache.whiteQueenCastleTo) {
        board[chessCache.whiteQueenRookCastleTo] = PIECES.WROOK;
        deleteTile(chessCache.whiteQueenRook);
        rooks[turnIndex].MovePiece(chessCache.whiteQueenRook, chessCache.whiteQueenRookCastleTo);
        allPIECES.setSquare(chessCache.whiteQueenRookCastleTo);
      } else if (to == chessCache.blackKingCastleTo) {
        board[chessCache.blackKingRookCastleTo] = PIECES.BROOK;
        deleteTile(chessCache.blackKingRook);
        rooks[turnIndex].MovePiece(chessCache.blackKingRook, chessCache.blackKingRookCastleTo);
        allPIECES.setSquare(chessCache.blackKingRookCastleTo);
      } else if (to == chessCache.blackQueenCastleTo) {
        board[chessCache.blackQueenRookCastleTo] = PIECES.BROOK;
        deleteTile(chessCache.blackQueenRook);
        rooks[turnIndex].MovePiece(chessCache.blackQueenRook, chessCache.blackQueenRookCastleTo);
        allPIECES.setSquare(chessCache.blackQueenRookCastleTo);
      }
    }

    if (m.isPromotion()) {
      pawns[turnIndex].removeAtTile(to);
      if      (m.promoteQueen())  { board[from] = PIECES.QUEEN  | turn; queens[turnIndex].addAtTile(to); }
      else if (m.promoteRook())   { board[from] = PIECES.ROOK   | turn; rooks[turnIndex].addAtTile(to); }
      else if (m.promoteBishop()) { board[from] = PIECES.BISHOP | turn; bishops[turnIndex].addAtTile(to); }
      else if (m.promoteKnight()) { board[from] = PIECES.KNIGHT | turn; knights[turnIndex].addAtTile(to); }
    }

    if (m.isEnPassant()) {
      int epSq = (turn == PIECES.WHITE) ? to - 8 : to + 8;
      pawns[oppTurnIndex].removeAtTile(epSq);
      deleteTile(epSq);
    }

    // Captures
    if      (board[to] == PIECES.BPAWN)   pawns[0].removeAtTile(to);
    else if (board[to] == PIECES.WPAWN)   pawns[1].removeAtTile(to);
    else if (board[to] == PIECES.BKNIGHT) knights[0].removeAtTile(to);
    else if (board[to] == PIECES.WKNIGHT) knights[1].removeAtTile(to);
    else if (board[to] == PIECES.WBISHOP) bishops[1].removeAtTile(to);
    else if (board[to] == PIECES.BBISHOP) bishops[0].removeAtTile(to);
    else if (board[to] == PIECES.WROOK) {
      rooks[1].removeAtTile(to);
      if      (to == chessCache.whiteKingRook)  newCastleState &= whiteCastleKingsideMask;
      else if (to == chessCache.whiteQueenRook) newCastleState &= whiteCastleQueensideMask;
    } else if (board[to] == PIECES.BROOK) {
      rooks[0].removeAtTile(to);
      if      (to == chessCache.blackKingRook)  newCastleState &= blackCastleKingsideMask;
      else if (to == chessCache.blackQueenRook) newCastleState &= blackCastleQueensideMask;
    } else if (board[to] == PIECES.WQUEEN) queens[1].removeAtTile(to);
    else if (board[to] == PIECES.BQUEEN)  queens[0].removeAtTile(to);

    allPIECES.setSquare(to);
    allPIECES.unSetSquare(from);
    ++plyCount;
    ++fiftyMoveCounter;

    if (PIECES.type(board[from]) == PIECES.PAWN || board[to] != PIECES.EMPTY || m.isEnPassant())
      fiftyMoveCounter = 0;

    currentGameState |= newCastleState;
    currentGameState |= (board[to] << 8);
    currentGameState |= (fiftyMoveCounter << 16);

    gameStateHistory.push(currentGameState);
    repHistory.push(zobristKey);

    board[to]   = board[from];
    board[from] = PIECES.EMPTY;
    makeTurn();
    generatePseudoLegals();
  }

  void unMakeMove(const Move &m) {
    --plyCount;
    uint8_t from = m.moveFrom();
    uint8_t to   = m.moveTo();
    uint8_t captured = (currentGameState >> 8) & 63;
    prevMove.clearMove();

    if      (board[to] == PIECES.BPAWN)   pawns[0].MovePiece(to, from);
    else if (board[to] == PIECES.WPAWN)   pawns[1].MovePiece(to, from);
    else if (board[to] == PIECES.BKNIGHT) knights[0].MovePiece(to, from);
    else if (board[to] == PIECES.WKNIGHT) knights[1].MovePiece(to, from);
    else if (board[to] == PIECES.WBISHOP) bishops[1].MovePiece(to, from);
    else if (board[to] == PIECES.BBISHOP) bishops[0].MovePiece(to, from);
    else if (board[to] == PIECES.WROOK)   rooks[1].MovePiece(to, from);
    else if (board[to] == PIECES.BROOK)   rooks[0].MovePiece(to, from);
    else if (board[to] == PIECES.WQUEEN)  queens[1].MovePiece(to, from);
    else if (board[to] == PIECES.BQUEEN)  queens[0].MovePiece(to, from);
    else if (to == whiteKing) whiteKing = from;
    else if (to == blackKing) blackKing = from;

    if (m.isCastling()) {
      if (to == chessCache.whiteKingCastleTo) {
        board[chessCache.whiteKingRook] = PIECES.WROOK;
        rooks[oppTurnIndex].MovePiece(chessCache.whiteKingRookCastleTo, chessCache.whiteKingRook);
        deleteTile(chessCache.whiteKingRookCastleTo);
        allPIECES.setSquare(chessCache.whiteKingRook);
      } else if (to == chessCache.whiteQueenCastleTo) {
        board[chessCache.whiteQueenRook] = PIECES.WROOK;
        rooks[oppTurnIndex].MovePiece(chessCache.whiteQueenRookCastleTo, chessCache.whiteQueenRook);
        deleteTile(chessCache.whiteQueenRookCastleTo);
        allPIECES.setSquare(chessCache.whiteQueenRook);
      } else if (to == chessCache.blackKingCastleTo) {
        board[chessCache.blackKingRook] = PIECES.BROOK;
        rooks[oppTurnIndex].MovePiece(chessCache.blackKingRookCastleTo, chessCache.blackKingRook);
        deleteTile(chessCache.blackKingRookCastleTo);
        allPIECES.setSquare(chessCache.blackKingRook);
      } else if (to == chessCache.blackQueenCastleTo) {
        board[chessCache.blackQueenRook] = PIECES.BROOK;
        rooks[oppTurnIndex].MovePiece(chessCache.blackQueenRookCastleTo, chessCache.blackQueenRook);
        deleteTile(chessCache.blackQueenRookCastleTo);
        allPIECES.setSquare(chessCache.blackQueenRook);
      }
    }

    if (m.isPromotion()) {
      board[to] = PIECES.PAWN | oppTurn;
      if      (m.promoteQueen())  queens[oppTurnIndex].removeAtTile(to);
      else if (m.promoteRook())   rooks[oppTurnIndex].removeAtTile(to);
      else if (m.promoteBishop()) bishops[oppTurnIndex].removeAtTile(to);
      else if (m.promoteKnight()) knights[oppTurnIndex].removeAtTile(to);
      pawns[oppTurnIndex].addAtTile(from);
    }

    if (m.isEnPassant()) {
      if (turn == PIECES.WHITE) {
        int epSq = to + 8;
        pawns[turnIndex].addAtTile(epSq);
        allPIECES.setSquare(epSq);
        board[epSq] = PIECES.WPAWN;
      } else {
        int epSq = to - 8;
        pawns[turnIndex].addAtTile(epSq);
        allPIECES.setSquare(epSq);
        board[epSq] = PIECES.BPAWN;
      }
    }

    allPIECES.setSquare(from);
    if (captured == PIECES.EMPTY) {
      allPIECES.unSetSquare(to);
    } else if (captured == PIECES.BPAWN)   pawns[0].addAtTile(to);
    else if (captured == PIECES.WPAWN)     pawns[1].addAtTile(to);
    else if (captured == PIECES.BKNIGHT)   knights[0].addAtTile(to);
    else if (captured == PIECES.WKNIGHT)   knights[1].addAtTile(to);
    else if (captured == PIECES.BBISHOP)   bishops[0].addAtTile(to);
    else if (captured == PIECES.WBISHOP)   bishops[1].addAtTile(to);
    else if (captured == PIECES.BROOK)     rooks[0].addAtTile(to);
    else if (captured == PIECES.WROOK)     rooks[1].addAtTile(to);
    else if (captured == PIECES.BQUEEN)    queens[0].addAtTile(to);
    else if (captured == PIECES.WQUEEN)    queens[1].addAtTile(to);

    board[from] = board[to];
    board[to]   = captured;

    gameStateHistory.pop();
    repHistory.pop();
    currentGameState = gameStateHistory.peek();
    fiftyMoveCounter = currentGameState >> 16;

    makeTurn();
    generatePseudoLegals();
  }

  int heuristicEval() {
    int eval = 0;

    eval += (pawns[1].amt   - pawns[0].amt)   * PreComputedCache::pawnValue;
    eval += (knights[1].amt - knights[0].amt) * PreComputedCache::knightValue;
    eval += (bishops[1].amt - bishops[0].amt) * PreComputedCache::bishopValue;
    eval += (rooks[1].amt   - rooks[0].amt)   * PreComputedCache::rookValue;
    eval += (queens[1].amt  - queens[0].amt)  * PreComputedCache::queenValue;

    int threatsW = whiteAtks.populationCountBAND(allPIECES.get());
    int threatsB = blackAtks.populationCountBAND(allPIECES.get());
    eval += threatsW - threatsB;
    if (plyCount > 8) eval += (threatsW - threatsB) * 10;

    eval += (int)whiteAtks.populationCount() - (int)blackAtks.populationCount();

    if (bishops[1].amt >= 2) eval += 60;
    if (bishops[0].amt >= 2) eval -= 60;

    int materialBig = (rooks[1].amt   + rooks[0].amt)   * 2 +
                       bishops[1].amt  + bishops[0].amt  +
                       knights[1].amt  + knights[0].amt  +
                      (queens[1].amt   + queens[0].amt)  * 5;

    if (pawns[1].amt + pawns[0].amt == 0 && materialBig < 2) return 0;

    if (materialBig <= 15) {
      for (int i = 0; i < pawns[0].amt; ++i)
        eval -= chessCache.pawnEndPST[0][pawns[0].PIECES[i]] -
                chessCache.distances[blackKing][pawns[0].PIECES[i]];
      for (int i = 0; i < pawns[1].amt; ++i)
        eval += chessCache.pawnEndPST[1][pawns[1].PIECES[i]] -
                chessCache.distances[whiteKing][pawns[1].PIECES[i]];
      eval += chessCache.centerPST[whiteKing] - chessCache.centerPST[blackKing];
    } else {
      for (int i = 0; i < pawns[0].amt; ++i) eval -= chessCache.pawnPST[0][pawns[0].PIECES[i]];
      for (int i = 0; i < pawns[1].amt; ++i) eval += chessCache.pawnPST[1][pawns[1].PIECES[i]];
      eval += chessCache.kingPST[1][whiteKing] - chessCache.kingPST[0][blackKing];
    }

    for (int i = 0; i < knights[0].amt; ++i) eval -= chessCache.horsePST[0][knights[0].PIECES[i]];
    for (int i = 0; i < knights[1].amt; ++i) eval += chessCache.horsePST[1][knights[1].PIECES[i]];
    for (int i = 0; i < bishops[0].amt; ++i) eval -= chessCache.bishopPST[0][bishops[0].PIECES[i]];
    for (int i = 0; i < bishops[1].amt; ++i) eval += chessCache.bishopPST[1][bishops[1].PIECES[i]];
    for (int i = 0; i < rooks[0].amt; ++i)   eval -= chessCache.rookPST[0][rooks[0].PIECES[i]];
    for (int i = 0; i < rooks[1].amt; ++i)   eval += chessCache.rookPST[1][rooks[1].PIECES[i]];

    return eval;
  }

  Move search_BestMove;
  int  search_Nodes         = 0;
  int  search_Depth         = 0;
  int  search_ExtendedDepth = 0;

  searchRes oSearch(int lockedDepth) {
    int beta  = PreComputedCache::evalPositiveInf;
    int alpha = PreComputedCache::evalNegativeInf;
    searchRes result;
    result.eval  = 0;
    search_Nodes = 0;

    int depthReached = 1;

    if (lockedDepth != 0) {
      depthReached = lockedDepth;
      search_BestMove.clearMove();
      result.eval = alphaBeta(lockedDepth, 0, alpha, beta, 0);
    } else {
      auto start = std::chrono::steady_clock::now();
      for (int depth = 1; depth < 100; ++depth) {
        depthReached = depth;
        search_BestMove.clearMove();
        result.eval = alphaBeta(depth, 0, alpha, beta, 0);

        setTxtColor(chessColors.greyLetCol);
        std::cout << "depth: " << depth << ", " << notateMove(search_BestMove)
                  << ", eval: " << result.eval << '\n';

        if (abs(result.eval) >= chessCache.evalWhiteWins) {
          result.mateIn = depth;
          break;
        }
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
                           std::chrono::steady_clock::now() - start).count();
        if (elapsed > 4000) break;
      }
    }

    result.m             = search_BestMove;
    result.nodes         = search_Nodes;
    result.depth         = depthReached;
    result.depthExtended = search_ExtendedDepth;
    return result;
  }

  int alphaBeta(int depth, int plyFromRoot, int alpha, int beta, int numExtensions) {
    if (depth == 0) {
      search_ExtendedDepth = plyFromRoot;
      return heuristicEval();
    }

    moveList genMoves;
    generateMoves(genMoves, true);

    if (genMoves.amt == 0) {
      if (repHistory.isThreeFold()) return 0;
      if (whiteInCheck()) return PreComputedCache::evalWhiteLoss - depth;
      if (blackInCheck()) return PreComputedCache::evalWhiteWins + depth;
      return 0;  // stalemate
    }

    orderMoves(genMoves);

    if (turnIndex) {  // White
      int maxEval = PreComputedCache::evalNegativeInf;
      for (uint8_t i = 0; i < genMoves.amt; ++i) {
        Move mv = genMoves.moves[i].move;
        makeMove(mv);
        int ext = (numExtensions < 8 && inCheck()) ? 1 : 0;
        if ((genMoves.moves[i].score < -10 || i > 20) && depth > 1) ext = -1;
        int score = alphaBeta(depth - 1 + ext, plyFromRoot + 1, alpha, beta, numExtensions + ext);
        unMakeMove(mv);
        ++search_Nodes;
        if (score > maxEval) {
          if (plyFromRoot == 0) search_BestMove = mv;
          maxEval = score;
        }
        if (score >= beta) {
          moveHHistory[1][mv.moveFrom()][mv.moveTo()] = depth * depth * 4;
          return beta;
        }
        if (score > alpha) alpha = score;
      }
      return maxEval;
    } else {  // Black
      int minEval = PreComputedCache::evalPositiveInf;
      for (uint8_t i = 0; i < genMoves.amt; ++i) {
        Move mv = genMoves.moves[i].move;
        makeMove(mv);
        int ext = (numExtensions < 8 && inCheck()) ? 1 : 0;
        if ((genMoves.moves[i].score < -10 || i > 20) && depth > 1) ext = -1;
        int score = alphaBeta(depth - 1 + ext, plyFromRoot + 1, alpha, beta, numExtensions + ext);
        unMakeMove(mv);
        ++search_Nodes;
        if (score < minEval) {
          if (plyFromRoot == 0) search_BestMove = mv;
          minEval = score;
        }
        if (score <= alpha) {
          moveHHistory[0][mv.moveFrom()][mv.moveTo()] = depth * depth * 4;
          return alpha;
        }
        if (score < beta) beta = score;
      }
      return minEval;
    }
  }

  Board()                  { setupFen(chessCache.startingFen); }
  Board(std::string fen)   { setupFen(fen); }
  ~Board()                 { deallocateMoveHHistory(moveHHistory); }
};

// ---- Perft ----

BitBoard cBoardHLight;

int PERFT(Board &chessBoard, int depth, int &depthCheck) {
  if (depth == 0) return 1;
  int total = 0;
  moveList genMoves;
  chessBoard.generateMoves(genMoves, true);
  chessBoard.orderMoves(genMoves);
  for (uint8_t i = 0; i < genMoves.amt; ++i) {
    if (depthCheck == depth)
      std::cout << '\n' << chessBoard.notateMove(genMoves.moves[i].move);
    chessBoard.makeMove(genMoves.moves[i].move);
    int branch = PERFT(chessBoard, depth - 1, depthCheck);
    if (depthCheck == depth)
      std::cout << ": " << branch << ", score: " << genMoves.moves[i].score;
    total += branch;
    chessBoard.unMakeMove(genMoves.moves[i].move);
  }
  return total;
}

void perftTest(Board &chessBoard) {
  int finalDepth = 0;
  std::cout << "\nstarting PERFT\ndepth: ";
  std::cin >> finalDepth;
  auto startAll = std::chrono::steady_clock::now();
  for (int d = 0; d <= finalDepth; ++d) {
    auto start   = std::chrono::steady_clock::now();
    int  numPos  = PERFT(chessBoard, d, finalDepth);
    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
                       std::chrono::steady_clock::now() - start).count();
    std::cout << '\n' << d << " ply,  " << numPos << " nodes,  " << elapsed << " ms";
  }
  auto total = std::chrono::duration_cast<std::chrono::milliseconds>(
                   std::chrono::steady_clock::now() - startAll).count();
  std::cout << "\ntotal time: " << intToString(static_cast<int>(total)) << "ms\n\n";
  system("PAUSE");
}

// ---- Helpers ----

std::string askFen() {
  std::cin.ignore();
  std::cout << "Enter FEN: ";
  std::string line;
  char ch;
  while ((ch = std::cin.get()) != '\n') line += ch;
  return line;
}

void EvalBar(int eval, Board &chessBoard) {
  for (int i = 0; i < 64; ++i) {
    int heval = std::min(63, std::max(1, 31 - eval / 100));
    setTxtColor(i > heval ? chessColors.bWhiteCol : chessColors.bBlackCol);
    if      (i == 31 || i == 32) std::cout << '|';
    else if (i == heval)         std::cout << '>';
    else if (i == heval + 1)     std::cout << '<';
    else                         std::cout << '=';
  }
  std::cout << "\n\n";
}

void startGame(Board &chessBoard) {
  BitBoard cBoardHLight;
  Move previousMoves[400];
  std::string input, aiTxt = " ";
  int aiEval = 0;
  bool blackSide = false;
  int chessBoardPly = 0;
  bool whiteAI = false, blackAI = false;
  int waiLockedDepth = 0, baiLockedDepth = 0;

  do {
    if (!chessBoard.isSynced()) system("PAUSE");
    system("CLS");
    cBoardHLight.clearBoard();
    EvalBar(aiEval, chessBoard);
    chessBoard.display(blackSide, cBoardHLight, aiTxt);

    if ((chessBoard.turn == PIECES.BLACK && blackAI) ||
        (chessBoard.turn == PIECES.WHITE && whiteAI)) {
      auto start = std::chrono::steady_clock::now();
      searchRes result = chessBoard.oSearch(
          chessBoard.turn == PIECES.WHITE ? waiLockedDepth : baiLockedDepth);
      aiEval = result.eval;
      std::string evalStr = result.mateIn ? "MATE #" + intToString(result.mateIn)
                                          : intToString(result.eval);
      if (!result.m.isNull()) {
        chessBoard.makeMove(result.m);
        auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                      std::chrono::steady_clock::now() - start).count();
        previousMoves[chessBoardPly++] = result.m;
        aiTxt = "  EVAL: " + evalStr +
                ",  nodes: " + intToString(result.nodes) +
                ",  time: "  + intToString(static_cast<int>(ms)) + "ms" +
                ",  depth: " + intToString(result.depth) + "/" +
                               intToString(result.depthExtended);
        continue;
      } else {
        aiTxt = "[NO MOVE] EVAL: " + evalStr +
                ",  nodes: " + intToString(result.nodes) +
                ",  depth: " + intToString(result.depth) + "/" +
                               intToString(result.depthExtended);
      }
    } else {
      aiEval = chessBoard.heuristicEval();
    }

    std::cout << "input: ";
    std::cin >> input;
    toLowercase(input);

    if (input == "help") {
      std::cout << "\n -> COMMANDS <-\n"
                << "\"exit\"     - exit current game\n"
                << "\"undo\"     - undo last move\n"
                << "\"flip\"     - flip board\n"
                << "\"wai\"      - toggle white AI\n"
                << "\"bai\"      - toggle black AI\n"
                << "\"f\"        - enter custom FEN\n"
                << "\"reset\"    - reset to starting position\n"
                << "\"baidepth\" - set black AI depth\n"
                << "\"waidepth\" - set white AI depth\n\n"
                << "\"cray\"     - highlight check ray\n"
                << "\"pieces\"   - highlight all pieces bitboard\n"
                << "\"watks\"    - highlight white attacks\n"
                << "\"batks\"    - highlight black attacks\n"
                << "\"pins\"     - highlight pins\n"
                << "\"kings\"    - highlight both kings\n"
                << "\"checkers\" - highlight white squares\n\n";
      system("PAUSE");
    } else if (input == "undo") {
      if (chessBoardPly > 0) chessBoard.unMakeMove(previousMoves[--chessBoardPly]);
      whiteAI = blackAI = false;
    } else if (input == "flip")     { blackSide = !blackSide; }
    else if (input == "wai")        { whiteAI = !whiteAI; }
    else if (input == "bai")        { blackAI = !blackAI; }
    else if (input == "f")          { chessBoard.setupFen(askFen()); }
    else if (input == "reset")      { chessBoard.setupFen(chessCache.startingFen); }
    else if (input == "baidepth")   { std::cout << "depth: "; std::cin >> baiLockedDepth; }
    else if (input == "waidepth")   { std::cout << "depth: "; std::cin >> waiLockedDepth; }
    else if (input == "cray") {
      BitBoard bb = chessBoard.getcray();
      chessBoard.display(blackSide, bb, "  popCount: " + intToString(bb.populationCount()));
      system("PAUSE");
    } else if (input == "checkers") {
      BitBoard bb;
      for (int i = 0; i < 64; ++i) if (chessCache.colorOfSquare(i)) bb.setSquare(i);
      chessBoard.display(blackSide, bb, "  popCount: " + intToString(bb.populationCount()));
      system("PAUSE");
    } else if (input == "pins") {
      BitBoard pinMasks[2][8], bb;
      chessBoard.getpinMasks(pinMasks);
      for (int t = 0; t < 2; ++t)
        for (int i = 0; i < 8; ++i) {
          bb.set(bb.get() | pinMasks[t][i].get());
          if (!pinMasks[t][i].isEmpty())
            chessBoard.display(blackSide, bb, "  popCount: " + intToString(bb.populationCount()));
        }
      system("PAUSE");
    } else if (input == "pieces") {
      BitBoard bb = chessBoard.getAllPIECES();
      chessBoard.display(blackSide, bb, "  popCount: " + intToString(bb.populationCount()));
      system("PAUSE");
    } else if (input == "watks") {
      BitBoard bb = chessBoard.getWhiteAtks();
      chessBoard.display(blackSide, bb, "  popCount: " + intToString(bb.populationCount()));
      system("PAUSE");
    } else if (input == "batks") {
      BitBoard bb = chessBoard.getBlackAtks();
      chessBoard.display(blackSide, bb, "  popCount: " + intToString(bb.populationCount()));
      system("PAUSE");
    } else if (input == "kings") {
      BitBoard bb;
      bb.setSquare(chessBoard.getWhiteKing());
      bb.setSquare(chessBoard.getBlackKing());
      chessBoard.display(blackSide, bb);
      system("PAUSE");
    } else if (input == "exit") {
      break;
    } else {
      int moveFrom = chessCache.notationToTile(input);
      cBoardHLight.setSquare(moveFrom);

      moveList genMoves;
      chessBoard.generateMoves(genMoves, true);
      for (int i = 0; i < genMoves.amt; ++i)
        if (genMoves.moves[i].move.moveFrom() == moveFrom)
          cBoardHLight.setSquare(genMoves.moves[i].move.moveTo());

      system("CLS");
      EvalBar(aiEval, chessBoard);
      chessBoard.display(blackSide, cBoardHLight, "  #moves: " + intToString(genMoves.amt));

      std::cout << "moveTo: ";
      std::cin >> input;
      int moveTo = chessCache.notationToTile(input);

      moveList Promotes;
      bool isPromotion = false;
      for (int i = 0; i < genMoves.amt; ++i) {
        if (genMoves.moves[i].move.moveFrom() == moveFrom &&
            genMoves.moves[i].move.moveTo()   == moveTo &&
            genMoves.moves[i].move.isPromotion()) {
          Promotes.addConstMove(genMoves.moves[i].move);
          isPromotion = true;
        }
      }

      if (isPromotion) {
        std::cout << "\npromote to:\n"
                  << "[q] " << PIECES.toUnicode(PIECES.QUEEN)  << " Queen\n"
                  << "[r] " << PIECES.toUnicode(PIECES.ROOK)   << " Rook\n"
                  << "[b] " << PIECES.toUnicode(PIECES.BISHOP) << " Bishop\n"
                  << "[n] " << PIECES.toUnicode(PIECES.KNIGHT) << " Knight\n";
        std::cin >> input;
        toLowercase(input);
        int idx = (input == "r") ? 1 : (input == "b") ? 2 : (input == "n") ? 3 : 0;
        chessBoard.makeMove(Promotes.moves[idx].move);
        previousMoves[chessBoardPly++] = Promotes.moves[idx].move;
      } else {
        for (int i = 0; i < genMoves.amt; ++i) {
          if (genMoves.moves[i].move.moveFrom() == moveFrom &&
              genMoves.moves[i].move.moveTo()   == moveTo) {
            chessBoard.makeMove(genMoves.moves[i].move);
            previousMoves[chessBoardPly++] = genMoves.moves[i].move;
            break;
          }
        }
      }
    }
  } while (true);
}

void runAITest(Board &chessBoard) {
  auto start = std::chrono::steady_clock::now();

  struct TestCase { const char *label; const char *fen; int depth; };
  TestCase tests[] = {
      {"endgame (tied rook vs two pawns)",    "5K2/8/7P/P7/7r/5k2/8/8 w - - 0 1",                                             8},
      {"mate in 5",                           "2q1nk1r/4Rp2/1ppp1P2/6Pp/3p1B2/3P3P/PPP1Q3/6K1 w",                           6},
      {"harder mate in 5",                    "6r1/p3p1rk/1p1pPp1p/q3n2R/4P3/3BR2P/PPP2QP1/7K w",                           7},
      {"english opening (+0.6)",              "r1bqk2r/ppppbppp/2n2n2/3Np3/2P5/5NP1/PP1PPPBP/R1BQK2R w KQkq - 5 6",        6},
      {"queens indian (+0.2)",                "rn1qk2r/p1p2ppp/bp2pn2/3p4/1bPP4/1P3NP1/P2BPPBP/RN1QK2R b KQkq - 3 4",     6},
      {"queens indian middlegame (+0.3)",     "2rr2k1/p2qbppp/5n2/2pB4/P1P2B2/6P1/4PP1P/1R1Q1RK1 b - - 0 19",              6},
      {"queens gambit middlegame (+0.9)",     "3nk2r/3q2pp/Q3b3/2R1Pp2/3p4/5N2/1p3PPP/1B4K1 w k - 1 24",                   6},
  };

  for (auto &tc : tests) {
    chessBoard.setupFen(tc.fen);
    searchRes r = chessBoard.oSearch(tc.depth);
    std::cout << "\n- " << tc.label << "\n  eval: " << r.eval << ",  nodes: " << r.nodes;
  }

  auto total = std::chrono::duration_cast<std::chrono::milliseconds>(
                   std::chrono::steady_clock::now() - start).count();
  std::cout << "\n\n - total time: " << intToString(static_cast<int>(total)) << "ms\n";
  system("PAUSE");
}

int main() {
  Board chessBoard;
  std::string input;
  allowEmojis();
  do {
    system("CLS");
    std::cout << " " << PIECES.toUnicode(PIECES.PAWN) << " CHESS MENU "
              << PIECES.toUnicode(PIECES.PAWN);
    setTxtColor(chessColors.greyLetCol);
    std::cout << " V7.4";
    setTxtColor(15);
    std::cout << "\n______________________\n"
              << "\n[p] Play";
    setTxtColor(chessColors.greyLetCol);
    std::cout << " (input \"help\" for commands)";
    setTxtColor(15);
    std::cout << "\n[v] Visuals"
              << "\n[t] Performance Test"
              << "\n[a] AI Test"
              << "\n[f] Custom FEN";
    setTxtColor(chessColors.greyLetCol);
    std::cout << " (starting position)";
    setTxtColor(15);
    std::cout << "\n[e] Exit\n";

    std::cin >> input;
    toLowercase(input);

    if      (input == "p") { startGame(chessBoard); }
    else if (input == "v") {
      std::cout << "[1] standard\n[2] blue vs red\n[3] all colors\n";
      std::cin >> input;
      if (input == "1") {
        chessColors.wBlackCol = 128; chessColors.wWhiteCol = 143;
        chessColors.bWhiteCol = 15;  chessColors.bBlackCol = 8;
      } else if (input == "2") {
        chessColors.wBlackCol = 12; chessColors.wWhiteCol = 11;
        chessColors.bWhiteCol = 11; chessColors.bBlackCol = 12;
      } else if (input == "3") {
        for (int i = 0; i < 16; ++i) {
          for (int x = 0; x < 16; ++x) {
            int color = i * 16 + x;
            setTxtColor(color);
            std::cout << color << (color <= 9 ? "  " : color <= 99 ? " " : "");
            setTxtColor(15);
          }
          std::cout << '\n';
        }
        system("PAUSE");
      }
    }
    else if (input == "t") { perftTest(chessBoard); }
    else if (input == "a") { runAITest(chessBoard); }
    else if (input == "f") { chessBoard.setupFen(askFen()); }
    else if (input == "e") { break; }
    else {
      std::cout << '"' << input << "\" invalid input\n";
      system("PAUSE");
    }
  } while (true);
  return 0;
}

// ---- Console/utility functions ----

void allowEmojis()   { SetConsoleOutputCP(CP_UTF8); }

void setTxtColor(int colorValue) {
  SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), colorValue);
}

std::string intToString(int num) {
  if (num == 0) return "0";
  return num < 0 ? "-" + std::to_string(-num) : std::to_string(num);
}

void toLowercase(std::string &input) {
  for (char &c : input)
    if (c >= 'A' && c <= 'Z') c += 32;
}

void printUint32Binary(uint32_t num) {
  for (int i = sizeof(num) * 8 - 1; i >= 0; --i) {
    std::cout << ((num >> i) & 1);
    if (i % 4 == 0) std::cout << " ";
  }
}

std::string invertFen(const std::string STR) {
  std::string result, word;
  for (char c : STR) {
    if (c != '/') {
      word += c;
    } else {
      for (int i = static_cast<int>(word.size()) - 1; i >= 0; --i) result += word[i];
      result += '/';
      word.clear();
    }
  }
  for (int i = static_cast<int>(word.size()) - 1; i >= 0; --i) result += word[i];
  return result;
}

uint16_t ***allocateMoveHHistory() {
  uint16_t ***h = new uint16_t**[2];
  for (int i = 0; i < 2; ++i) {
    h[i] = new uint16_t*[64];
    for (int j = 0; j < 64; ++j) {
      h[i][j] = new uint16_t[64]();  // zero-initialized
    }
  }
  return h;
}

void deallocateMoveHHistory(uint16_t ***h) {
  for (int i = 0; i < 2; ++i) {
    for (int j = 0; j < 64; ++j) delete[] h[i][j];
    delete[] h[i];
  }
  delete[] h;
}

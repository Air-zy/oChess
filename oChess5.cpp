/*=====================================
Programmer: Airzy T
Assignment:

Description(I.P.O):
    Input:
        -

    Process:
        -

    Outputs:
        -

Assumptions:
    - using smaller variable types like char/short instead of int to save
memmory
    - using ++i pre-increment insted of post-increment i++, its abit faster


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

TODO:
+ fix pins?
+ optimize move gen

=======================================*/

#include <windows.h>  // for console visuals

#include <algorithm>  // for move sorting
#include <chrono>     // for timing
#include <iostream>   //
#include <string>     // string to int stoi()

void setTxtColor(int colorValue);

void printShortBinary(short num) {
  for (int i = sizeof(num) * 8 - 1; i >= 0; --i) {
    std::cout << ((num >> i) & 1);
    if (i % 4 == 0) std::cout << " ";
  }
}

std::string intToString(int num) {
  std::string result;

  // Handle the case of zero separately
  if (num == 0) return "0";

  // Handle negative numbers
  bool isNegative = false;
  if (num < 0) {
    isNegative = true;
    num = -num;  // Convert num to positive
  }

  while (num != 0) {
    char digit = '0' + (num % 10);  // Convert the digit to ASCII
    result = digit + result;        // Append the digit to the result string
    num /= 10;                      // Move to the next digit
  }

  if (isNegative) result = '-' + result;  // Add the negative sign

  return result;
}

// just reverses/like/this --> sesrever/ekil/siht
std::string invertFen(const std::string str) {
  std::string reversedStr = "";
  std::string word = "";
  for (char c : str) {
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

std::string toLowercase(std::string input) {
  for (size_t i = 0; i < input.length(); ++i) {
    if (input[i] >= 'A' && input[i] <= 'Z') {
      input[i] = input[i] + 32;  // Convert uppercase letter to lowercase
    }
  }
  return input;
}

// instead of using boolean array
// 1ULL = 00000000 00000000 00000000 00000000 00000000 00000000 00000000
// 00000001
class BitBoard {
 private:
  uint64_t bitBoard = 0;  // 64 bits

 public:
  inline void setSquare(unsigned char square) {
    bitBoard |= (1ULL << square);  // set the bit corresponding to the index square
  };

  inline void unSetSquare(unsigned char square) {
    bitBoard &= ~(1ULL << square);  // clear the bit corresponding to the index square
  };

  inline bool isSet(unsigned char square) const {
    return (bitBoard >> square) & 1;  // check if the bit corresponding to the square is set
  };

  inline void clearBoard() {
    bitBoard = 0;  // clear the entire bitboard
  };

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
    return x & 0x7F;    // return only the least significant 7 bits (to handle// overflow)
  }

  inline uint64_t get() const { return bitBoard; };
  inline void set(uint64_t bb) { bitBoard = bb; };

  inline bool isEmpty() const { return bitBoard == 0; };

  BitBoard(){};
  BitBoard(uint64_t &newbb) { bitBoard = newbb; };
};

class PieceCache {
 private:
  const unsigned char TYPE_MASK = 0b111;    // 7
  const unsigned char COLOR_MASK = 0b1000;  // 8

  // stored unicodes for displaying pieces
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
  const unsigned char EMPTY = 0;   // 0000
  const unsigned char PAWN = 1;    // 0001
  const unsigned char KNIGHT = 2;  // 0010
  const unsigned char BISHOP = 3;  // 0011
  const unsigned char ROOK = 4;    // 0100
  const unsigned char QUEEN = 5;   // 0101
  const unsigned char KING = 6;    // 0110

  // colors
  // (value & 8) to get. Reason = 8 = 1000 color bitmask
  const unsigned char WHITE = 0;
  const unsigned char BLACK = 8;

  // pre defined pieces
  const unsigned char WPAWN = PAWN | WHITE;      // 1
  const unsigned char WKNIGHT = KNIGHT | WHITE;  // 2
  const unsigned char WBISHOP = BISHOP | WHITE;  // 3
  const unsigned char WROOK = ROOK | WHITE;      // 4
  const unsigned char WQUEEN = QUEEN | WHITE;    // 5
  const unsigned char WKING = KING | WHITE;      // 6

  const unsigned char BPAWN = PAWN | BLACK;      // 9
  const unsigned char BKNIGHT = KNIGHT | BLACK;  // 10
  const unsigned char BBISHOP = BISHOP | BLACK;  // 11
  const unsigned char BROOK = ROOK | BLACK;      // 12
  const unsigned char BQUEEN = QUEEN | BLACK;    // 13
  const unsigned char BKING = KING | BLACK;      // 14

  unsigned char type(const unsigned char &piece) const {
    return (piece & TYPE_MASK);
  }

  unsigned char color(const unsigned char &piece) const {
    return (piece & COLOR_MASK);
  }

  const std::string toUnicode(const unsigned char &pieceType) const {
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

const PieceCache pieces;

struct MoveCache {
  // Masks
  const unsigned short fromTileMask = 0b0000000000111111;
  const unsigned short toTileMask = 0b0000111111000000;
  const unsigned short flagMask = 0b1111000000000000;

  // Flags
  const unsigned char NoFlag = 0b0000;                // 0
  const unsigned char EnPassantCaptureFlag = 0b0001;  // 1
  const unsigned char CastleFlag = 0b0010;            // 2
  const unsigned char PawnTwoUpFlag = 0b0011;         // 3

  const unsigned char PromoteToQueenFlag = 0b0100;   // 4
  const unsigned char PromoteToKnightFlag = 0b0101;  // 5
  const unsigned char PromoteToRookFlag = 0b0110;    // 6
  const unsigned char PromoteToBishopFlag = 0b0111;  // 7

  // other
  const unsigned short Null = 0b0000000000000000;
};

const MoveCache moveFlags;

// Compact (16 bit) move representation to preserve memory during search.
// The format is as follows (ffffttttttssssss)
// Bits 0-5: start square index
// Bits 6-11: target square index
// Bits 12-15: flag (promotion type, ect)
class Move {
 private:
  // 16bit move value          ffffttttttssssss;
  unsigned short moveValue = 0;
  unsigned char flag() const { return moveValue >> 12; };

 public:
  inline unsigned char moveFrom() const { return moveValue & moveFlags.fromTileMask; };
  inline unsigned char moveTo() const { return (moveValue & moveFlags.toTileMask) >> 6; };

  inline bool isNull() const { return moveValue == moveFlags.Null; };
  inline bool isCastling() const { return flag() == moveFlags.CastleFlag; };
  inline bool isPawnTwoUp() const { return flag() == moveFlags.PawnTwoUpFlag; };
  inline bool isEnPassant() const { return flag() == moveFlags.EnPassantCaptureFlag; };
  inline bool promoteQueen() const { return flag() == moveFlags.PromoteToQueenFlag; };
  inline bool promoteRook() const { return flag() == moveFlags.PromoteToRookFlag; };
  inline bool promoteBishop() const {
    return flag() == moveFlags.PromoteToBishopFlag;
  };
  inline bool promoteKnight() const {
    return flag() == moveFlags.PromoteToKnightFlag;
  };
  inline bool isDoublePawnPush() const { return flag() == moveFlags.PawnTwoUpFlag; }
  inline void setFlag(unsigned char newFlag) {
    // Clear the existing flag bits
    moveValue &= ~(0b1111 << 12);
    // Set the new flag bits
    moveValue |= (newFlag << 12);
  }
  inline void clearMove() { moveValue = moveFlags.Null; };

  Move() { moveValue = moveFlags.Null; };

  Move(unsigned char fromTile, unsigned char toTile) {
    moveValue = (fromTile | (toTile << 6));
  };

  Move(unsigned char fromTile, unsigned char toTile, unsigned char flag) {
    moveValue = (fromTile | toTile << 6 | flag << 12);
  }
};

class PreComputedCache {
 private:
  unsigned char rowColValues[8][8];  // each element of [row][col] is a tile
 public:
  unsigned char preComputedRows[64];  // each element is set to which row its on
  unsigned char preComputedCols[64];  // each element is set to which col its on

  const int directionOffsets[8] = {
      // diagonals
      7,   // [0]
      9,   // [1]
      -9,  // [2]
      -7,  // [3]
      // orthogonals
      8,   // [4]
      -8,  // [5]
      -1,  // [6]
      1,   // [7]
  };

  unsigned char oppDir(unsigned char dirId) {
    switch (dirId) {
      case 0:
        return 3;
      case 1:
        return 2;
      case 2:
        return 1;
      case 3:
        return 0;
      case 4:
        return 5;
      case 5:
        return 4;
      case 6:
        return 7;
      case 7:
        return 6;
    }
    return 0;  // ??? impossible
  };

  // visual colors
  const unsigned char hlightCol = 240;  // White background Black text
  const unsigned char wBlackCol = 128;  // Gray background black letter
  const unsigned char wWhiteCol = 143;  // Gray background white letter
  const unsigned char bWhiteCol = 15;   // Black background white latter
  const unsigned char bBlackCol = 8;    // Black background gray letter
  const unsigned char greyLetCol = 8;   // Black background gray letter

  // green previous move
  const unsigned char prevMBlackCol = 32;
  const unsigned char prevMWhiteCol = 47;

  // red check color
  const unsigned char checkBlackCol = 207;  // 64;
  const unsigned char checkWhiteCol = 192;  // 79;

  // pre-compute pseudo legal moves for each tile
  Move bPawnMoves[64][8];  // 2 max possible moves 2 diagonal takes + promotions
  Move wPawnMoves[64][8];  // 2 max possible moves 2 diagonal takes + promotions
  Move knightMoves[64][8];  // 8 max possible moves
  Move kingMoves[64][8];    // 8 max possible moves

  // directions
  // [0] is
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
  int distances[64][64];  // [fromtile][totile]

  // normal rook tiles
  const unsigned char whiteKingRook = 7;
  const unsigned char whiteQueenRook = 0;
  const unsigned char blackKingRook = 63;
  const unsigned char blackQueenRook = 56;

  // castled rook tiles
  const unsigned char whiteKingRookCastleTo = 5;
  const unsigned char whiteQueenRookCastleTo = 3;
  const unsigned char blackKingRookCastleTo = 61;
  const unsigned char blackQueenRookCastleTo = 59;

  // castled king tiles
  const unsigned char whiteKingCastleTo = 6;
  const unsigned char whiteQueenCastleTo = 2;
  const unsigned char blackKingCastleTo = 62;
  const unsigned char blackQueenCastleTo = 58;

  // queen side vacant tiles - tiles in where it has to be empty to allow
  // castling
  const unsigned char whiteQueenCastleVacant = 1;
  const unsigned char blackQueenCastleVacant = 57;

  std::string notatedTiles[64];  // lookup array for tile notations
  unsigned char notationToTile(std::string &notation) const {
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

  std::string tileToNotation(const unsigned char &tile) const {
    return notatedTiles[tile];
  }

  // piece values
  const int pawnValue = 100 * 10;
  const int knightValue = 300 * 10;
  const int bishopValue = 300 * 10;
  const int rookValue = 500 * 10;
  const int queenValue = 900 * 10;
  // mult 2

  const int evalPositiveInf = 1000000;
  const int evalNegativeInf = -1000000;

  const int evalWhiteWins = 900000;
  const int evalWhiteLoss = -900000;

  // piece square tables
  int centerPST[64] = {
      0, 0,  1,  2,  2,  1,  0,  0,  //
      0, 10, 15, 15, 15, 15, 10, 0,  //
      0, 15, 30, 35, 35, 30, 15, 1,  //
      2, 15, 35, 40, 40, 35, 15, 2,  //
      2, 15, 35, 40, 40, 35, 15, 2,  //
      0, 15, 30, 35, 35, 30, 15, 1,  //
      0, 10, 15, 15, 15, 15, 10, 0,  //
      0, 0,  1,  2,  2,  1,  0,  0,  //
  };

  int statOrdPST[64] = {
      0, 0, 1, 2,  2,  1, 0, 0,  //
      0, 5, 4, 4,  4,  4, 5, 0,  //
      0, 4, 6, 8,  8,  6, 4, 1,  //
      2, 8, 8, 10, 10, 8, 8, 2,  //
      2, 8, 8, 10, 10, 8, 8, 2,  //
      0, 4, 6, 8,  8,  6, 4, 1,  //
      0, 5, 4, 4,  4,  4, 5, 0,  //
      0, 0, 1, 2,  2,  1, 0, 0,  //
  };

  int pawnPST[2][64] = {
      0,  0,  0,  0,   0,   0,  0,  0,   //
      60, 60, 60, 60,  60,  60, 60, 60,  //
      10, 10, 20, 30,  30,  20, 10, 10,  //
      0,  0,  0,  40,  40,  0,  0,  0,   //
      -5, -5, 10, 40,  40,  -5, -5, -5,  //
      0,  0,  0,  0,   0,   -5, 5,  3,   //
      0,  0,  -5, -40, -40, 0,  5,  3,   //
      0,  0,  0,  0,   0,   0,  0,  0,   //
  };

  int pawnEndPST[2][64] = {
      0,  0,  0,  0,  0,  0,  0,  0,   //
      90, 90, 90, 80, 80, 90, 90, 90,  //
      50, 50, 50, 40, 40, 50, 50, 50,  //
      30, 30, 30, 20, 20, 30, 30, 30,  //
      20, 20, 20, 20, 20, 20, 20, 20,  //
      15, 15, 15, 15, 15, 15, 15, 15,  //
      0,  0,  0,  0,  0,  0,  0,  0,   //
      0,  0,  0,  0,  0,  0,  0,  0,   //
  };

  int horsePST[2][64] = {
      -50, -30, -30, -30, -30, -30, -30, -50,  //
      -40, -20, 15,  0,   0,   15,  -20, -40,  //
      -30, 0,   15,  15,  15,  15,  0,   -30,  //
      -30, 5,   15,  20,  20,  15,  5,   -30,  //
      -30, 0,   15,  20,  20,  15,  0,   -30,  //
      -30, 5,   20,  15,  15,  20,  5,   -30,  //
      -40, -20, 0,   0,   0,   0,   -20, -40,  //
      -50, -50, -30, -30, -30, -30, -50, -50,  //
  };

  int bishopPST[2][64] = {
      -20, -10, -10, -10, -10, -10, -10, -20,  //
      -10, 10,  0,   0,   0,   0,   10,  -10,  //
      -10, 0,   5,   10,  10,  5,   0,   -10,  //
      -10, 5,   10,  10,  10,  10,  5,   -10,  //
      -10, 0,   10,  10,  10,  10,  0,   -10,  //
      -10, 10,  10,  5,   5,   10,  10,  -10,  //
      -10, 10,  10,  0,   0,   10,  10,  -10,  //
      -20, -10, -20, -10, -10, -20, -10, -20,  //
  };
  int rookPST[2][64] = {
      -5, -5, 0,  0,  0,  0,  -5, -5,  //
      -5, 10, 10, 10, 10, 10, 10, -5,  //
      -5, 0,  0,  0,  0,  0,  0,  -5,  //
      -5, 0,  0,  0,  0,  0,  0,  -5,  //
      -5, 0,  0,  0,  0,  0,  0,  -5,  //
      0,  0,  0,  0,  0,  0,  0,  0,   //
      -5, 0,  0,  0,  0,  0,  0,  -5,  //
      -5, -5, 5,  8,  8,  5,  -5, -5,  //
  };

  int kingPST[2][64] = {
      -60, -60, -60, -60, -60, -60, -60, -60,  //
      -60, -60, -60, -60, -60, -60, -60, -60,  //
      -60, -60, -60, -60, -60, -60, -60, -60,  //
      -60, -60, -60, -60, -60, -60, -60, -60,  //
      -40, -40, -60, -60, -60, -60, -40, -40,  //
      -40, -40, -40, -40, -40, -40, -40, -40,  //
      -40, -40, -40, -40, -40, -40, -40, -40,  //
      -5,  5,   -10, -30, -30, -30, 0,   -5,   //
      -5,  5,   5,   -30, -5,  -10, 5,   -5,   //
  };

  int value(const unsigned char &piece) const {
    unsigned char pieceType = pieces.type(piece);
    if (pieceType == pieces.PAWN) {
      return 10;
    } else if (pieceType == pieces.KNIGHT) {
      return 30;
    } else if (pieceType == pieces.BISHOP) {
      return 30;
    } else if (pieceType == pieces.ROOK) {
      return 50;
    } else if (pieceType == pieces.QUEEN) {
      return 90;
    }
    return 0;
  }

  bool inBounds(int tile) const { return tile >= 0 && tile < 64; };

  PreComputedCache() {
    int dfar = 0;
    // pre compute moves
    whiteKingSideCastle = Move(4, whiteKingCastleTo, moveFlags.CastleFlag);
    whiteQueenSideCastle = Move(4, whiteQueenCastleTo, moveFlags.CastleFlag);
    blackKingSideCastle = Move(60, blackKingCastleTo, moveFlags.CastleFlag);
    blackQueenSideCastle = Move(60, blackQueenCastleTo, moveFlags.CastleFlag);

    // piece square tables

    // Display the resulting array
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
    }

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
          wPawnMoves[i][4] = Move(i, i + 8, moveFlags.PromoteToQueenFlag);
          wPawnMoves[i][5] = Move(i, i + 8, moveFlags.PromoteToRookFlag);
          wPawnMoves[i][6] = Move(i, i + 8, moveFlags.PromoteToBishopFlag);
          wPawnMoves[i][7] = Move(i, i + 8, moveFlags.PromoteToKnightFlag);
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
          wPawnMoves[i][1] = Move(i, i + 16, moveFlags.PawnTwoUpFlag);
        }
      }

      // black pawns
      if (row > 0) {
        if (row == 1) {
          bPawnMoves[i][4] = Move(i, i - 8, moveFlags.PromoteToQueenFlag);
          bPawnMoves[i][5] = Move(i, i - 8, moveFlags.PromoteToRookFlag);
          bPawnMoves[i][6] = Move(i, i - 8, moveFlags.PromoteToBishopFlag);
          bPawnMoves[i][7] = Move(i, i - 8, moveFlags.PromoteToKnightFlag);
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
          bPawnMoves[i][1] = Move(i, i - 16, moveFlags.PawnTwoUpFlag);
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
      for (unsigned char j = 1; j < 8; ++j) {  // down
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
        for (unsigned char j = 1; j < 8; ++j) {  // left
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
        for (unsigned char j = 1; j < 8; ++j) {  // right
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
                                   // is used for pawn evaluation

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
              const Move &addingMove = rookMoves[d][i][r];
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
              const Move &addingMove = bishopMoves[d][i][r];
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

const PreComputedCache chessCache;

struct movePair {
  int score = 0;
  Move move;
};

struct moveList {
  movePair moves[218];  // 218 max moves in chess
  unsigned char amt = 0;

  void addConstMove(const Move &m) {
    if (!m.isNull()) {
      moves[amt].move = m;
      ++amt;
    }
  }
};

class pieceList {
 private:
  unsigned int pieceMap[64] = {0};  // to keep track of where pieces are
 public:
  unsigned int pieces[10] = {0};  // 10 possible of the same piece in chess
  int amt = 0;

  void addAtTile(int tile) {
    pieces[amt] = tile;
    pieceMap[tile] = amt;
    ++amt;
  };

  void removeAtTile(int tile) {
    pieces[pieceMap[tile]] = pieces[amt - 1];
    pieceMap[pieces[pieceMap[tile]]] = pieceMap[tile];
    //if (amt < 0) {
    //  std::cout << '\n' << amt << ", invalid amt for pieceList ";
    //  system("PAUSE");
    //}
    if (amt > 0) {
      --amt;
    }
  };

  void MovePiece(int startSquare, int targetSquare) {
    pieces[pieceMap[startSquare]] = targetSquare;
    pieceMap[targetSquare] = pieceMap[startSquare];
  };

  void clear() {
    for (int i = 0; i < 64; ++i) {
      pieceMap[i] = 0;
    };
    //for (int i = 0; i < 10; ++i) { // 10 max amt of that piece type
    //  pieces[i] = 0;
    //};
    amt = 0;
  }

  pieceList() { clear(); };
};

// optimized stack for chess
class gameStateStack {
 private:
  unsigned short gameStateHistory[400] = {0b0000000000000000};  // Array to hold game states
  int top;

 public:
  gameStateStack() : top(-1) {}

  bool isEmpty() const { return top == -1; };

  bool isFull() const { return top == 400 - 1; };

  void push(unsigned short gameState) {
    if (isFull()) {
      std::cout << "gameStack Overflow\n";
      system("PAUSE");
      return;
    }
    gameStateHistory[++top] = gameState;
  }

  unsigned short pop() {
    if (isEmpty()) {
      // std::cout << "Stack Underflow\n";
      return 0;  // Returning 0 as error value
    }
    return gameStateHistory[top--];
  }

  unsigned short peek() const {
    if (isEmpty()) {
      // std::cout << "Stack is empty\n";
      return 0;  // Returning 0 as error value
    }
    return gameStateHistory[top];
  }
};

// Function to allocate memory for moveHHistory array
int ***allocateMoveHHistory() {
  int ***moveHHistory =
      new int **[2];  // Allocate memory for the first dimension
  for (int i = 0; i < 2; ++i) {
    moveHHistory[i] =
        new int *[64];  // Allocate memory for the second dimension
    for (int j = 0; j < 64; ++j) {
      moveHHistory[i][j] =
          new int[64];  // Allocate memory for the third dimension
      // Initialize elements to zero
      for (int k = 0; k < 64; ++k) {
        moveHHistory[i][j][k] = 0;
      }
    }
  }
  return moveHHistory;
}

// Function to deallocate memory for moveHHistory array
void deallocateMoveHHistory(int ***moveHHistory) {
  for (int i = 0; i < 2; ++i) {
    for (int j = 0; j < 64; ++j) {
      delete[] moveHHistory[i][j];  // Deallocate memory for the third dimension
    }
    delete[] moveHHistory[i];  // Deallocate memory for the second dimension
  }
  delete[] moveHHistory;  // Deallocate memory for the first dimension
}

class Board {
 private:
  // consts
  const unsigned short whiteCastleKingsideMask = 0b1111111111111110;
  const unsigned short whiteCastleQueensideMask = 0b1111111111111101;
  const unsigned short blackCastleKingsideMask = 0b1111111111111011;
  const unsigned short blackCastleQueensideMask = 0b1111111111110111;

  const unsigned short whiteCastleKingsideBit = 0b0000000000000001;
  const unsigned short whiteCastleQueensideBit = 0b0000000000000010;
  const unsigned short blackCastleKingsideBit = 0b0000000000000100;
  const unsigned short blackCastleQueensideBit = 0b0000000000001000;

  const unsigned short whiteCastleMask =
      whiteCastleKingsideMask & whiteCastleQueensideMask;
  const unsigned short blackCastleMask =
      blackCastleKingsideMask & blackCastleQueensideMask;

  // prev moves, only used for display
  Move prevMove;

  // index position of the kings
  unsigned char whiteKing = 0;
  unsigned char blackKing = 0;

  // Total plies (half-moves) played in game
  int plyCount = 0;
  int fiftyMoveCounter = 0;
  int oppTurn = pieces.BLACK;  // opposite of turn
  int oppTurnIndex = 0;
  int turnIndex = 1;

  // stack like lists for faster move gen, one for black, one for white
  pieceList pawns[2];
  pieceList knights[2];
  pieceList bishops[2];
  pieceList rooks[2];
  pieceList queens[2];

  // gameState History
  // Bits 0-3 store white and black kingside/queenside castling legality
  // Bits 4-7 store file of ep square (starting at 1, so 0 = no ep square)
  // Bits 8-13 captured piece
  // Bits 14-... fifty mover counter
  gameStateStack gameStateHistory;
  unsigned short currentGameState = 0b0000000000000000;
  bool whiteKingSideCastle() const {
    return (currentGameState & whiteCastleKingsideBit);
  };
  bool whiteQueenSideCastle() const {
    return (currentGameState & whiteCastleQueensideBit);
  };
  bool blackKingSideCastle() const {
    return (currentGameState & blackCastleKingsideBit);
  };
  bool blackQueenSideCastle() const {
    return (currentGameState & blackCastleQueensideBit);
  };

  // bitboards
  BitBoard allPieces;
  BitBoard whiteAtks;
  BitBoard blackAtks;
  BitBoard checkRay;
  BitBoard blockRay;
  BitBoard pinMasks[2][8];  // pins possible for each direction
  bool pinExistInPosition = false;

  // board
  unsigned int board[64];

  void deleteTile(const unsigned char &index) {
    board[index] = pieces.EMPTY;
    allPieces.unSetSquare(index);
  }

  unsigned char getEnPassantFile() const {
    return (currentGameState >> 4) & 15;
  }

  void resetValues() {
    checkRay.clearBoard();
    blockRay.clearBoard();
    allPieces.clearBoard();
    blackAtks.clearBoard();
    whiteAtks.clearBoard();
    for (int d = 0; d < 8; ++d) {
      pinMasks[0][d].clearBoard();
      pinMasks[1][d].clearBoard();
    }

    currentGameState = 0;
    for (int i = 0; i < 400; ++i) {
      gameStateHistory.pop();  // clear history
    };
    for (int i = 0; i < 64; ++i) {
      board[i] = pieces.EMPTY;
    };
    allPieces.clearBoard();
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
    turn = pieces.WHITE;
    oppTurnIndex = 0;
    turnIndex = 1;
    oppTurn = pieces.BLACK;
  };

  // bools for more efficient access on checks
  bool boolWhiteCheck = false;
  bool boolBlackCheck = false;
  bool genQuiets = true;
  unsigned int checkCount = 0;

 public:
  int turn = pieces.WHITE;

  // for move oredring
  //int moveHHistory[2][64][64] = {0};  // heuristic history
  int ***moveHHistory = allocateMoveHHistory(); // 3d pointer array

  void getpinMasks(BitBoard pmasks[2][8]) const {
    for (int t = 0; t < 2; ++t) {
      for (int d = 0; d < 8; ++d) {
        pmasks[t][d] = pinMasks[t][d];
      }
    }
  }

  BitBoard getcray() const { return checkRay; };
  BitBoard getAllPieces() const { return allPieces; };
  BitBoard getWhiteAtks() const { return whiteAtks; };
  BitBoard getBlackAtks() const { return blackAtks; };
  pieceList getWPawns() const { return pawns[1]; };
  pieceList getBPawns() const { return pawns[0]; };
  pieceList getWRooks() const { return rooks[1]; };
  pieceList getBRooks() const { return rooks[0]; };
  unsigned char getWhiteKing() const { return whiteKing; };
  unsigned char getBlackKing() const { return blackKing; };
  bool whiteInCheck() const { return blackAtks.isSet(whiteKing); }
  bool blackInCheck() const { return whiteAtks.isSet(blackKing); }

  // makes sure pieces lists and bitboards are in sync with int board
  bool isSynced() {  // returns if board matches bitboards and lists
    int bpawnCount = 0;
    int wpawnCount = 0;
    for (int i = 0; i < 64; ++i) {
      if (board[i] == pieces.BPAWN) {
        ++bpawnCount;
      } else if (board[i] == pieces.WPAWN) {
        ++wpawnCount;
      }
      if (board[i] == pieces.EMPTY && allPieces.isSet(i)) {  // set but empty ?!
        std::cout << "\nunsync allPieces1[" << i << "]\n";
        return false;
      } else if (board[i] != pieces.EMPTY &&
                 !allPieces.isSet(i)) {  // unset but not empty??
        std::cout << "\nunsync allPieces2[" << i << "]\n";
        return false;
      }
    }
    for (int i = 0; i < rooks[0].amt; ++i) {
      if (board[rooks[0].pieces[i]] != pieces.BROOK) {
        std::cout << "\nunsync list, brook[" << intToString(rooks[0].pieces[i])
                  << "]\n";
        return false;
      }
    }
    for (int i = 0; i < rooks[1].amt; ++i) {
      if (board[rooks[1].pieces[i]] != pieces.WROOK) {
        std::cout << "\nunsync list, wrook[" << intToString(rooks[1].pieces[i])
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
      if (board[pawns[0].pieces[i]] != pieces.BPAWN) {
        std::cout << "\nunsync list, bpawn[" << intToString(pawns[0].pieces[i])
                  << "]\n";
        return false;
      }
    }
    for (int i = 0; i < pawns[1].amt; ++i) {
      if (board[pawns[1].pieces[i]] != pieces.WPAWN) {
        std::cout << "\nunsync list, wpawn[" << intToString(pawns[1].pieces[i])
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
      const unsigned char &moveTo = cMove.moveTo();
      const unsigned char &moveFrom = cMove.moveFrom();
      const unsigned int &captured = board[moveTo];
      const int &myValue = chessCache.value(board[moveFrom]);

      int turnI = 0;
      cMPair.score = chessCache.statOrdPST[moveTo];
      if (turn == pieces.WHITE) {
        if (blackAtks.isSet(moveTo)) { // if moves to enemy guarded square
          cMPair.score = -myValue; // order captures higher
          if (captured != pieces.EMPTY) {
            cMPair.score += chessCache.value(captured) + 100;
          }
        } else { // moves to unguarded square
          cMPair.score += chessCache.value(captured);
        }
        if (captured == pieces.EMPTY) {
            cMPair.score += moveHHistory[turnI][moveFrom][moveTo];
        }
      } else {
        if (whiteAtks.isSet(moveTo)) { // if moves to enemy guarded square
          cMPair.score = -myValue; // order captures higher
          if (captured != pieces.EMPTY) {
            cMPair.score += chessCache.value(captured) + 100;
          }
        } else { // moves to unguarded square
          cMPair.score += chessCache.value(captured);
        }
        if (captured == pieces.EMPTY) {
            cMPair.score += moveHHistory[0][moveFrom][moveTo];
        }
      }

      if (cMove.promoteBishop() || cMove.promoteKnight() || cMove.promoteRook() || cMove.promoteQueen()) {
        cMPair.score += 1000;
      }
    }
    std::sort(moves.moves, moves.moves + moves.amt,
              [](const movePair &a, const movePair &b) {
                return a.score > b.score;  // Sort in descending order of score
              });
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
      turn = pieces.WHITE;
      oppTurnIndex = 0;
      turnIndex = 1;
    } else if (fenTurn == "b") {
      turn = pieces.BLACK;
      oppTurnIndex = 1;
      turnIndex = 0;
    }

    if (enPassantTargetSQR != "-") {
      unsigned char enPassSquare = chessCache.notationToTile(enPassantTargetSQR);
      int file = chessCache.preComputedCols[enPassSquare] + 1;  // +1 cuz 0 means none
      currentGameState |= (file << 4);
    }

    if (halfMoveClockFen != "-") {
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

    std::string fen = invertFen(unflippedFen);
    char index = 63;
    for (int i = 0; i < fen.length(); ++i) {
      char letter = fen[i];
      if (isdigit(letter)) {
        for (char j = 0; j < letter - '0'; ++j) {
          board[index] = pieces.EMPTY;
          index--;
        }
      } else {
        switch (letter) {
          case 'p':
            pawns[0].addAtTile(index);
            board[index] = pieces.BPAWN;
            allPieces.setSquare(index);
            index--;
            break;
          case 'n':
            knights[0].addAtTile(index);
            board[index] = pieces.BKNIGHT;
            allPieces.setSquare(index);
            index--;
            break;
          case 'b':
            bishops[0].addAtTile(index);
            board[index] = pieces.BBISHOP;
            allPieces.setSquare(index);
            index--;
            break;
          case 'r':
            rooks[0].addAtTile(index);
            board[index] = pieces.BROOK;
            allPieces.setSquare(index);
            index--;
            break;
          case 'q':
            queens[0].addAtTile(index);
            board[index] = pieces.BQUEEN;
            allPieces.setSquare(index);
            index--;
            break;
          case 'k':
            blackKing = index;
            board[index] = pieces.BKING;
            allPieces.setSquare(index);
            index--;
            break;
          case 'P':
            pawns[1].addAtTile(index);
            board[index] = pieces.WPAWN;
            allPieces.setSquare(index);
            index--;
            break;
          case 'N':
            knights[1].addAtTile(index);
            board[index] = pieces.WKNIGHT;
            allPieces.setSquare(index);
            index--;
            break;
          case 'B':
            bishops[1].addAtTile(index);
            board[index] = pieces.WBISHOP;
            allPieces.setSquare(index);
            index--;
            break;
          case 'R':
            rooks[1].addAtTile(index);
            board[index] = pieces.WROOK;
            allPieces.setSquare(index);
            index--;
            break;
          case 'Q':
            queens[1].addAtTile(index);
            board[index] = pieces.WQUEEN;
            allPieces.setSquare(index);
            index--;
            break;
          case 'K':
            whiteKing = index;
            board[index] = pieces.WKING;
            allPieces.setSquare(index);
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
        setTxtColor(chessCache.greyLetCol);
        std::cout << (row2 + 1) << '|';
      }

      if (highlights.isSet(i2)) {
        setTxtColor(chessCache.hlightCol);
      } else {
        int colorIndex = (row2 + col) % 2;
        int pieceColor = pieces.color(board[i2]) == 0 ? 0 : 1;
        if (colorIndex) {
          setTxtColor(pieceColor == 0 ? chessCache.wWhiteCol
                                      : chessCache.wBlackCol);
        } else {
          setTxtColor(pieceColor == 0 ? chessCache.bWhiteCol
                                      : chessCache.bBlackCol);
        };

        if (!prevMove.isNull() &&
            (prevMove.moveFrom() == i2 || prevMove.moveTo() == i2)) {
          if (pieces.color(board[prevMove.moveTo()]) == pieces.WHITE) {
            setTxtColor(chessCache.prevMWhiteCol);
          } else {
            setTxtColor(chessCache.prevMBlackCol);
          }
        }

        if (checkCount != 0) {
          if (i2 == whiteKing && checkRay.isSet(i2)) {
            setTxtColor(chessCache.checkBlackCol);
          }
          if (i2 == blackKing && checkRay.isSet(i2)) {
            setTxtColor(chessCache.checkWhiteCol);
          }
        }

        if (blockRay.isSet(i2)) {
          if (pieces.color(board[i2]) == pieces.WHITE) {
            setTxtColor(chessCache.checkBlackCol);
          } else {
            setTxtColor(chessCache.checkWhiteCol);
          }
        }
      }
      std::cout << pieces.toUnicode(pieces.type(board[i2])) << " ";
      if ((i + 1) % 8 == 0) {
        setTxtColor(chessCache.greyLetCol);
        if (chessCache.preComputedRows[i] == 0) {
          std::cout << "  ply: " << plyCount;
          std::cout << ", ";
          if (turn) {  // if blacks turn
            std::cout << " turn: b, ";
          } else {
            std::cout << " turn: w, ";
          }
          std::cout << (whiteKingSideCastle() ? "K" : "-");
          std::cout << (whiteQueenSideCastle() ? "Q" : "-");
          std::cout << (blackKingSideCastle() ? "k" : "-");
          std::cout << (blackQueenSideCastle() ? "q" : "-");
          //std::cout << ",  " << intToString(fiftyMoveCounter);
          std::cout << ",  ( ";
          printShortBinary(currentGameState);
          std::cout << ")";
        } else if (chessCache.preComputedRows[i] == 1) {
          std::cout << line1;
        } else if (chessCache.preComputedRows[i] == 2) {
          std::cout << line2;
        } else if (chessCache.preComputedRows[i] == 3) {
          std::cout << line3;
        } else if (chessCache.preComputedRows[i] == 4) {
          std::cout << line4;
        } else if (chessCache.preComputedRows[i] == 7) {
          std::cout << "  hEval: " << heuristicEval();
        }
        setTxtColor(15);
        std::cout << '\n';
      }
    }
    setTxtColor(chessCache.greyLetCol);
    std::cout << "  a b c d e f g h\n";
    setTxtColor(15);
  };

  void makeTurn() {
    oppTurn = turn;
    if (turn) {  // if blacks turn
      oppTurnIndex = 0;
      turnIndex = 1;
      turn = pieces.WHITE;
    } else {
      oppTurnIndex = 1;
      turnIndex = 0;
      turn = pieces.BLACK;
    }
  };

  bool isCapture(const Move &m) const {
    return allPieces.isSet(m.moveTo());
    // return (board[m.moveTo()] != pieces.EMPTY);
  };

  void addLegal(moveList &ML, const Move &m) const {
    if (genQuiets == false && !isCapture(m)) {  // remove if not capture in captures only
      return;
    }
    // if empty capture or captures enemy
    const unsigned int &mto = m.moveTo();
    if (!blockRay.isEmpty()) {
      if (!blockRay.isSet(mto)) {
        return;
      }
    }
    if (pinExistInPosition) {
      for (int d = 0; d < 8; ++d) {
        if (pinMasks[oppTurnIndex][d].isSet(m.moveFrom())) {  // if piece moving is pinned
          if (!pinMasks[oppTurnIndex][d].isSet(mto)) {  // restricts movement to only the pinmask
            return;
          }
        };
      }
    }
    if (board[mto] == pieces.EMPTY || turn != pieces.color(board[mto])) {
      if (mto != whiteKing && mto != blackKing) {  // and not king captures
        ML.addConstMove(m);
      }
    }
  };

  void addPLegal(moveList &ML, const Move &m) const {
    if (genQuiets == false &&
        !isCapture(m)) {  // remove if not capture in captures only
      return;
    }

    const unsigned int &mto = m.moveTo();
    if (board[mto] == pieces.EMPTY || turn != pieces.color(board[mto])) {
      if (mto != whiteKing && mto != blackKing) {  // and not king captures
        ML.addConstMove(m);
      }
    }
  };

  // manually written/hard coded for less looping, so its a bit faster
  void generatePawnMoves(moveList &m) const {
    if (turn == pieces.WHITE) {
      for (int i = 0; i < pawns[1].amt; ++i) {
        int pieceIndex = pawns[1].pieces[i];

        if (genQuiets) {
          const Move &pawnPush = chessCache.wPawnMoves[pieceIndex][0];
          if (!pawnPush.isNull() && !allPieces.isSet(pawnPush.moveTo())) {
            if (chessCache.preComputedRows[pieceIndex] == 6) {
              addLegal(m, chessCache.wPawnMoves[pieceIndex][4]);  // Queen
              addLegal(m, chessCache.wPawnMoves[pieceIndex][5]);  // Rook
              addLegal(m, chessCache.wPawnMoves[pieceIndex][6]);  // Bishop
              addLegal(m, chessCache.wPawnMoves[pieceIndex][7]);  // Knight
            } else {
              addLegal(m, pawnPush);  // Single Push
            }
            if (!allPieces.isSet(
                    chessCache.wPawnMoves[pieceIndex][1].moveTo())) {
              addLegal(m, chessCache.wPawnMoves[pieceIndex][1]);  // Double Push
            }
          }
        }
        
        Move addingMove = chessCache.wPawnMoves[pieceIndex][2];
        unsigned char enPFile = getEnPassantFile();
        if (!addingMove.isNull()) {
          if (isCapture(addingMove)) {
            if (chessCache.preComputedRows[pieceIndex] == 6) {
              addingMove.setFlag(moveFlags.PromoteToQueenFlag);
              addLegal(m, addingMove);  // Promote
              addingMove.setFlag(moveFlags.PromoteToRookFlag);
              addLegal(m, addingMove);  // Promote
              addingMove.setFlag(moveFlags.PromoteToBishopFlag);
              addLegal(m, addingMove);  // Promote
              addingMove.setFlag(moveFlags.PromoteToKnightFlag);
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
            addingMove.setFlag(moveFlags.EnPassantCaptureFlag);
            m.addConstMove(addingMove);  // en passant capture
          }
        }
        Move addingMove2 = chessCache.wPawnMoves[pieceIndex][3];
        unsigned char enPFile2 = getEnPassantFile();
        if (!addingMove2.isNull()) {
          if (isCapture(addingMove2)) {
            if (chessCache.preComputedRows[pieceIndex] == 6) {
              addingMove2.setFlag(moveFlags.PromoteToQueenFlag);
              addLegal(m, addingMove2);  // Promote
              addingMove2.setFlag(moveFlags.PromoteToRookFlag);
              addLegal(m, addingMove2);  // Promote
              addingMove2.setFlag(moveFlags.PromoteToBishopFlag);
              addLegal(m, addingMove2);  // Promote
              addingMove2.setFlag(moveFlags.PromoteToKnightFlag);
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
            addingMove2.setFlag(moveFlags.EnPassantCaptureFlag);
            m.addConstMove(addingMove2);  // en passant capture
          }
        }
      }
    } else {  // if black turn
      for (int i = 0; i < pawns[0].amt; ++i) {
        int pieceIndex = pawns[0].pieces[i];
        const Move &pawnPush = chessCache.bPawnMoves[pieceIndex][0];
        if (!pawnPush.isNull() && !allPieces.isSet(pawnPush.moveTo())) {
          if (chessCache.preComputedRows[pieceIndex] == 1) {
            addLegal(m, chessCache.bPawnMoves[pieceIndex][4]);  // Queen
            addLegal(m, chessCache.bPawnMoves[pieceIndex][5]);  // Rook
            addLegal(m, chessCache.bPawnMoves[pieceIndex][6]);  // Bishop
            addLegal(m, chessCache.bPawnMoves[pieceIndex][7]);  // Knight
          } else {
            addLegal(m, pawnPush);  // Single Push
          }
          if (!allPieces.isSet(chessCache.bPawnMoves[pieceIndex][1].moveTo())) {
            addLegal(m, chessCache.bPawnMoves[pieceIndex][1]);
          }
        }
        Move addingMove = chessCache.bPawnMoves[pieceIndex][2];
        if (!addingMove.isNull()) {
          unsigned char enPFile = getEnPassantFile();
          if (isCapture(addingMove)) {
            if (chessCache.preComputedRows[pieceIndex] == 1) {
              addingMove.setFlag(moveFlags.PromoteToQueenFlag);
              addLegal(m, addingMove);  // Promote
              addingMove.setFlag(moveFlags.PromoteToRookFlag);
              addLegal(m, addingMove);  // Promote
              addingMove.setFlag(moveFlags.PromoteToBishopFlag);
              addLegal(m, addingMove);  // Promote
              addingMove.setFlag(moveFlags.PromoteToKnightFlag);
              addLegal(m, addingMove);  // Promote
            } else {
              addLegal(m, addingMove);  // Diagonal Capture
            }
          } else if (enPFile != 0 &&
                     chessCache.preComputedCols[addingMove.moveTo()] ==
                         enPFile - 1 &&
                     chessCache.preComputedRows[addingMove.moveFrom()] == 3) {
            addingMove.setFlag(moveFlags.EnPassantCaptureFlag);
            m.addConstMove(addingMove);  // en passant capture
          }
        }
        Move addingMove2 = chessCache.bPawnMoves[pieceIndex][3];
        if (!addingMove2.isNull()) {
          unsigned char enPFile2 = getEnPassantFile();
          if (isCapture(addingMove2)) {
            if (chessCache.preComputedRows[pieceIndex] == 1) {
              addingMove2.setFlag(moveFlags.PromoteToQueenFlag);
              addLegal(m, addingMove2);  // Promote
              addingMove2.setFlag(moveFlags.PromoteToRookFlag);
              addLegal(m, addingMove2);  // Promote
              addingMove2.setFlag(moveFlags.PromoteToBishopFlag);
              addLegal(m, addingMove2);  // Promote
              addingMove2.setFlag(moveFlags.PromoteToKnightFlag);
              addLegal(m, addingMove2);  // Promote
            } else {
              addLegal(m, addingMove2);  // Diagonal Capture
            }
          } else if (enPFile2 != 0 &&
                     chessCache.preComputedCols[addingMove2.moveTo()] ==
                         enPFile2 - 1 &&
                     chessCache.preComputedRows[addingMove2.moveFrom()] == 3) {
            addingMove2.setFlag(moveFlags.EnPassantCaptureFlag);
            m.addConstMove(addingMove2);  // en passant capture
          }
        }
      }
    }
  };

  // manually written/hard coded for less looping, so its a bit faster
  void generateKnightMoves(moveList &m) const {
    if (turn == pieces.WHITE) {
      for (int i = 0; i < knights[1].amt; ++i) {
        for (int j = 0; j < 8; ++j) {
          addLegal(m, chessCache.knightMoves[knights[1].pieces[i]][j]);
        }
      }
    } else {
      for (int i = 0; i < knights[0].amt; ++i) {
        for (int j = 0; j < 8; ++j) {
          addLegal(m, chessCache.knightMoves[knights[0].pieces[i]][j]);
        }
      }
    }
  };

  // manually written/hard coded for less looping, so its a bit faster
  void generateBishopMoves(moveList &m) const {
    if (turn == pieces.WHITE) {
      for (int i = 0; i < bishops[1].amt; ++i) {
        // for each direction
        for (int d = 0; d < 4; ++d) {
          for (int r = 0; r < 7; ++r) {
            const Move &addingMove =
                chessCache.bishopMoves[d][bishops[1].pieces[i]][r];
            addLegal(m, addingMove);
            if (allPieces.isSet(addingMove.moveTo())) {
              break;
            }
          }
        }
      }

      for (int i = 0; i < queens[1].amt; ++i) {
        // for each direction
        for (int d = 0; d < 4; ++d) {
          for (int r = 0; r < 7; ++r) {
            const Move &addingMove =
                chessCache.bishopMoves[d][queens[1].pieces[i]][r];
            addLegal(m, addingMove);
            if (allPieces.isSet(addingMove.moveTo())) {
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
            const Move &addingMove =
                chessCache.bishopMoves[d][bishops[0].pieces[i]][r];
            addLegal(m, addingMove);
            if (allPieces.isSet(addingMove.moveTo())) {
              break;
            }
          }
        }
      }

      for (int i = 0; i < queens[0].amt; ++i) {
        // for each direction
        for (int d = 0; d < 4; ++d) {
          for (int r = 0; r < 7; ++r) {
            const Move &addingMove =
                chessCache.bishopMoves[d][queens[0].pieces[i]][r];
            addLegal(m, addingMove);
            if (allPieces.isSet(addingMove.moveTo())) {
              break;
            }
          }
        }
      }
    }
  };

  // manually written/hard coded for less looping, so its a bit faster
  void generateRookMoves(moveList &m) const {
    if (turn == pieces.WHITE) {
      for (int i = 0; i < rooks[1].amt; ++i) {
        // for each direction
        for (int d = 0; d < 4; ++d) {
          for (int r = 0; r < 7; ++r) {
            const Move &addingMove =
                chessCache.rookMoves[d][rooks[1].pieces[i]][r];
            addLegal(m, addingMove);
            if (allPieces.isSet(addingMove.moveTo())) {
              break;
            }
          }
        }
      }

      for (int i = 0; i < queens[1].amt; ++i) {
        // for each direction
        for (int d = 0; d < 4; ++d) {
          for (int r = 0; r < 7; ++r) {
            const Move &addingMove =
                chessCache.rookMoves[d][queens[1].pieces[i]][r];
            addLegal(m, addingMove);
            if (allPieces.isSet(addingMove.moveTo())) {
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
            const Move &addingMove =
                chessCache.rookMoves[d][rooks[0].pieces[i]][r];
            addLegal(m, addingMove);
            if (allPieces.isSet(addingMove.moveTo())) {
              break;
            }
          }
        }
      }

      for (int i = 0; i < queens[0].amt; ++i) {
        // for each direction
        for (int d = 0; d < 4; ++d) {
          for (int r = 0; r < 7; ++r) {
            const Move &addingMove =
                chessCache.rookMoves[d][queens[0].pieces[i]][r];
            addLegal(m, addingMove);
            if (allPieces.isSet(addingMove.moveTo())) {
              break;
            }
          }
        }
      }
    }
  };

  // manually written/hard coded for less looping, so its a bit faster
  void generateKingMoves(moveList &m) const {
    if (turn == pieces.WHITE) {
      for (int j = 0; j < 8; ++j) {
        const Move &addingMove = chessCache.kingMoves[whiteKing][j];
        if (!blackAtks.isSet(addingMove.moveTo()) &&
            !checkRay.isSet(addingMove.moveTo())) {
          addPLegal(m, addingMove);
        }
      }
      if (!whiteInCheck()) {  // if not in check allow castling
        if (whiteKingSideCastle()) {
          if (board[chessCache.whiteKingCastleTo] == pieces.EMPTY &&
              board[chessCache.whiteKingRookCastleTo] == pieces.EMPTY &&
              !blackAtks.isSet(chessCache.whiteKingCastleTo) &&
              !blackAtks.isSet(
                  chessCache.whiteKingRookCastleTo)) {  // and not attacked
            m.addConstMove(chessCache.whiteKingSideCastle);
          }
        }
        if (whiteQueenSideCastle()) {
          if (board[chessCache.whiteQueenCastleTo] == pieces.EMPTY &&
              board[chessCache.whiteQueenRookCastleTo] == pieces.EMPTY &&
              board[chessCache.whiteQueenCastleVacant] == pieces.EMPTY &&
              !blackAtks.isSet(chessCache.whiteQueenCastleTo) &&
              !blackAtks.isSet(
                  chessCache.whiteQueenRookCastleTo)) {  // and not attacked
            m.addConstMove(chessCache.whiteQueenSideCastle);
          }
        }
      }
    } else {
      for (int j = 0; j < 8; ++j) {
        const Move &addingMove = chessCache.kingMoves[blackKing][j];
        if (!whiteAtks.isSet(addingMove.moveTo()) &&
            !checkRay.isSet(addingMove.moveTo())) {
          addPLegal(m, addingMove);
        }
      }
      if (!blackInCheck()) {  // if not in check allow castling
        if (blackKingSideCastle()) {
          if (board[chessCache.blackKingCastleTo] == pieces.EMPTY &&
              board[chessCache.blackKingRookCastleTo] == pieces.EMPTY &&
              !whiteAtks.isSet(chessCache.blackKingCastleTo) &&
              !whiteAtks.isSet(
                  chessCache.blackKingRookCastleTo)) {  // and not attacked
            m.addConstMove(chessCache.blackKingSideCastle);
          }
        }
        if (blackQueenSideCastle()) {
          if (board[chessCache.blackQueenCastleTo] == pieces.EMPTY &&
              board[chessCache.blackQueenRookCastleTo] == pieces.EMPTY &&
              board[chessCache.blackQueenCastleVacant] == pieces.EMPTY &&
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
      int pieceIndex = pawns[1].pieces[i];
      const Move &addingMove = chessCache.wPawnMoves[pieceIndex][2];
      if (!addingMove.isNull()) {
        whiteAtks.setSquare(addingMove.moveTo());
        if (addingMove.moveTo() == blackKing) {
          blockRay.setSquare(pieceIndex);
          checkRay.setSquare(blackKing);
          ++checkCount;
        }
      }
      const Move &addingMove2 = chessCache.wPawnMoves[pieceIndex][3];
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
      int pieceIndex = pawns[0].pieces[i];
      const Move &addingMove = chessCache.bPawnMoves[pieceIndex][2];
      if (!addingMove.isNull()) {
        blackAtks.setSquare(addingMove.moveTo());
        if (addingMove.moveTo() == whiteKing) {
          blockRay.setSquare(pieceIndex);
          checkRay.setSquare(whiteKing);
          ++checkCount;
        }
      }
      const Move &addingMove2 = chessCache.bPawnMoves[pieceIndex][3];
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
    // if (turn == pieces.WHITE) {
    for (int i = 0; i < knights[1].amt; ++i) {
      for (int j = 0; j < 8; ++j) {
        const Move &addingMove =
            chessCache.knightMoves[knights[1].pieces[i]][j];
        if (!addingMove.isNull()) {
          whiteAtks.setSquare(addingMove.moveTo());
          if (addingMove.moveTo() == blackKing) {
            blockRay.setSquare(knights[1].pieces[i]);
            checkRay.setSquare(blackKing);
            ++checkCount;
          }
        }
      }
    }
    //} else {
    for (int i = 0; i < knights[0].amt; ++i) {
      for (int j = 0; j < 8; ++j) {
        const Move &addingMove =
            chessCache.knightMoves[knights[0].pieces[i]][j];
        if (!addingMove.isNull()) {
          blackAtks.setSquare(addingMove.moveTo());
          if (addingMove.moveTo() == whiteKing) {
            blockRay.setSquare(knights[0].pieces[i]);
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
    // if (turn == pieces.WHITE) {
    for (int i = 0; i < bishops[1].amt; ++i) {
      // for each direction
      for (int d = 0; d < 4; ++d) {
        for (int r = 0; r < 7; ++r) {
          const Move &addingMove =
              chessCache.bishopMoves[d][bishops[1].pieces[i]][r];
          if (!addingMove.isNull()) {
            whiteAtks.setSquare(addingMove.moveTo());
            if (addingMove.moveTo() == blackKing) {
              checkRay =
                  chessCache.rays1Extra[addingMove.moveFrom()][blackKing];
              blockRay = chessCache.rays[addingMove.moveFrom()][blackKing];
              ++checkCount;
            }
            if (allPieces.isSet(addingMove.moveTo())) {
              if (r < 6) {
                for (int r2 = r + 1; r2 < 7; ++r2) {
                  const Move &continueMove =
                      chessCache.bishopMoves[d][bishops[1].pieces[i]][r2];
                  if (!continueMove.isNull()) {
                    unsigned char moveTo = continueMove.moveTo();
                    if (allPieces.isSet(moveTo)) {
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
          const Move &addingMove =
              chessCache.bishopMoves[d][queens[1].pieces[i]][r];
          if (!addingMove.isNull()) {
            whiteAtks.setSquare(addingMove.moveTo());
            if (addingMove.moveTo() == blackKing) {
              checkRay =
                  chessCache.rays1Extra[addingMove.moveFrom()][blackKing];
              blockRay = chessCache.rays[addingMove.moveFrom()][blackKing];
              ++checkCount;
            }
            if (allPieces.isSet(addingMove.moveTo())) {
              if (r < 6) {
                for (int r2 = r + 1; r2 < 7; ++r2) {
                  const Move &continueMove =
                      chessCache.bishopMoves[d][queens[1].pieces[i]][r2];
                  if (!continueMove.isNull()) {
                    unsigned char moveTo = continueMove.moveTo();
                    if (allPieces.isSet(moveTo)) {
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
          const Move &addingMove =
              chessCache.bishopMoves[d][bishops[0].pieces[i]][r];
          if (!addingMove.isNull()) {
            blackAtks.setSquare(addingMove.moveTo());
            if (addingMove.moveTo() == whiteKing) {
              checkRay =
                  chessCache.rays1Extra[addingMove.moveFrom()][whiteKing];
              blockRay = chessCache.rays[addingMove.moveFrom()][whiteKing];
              ++checkCount;
            }
            if (allPieces.isSet(addingMove.moveTo())) {
              if (r < 6) {
                for (int r2 = r + 1; r2 < 7; ++r2) {
                  const Move &continueMove =
                      chessCache.bishopMoves[d][bishops[0].pieces[i]][r2];
                  if (!continueMove.isNull()) {
                    unsigned char moveTo = continueMove.moveTo();
                    if (allPieces.isSet(moveTo)) {
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
          const Move &addingMove =
              chessCache.bishopMoves[d][queens[0].pieces[i]][r];
          if (!addingMove.isNull()) {
            blackAtks.setSquare(addingMove.moveTo());
            if (addingMove.moveTo() == whiteKing) {
              checkRay =
                  chessCache.rays1Extra[addingMove.moveFrom()][whiteKing];
              blockRay = chessCache.rays[addingMove.moveFrom()][whiteKing];
              ++checkCount;
            }
            if (allPieces.isSet(addingMove.moveTo())) {
              if (r < 6) {
                for (int r2 = r + 1; r2 < 7; ++r2) {
                  const Move &continueMove =
                      chessCache.bishopMoves[d][queens[0].pieces[i]][r2];
                  if (!continueMove.isNull()) {
                    unsigned char moveTo = continueMove.moveTo();
                    if (allPieces.isSet(moveTo)) {
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
    // if (turn == pieces.WHITE) {
    for (int i = 0; i < rooks[1].amt; ++i) {
      // for each direction
      for (int d = 0; d < 4; ++d) {
        for (int r = 0; r < 7; ++r) {
          const Move &addingMove =
              chessCache.rookMoves[d][rooks[1].pieces[i]][r];
          if (!addingMove.isNull()) {
            whiteAtks.setSquare(addingMove.moveTo());
            if (addingMove.moveTo() == blackKing) {
              checkRay =
                  chessCache.rays1Extra[addingMove.moveFrom()][blackKing];
              blockRay = chessCache.rays[addingMove.moveFrom()][blackKing];
              ++checkCount;
            }
            if (allPieces.isSet(addingMove.moveTo())) {
              // pin masks
              if (r < 6) {
                for (int r2 = r + 1; r2 < 7; ++r2) {
                  const Move &continueMove =
                      chessCache.rookMoves[d][rooks[1].pieces[i]][r2];
                  if (!continueMove.isNull()) {
                    unsigned char moveTo = continueMove.moveTo();
                    if (allPieces.isSet(moveTo)) {
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
          const Move &addingMove =
              chessCache.rookMoves[d][queens[1].pieces[i]][r];
          if (!addingMove.isNull()) {
            whiteAtks.setSquare(addingMove.moveTo());
            if (addingMove.moveTo() == blackKing) {
              checkRay =
                  chessCache.rays1Extra[addingMove.moveFrom()][blackKing];
              blockRay = chessCache.rays[addingMove.moveFrom()][blackKing];
              ++checkCount;
            }
            if (allPieces.isSet(addingMove.moveTo())) {
              // pin masks
              if (r < 6) {
                for (int r2 = r + 1; r2 < 7; ++r2) {
                  const Move &continueMove =
                      chessCache.rookMoves[d][queens[1].pieces[i]][r2];
                  if (!continueMove.isNull()) {
                    unsigned char moveTo = continueMove.moveTo();
                    if (allPieces.isSet(moveTo)) {
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
          const Move &addingMove =
              chessCache.rookMoves[d][rooks[0].pieces[i]][r];
          if (!addingMove.isNull()) {
            blackAtks.setSquare(addingMove.moveTo());
            if (addingMove.moveTo() == whiteKing) {
              checkRay =
                  chessCache.rays1Extra[addingMove.moveFrom()][whiteKing];
              blockRay = chessCache.rays[addingMove.moveFrom()][whiteKing];
              ++checkCount;
            }
            if (allPieces.isSet(addingMove.moveTo())) {
              // pin masks
              if (r < 6) {
                for (int r2 = r + 1; r2 < 7; ++r2) {
                  const Move &continueMove =
                      chessCache.rookMoves[d][rooks[0].pieces[i]][r2];
                  if (!continueMove.isNull()) {
                    unsigned char moveTo = continueMove.moveTo();
                    if (allPieces.isSet(moveTo)) {
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
          const Move &addingMove =
              chessCache.rookMoves[d][queens[0].pieces[i]][r];
          if (!addingMove.isNull()) {
            blackAtks.setSquare(addingMove.moveTo());
            if (addingMove.moveTo() == whiteKing) {
              checkRay =
                  chessCache.rays1Extra[addingMove.moveFrom()][whiteKing];
              blockRay = chessCache.rays[addingMove.moveFrom()][whiteKing];
              ++checkCount;
            }
            if (allPieces.isSet(addingMove.moveTo())) {
              // pin masks
              if (r < 6) {
                for (int r2 = r + 1; r2 < 7; ++r2) {
                  const Move &continueMove =
                      chessCache.rookMoves[d][queens[0].pieces[i]][r2];
                  if (!continueMove.isNull()) {
                    unsigned char moveTo = continueMove.moveTo();
                    if (allPieces.isSet(moveTo)) {
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
      const Move &addingMove = chessCache.kingMoves[whiteKing][j];
      if (!addingMove.isNull()) {
        whiteAtks.setSquare(addingMove.moveTo());
      }
    }
    for (int j = 0; j < 8; ++j) {
      const Move &addingMove = chessCache.kingMoves[blackKing][j];
      if (!addingMove.isNull()) {
        blackAtks.setSquare(addingMove.moveTo());
      }
    }
  };

  void generatePseudoLegals() {
    // clear pins
    pinExistInPosition = false;
    for (int d = 0; d < 8; ++d) {
      pinMasks[0][d].clearBoard();
      pinMasks[1][d].clearBoard();
    }
    // clear checks
    checkCount = 0;
    checkRay.clearBoard();
    blockRay.clearBoard();
    if (whiteInCheck()) {
      boolWhiteCheck = true;
    } else if (blackInCheck()) {
      boolBlackCheck = true;
    } else {
      boolWhiteCheck = false;
      boolBlackCheck = false;
    }
    // clear attacks
    whiteAtks.clearBoard();
    blackAtks.clearBoard();
    genPseudoPawnMoves();
    genPseudoKnightMoves();
    genPseudoBishopMoves();
    genPseudoRookMoves();
    genPseudoKingMoves();
  }

  void generateMoves(moveList &moves, bool includeQuiets) {
    if (fiftyMoveCounter > 50) {
      // return;
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
    unsigned short newCastleState = (currentGameState & 15); // 15 = 0b1111
    currentGameState = 0;

    unsigned char startSquare = m.moveFrom();
    unsigned char targetSquare = m.moveTo();

    prevMove = m;

    // moving
    if (board[startSquare] == pieces.BPAWN) {
      pawns[0].MovePiece(startSquare, targetSquare);
    } else if (board[startSquare] == pieces.WPAWN) {
      pawns[1].MovePiece(startSquare, targetSquare);
    } else if (board[startSquare] == pieces.BKNIGHT) {
      knights[0].MovePiece(startSquare, targetSquare);
    } else if (board[startSquare] == pieces.WKNIGHT) {
      knights[1].MovePiece(startSquare, targetSquare);
    } else if (board[startSquare] == pieces.WBISHOP) {
      bishops[1].MovePiece(startSquare, targetSquare);
    } else if (board[startSquare] == pieces.BBISHOP) {
      bishops[0].MovePiece(startSquare, targetSquare);
    } else if (board[startSquare] == pieces.WROOK) {
      rooks[1].MovePiece(startSquare, targetSquare);
      if (startSquare ==
          chessCache.whiteKingRook) {  // remove king side castling
        newCastleState &= whiteCastleKingsideMask;
      } else if (startSquare ==
                 chessCache.whiteQueenRook) {  // remove queen side castling
        newCastleState &= whiteCastleQueensideMask;
      };
    } else if (board[startSquare] == pieces.BROOK) {
      rooks[0].MovePiece(startSquare, targetSquare);
      if (startSquare ==
          chessCache.blackKingRook) {  // remove king side castling
        newCastleState &= blackCastleKingsideMask;
      } else if (startSquare ==
                 chessCache.blackQueenRook) {  // remove queen side castling
        newCastleState &= blackCastleQueensideMask;
      };
    } else if (board[startSquare] == pieces.WQUEEN) {
      queens[1].MovePiece(startSquare, targetSquare);
    } else if (board[startSquare] == pieces.BQUEEN) {
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
        board[chessCache.whiteKingRookCastleTo] = pieces.WROOK;
        deleteTile(chessCache.whiteKingRook);
        rooks[turnIndex].MovePiece(chessCache.whiteKingRook,
                                   chessCache.whiteKingRookCastleTo);
        allPieces.setSquare(chessCache.whiteKingRookCastleTo);

      } else if (targetSquare ==
                 chessCache.whiteQueenCastleTo) {  // white queen side castle
        board[chessCache.whiteQueenRookCastleTo] = pieces.WROOK;
        deleteTile(chessCache.whiteQueenRook);
        rooks[turnIndex].MovePiece(chessCache.whiteQueenRook,
                                   chessCache.whiteQueenRookCastleTo);
        allPieces.setSquare(chessCache.whiteQueenRookCastleTo);

      } else if (targetSquare ==
                 chessCache.blackKingCastleTo) {  // black king side castle
        board[chessCache.blackKingRookCastleTo] = pieces.BROOK;
        deleteTile(chessCache.blackKingRook);
        rooks[turnIndex].MovePiece(chessCache.blackKingRook,
                                   chessCache.blackKingRookCastleTo);
        allPieces.setSquare(chessCache.blackKingRookCastleTo);

      } else if (targetSquare ==
                 chessCache.blackQueenCastleTo) {  // black king side castle
        board[chessCache.blackQueenRookCastleTo] = pieces.BROOK;
        deleteTile(chessCache.blackQueenRook);
        rooks[turnIndex].MovePiece(chessCache.blackQueenRook,
                                   chessCache.blackQueenRookCastleTo);
        allPieces.setSquare(chessCache.blackQueenRookCastleTo);
      }
    }

    // promotion
    if ((chessCache.preComputedRows[targetSquare] == 7 ||
         chessCache.preComputedRows[targetSquare] == 0) &&
        (board[startSquare] == pieces.BPAWN ||
         board[startSquare] == pieces.WPAWN)) {
      pawns[turnIndex].removeAtTile(
          targetSquare);  // remove at target square where move piece was used
      if (m.promoteQueen()) {
        board[startSquare] = pieces.QUEEN | turn;
        queens[turnIndex].addAtTile(targetSquare);
      } else if (m.promoteRook()) {
        board[startSquare] = pieces.ROOK | turn;
        rooks[turnIndex].addAtTile(targetSquare);
      } else if (m.promoteBishop()) {
        board[startSquare] = pieces.BISHOP | turn;
        bishops[turnIndex].addAtTile(targetSquare);
      } else if (m.promoteKnight()) {
        board[startSquare] = pieces.KNIGHT | turn;
        knights[turnIndex].addAtTile(targetSquare);
      }
    };

    // en passant
    if (m.isEnPassant()) {
      if (turn == pieces.WHITE) {
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
    if (board[targetSquare] == pieces.BPAWN) {
      pawns[0].removeAtTile(targetSquare);
    } else if (board[targetSquare] == pieces.WPAWN) {
      pawns[1].removeAtTile(targetSquare);
    } else if (board[targetSquare] == pieces.BKNIGHT) {
      knights[0].removeAtTile(targetSquare);
    } else if (board[targetSquare] == pieces.WKNIGHT) {
      knights[1].removeAtTile(targetSquare);
    } else if (board[targetSquare] == pieces.WBISHOP) {
      bishops[1].removeAtTile(targetSquare);
    } else if (board[targetSquare] == pieces.BBISHOP) {
      bishops[0].removeAtTile(targetSquare);
    } else if (board[targetSquare] == pieces.WROOK) {
      rooks[1].removeAtTile(targetSquare);
      if (targetSquare ==
          chessCache.whiteKingRook) {  // remove king side castling
        newCastleState &= whiteCastleKingsideMask;
      } else if (targetSquare ==
                 chessCache.whiteQueenRook) {  // remove queen side castling
        newCastleState &= whiteCastleQueensideMask;
      };
    } else if (board[targetSquare] == pieces.BROOK) {
      rooks[0].removeAtTile(targetSquare);
      if (targetSquare ==
          chessCache.blackKingRook) {  // remove king side castling
        newCastleState &= blackCastleKingsideMask;
      } else if (targetSquare ==
                 chessCache.blackQueenRook) {  // remove queen side castling
        newCastleState &= blackCastleQueensideMask;
      };
    } else if (board[targetSquare] == pieces.WQUEEN) {
      queens[1].removeAtTile(targetSquare);
    } else if (board[targetSquare] == pieces.BQUEEN) {
      queens[0].removeAtTile(targetSquare);
    }

    allPieces.setSquare(targetSquare);
    allPieces.unSetSquare(startSquare);

    currentGameState |= newCastleState;              // castling
    currentGameState |= (board[targetSquare] << 8);  // capture

    gameStateHistory.push(currentGameState);

    ++plyCount;
    ++fiftyMoveCounter;

    if (pieces.type(board[startSquare]) == pieces.PAWN || board[targetSquare] != pieces.EMPTY || m.isEnPassant()) {
      fiftyMoveCounter = 0;
    }

    board[targetSquare] = board[startSquare];
    board[startSquare] = pieces.EMPTY;
    makeTurn();

    generatePseudoLegals();
  };

  void unMakeMove(const Move &m) {
    --plyCount;
    unsigned char startSquare = m.moveFrom();
    unsigned char targetSquare = m.moveTo();

    unsigned char capturedPiece = (currentGameState >> 8) & 63;
    prevMove.clearMove();

    // moving
    if (board[targetSquare] == pieces.BPAWN) {
      pawns[0].MovePiece(targetSquare, startSquare);
    } else if (board[targetSquare] == pieces.WPAWN) {
      pawns[1].MovePiece(targetSquare, startSquare);
    } else if (board[targetSquare] == pieces.BKNIGHT) {
      knights[0].MovePiece(targetSquare, startSquare);
    } else if (board[targetSquare] == pieces.WKNIGHT) {
      knights[1].MovePiece(targetSquare, startSquare);
    } else if (board[targetSquare] == pieces.WBISHOP) {
      bishops[1].MovePiece(targetSquare, startSquare);
    } else if (board[targetSquare] == pieces.BBISHOP) {
      bishops[0].MovePiece(targetSquare, startSquare);
    } else if (board[targetSquare] == pieces.WROOK) {
      rooks[1].MovePiece(targetSquare, startSquare);
    } else if (board[targetSquare] == pieces.BROOK) {
      rooks[0].MovePiece(targetSquare, startSquare);
    } else if (board[targetSquare] == pieces.WQUEEN) {
      queens[1].MovePiece(targetSquare, startSquare);
    } else if (board[targetSquare] == pieces.BQUEEN) {
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
        board[chessCache.whiteKingRook] = pieces.WROOK;
        rooks[oppTurnIndex].MovePiece(chessCache.whiteKingRookCastleTo,
                                      chessCache.whiteKingRook);
        deleteTile(chessCache.whiteKingRookCastleTo);
        allPieces.setSquare(chessCache.whiteKingRook);

      } else if (targetSquare ==
                 chessCache.whiteQueenCastleTo) {  // white queen side castle
        board[chessCache.whiteQueenRook] = pieces.WROOK;
        rooks[oppTurnIndex].MovePiece(chessCache.whiteQueenRookCastleTo,
                                      chessCache.whiteQueenRook);
        deleteTile(chessCache.whiteQueenRookCastleTo);
        allPieces.setSquare(chessCache.whiteQueenRook);

      } else if (targetSquare ==
                 chessCache.blackKingCastleTo) {  // black king side castle
        board[chessCache.blackKingRook] = pieces.BROOK;
        rooks[oppTurnIndex].MovePiece(chessCache.blackKingRookCastleTo,
                                      chessCache.blackKingRook);
        deleteTile(chessCache.blackKingRookCastleTo);
        allPieces.setSquare(chessCache.blackKingRook);

      } else if (targetSquare ==
                 chessCache.blackQueenCastleTo) {  // black king side castle
        board[chessCache.blackQueenRook] = pieces.BROOK;
        rooks[oppTurnIndex].MovePiece(chessCache.blackQueenRookCastleTo,
                                      chessCache.blackQueenRook);
        deleteTile(chessCache.blackQueenRookCastleTo);
        allPieces.setSquare(chessCache.blackQueenRook);
      }
    }

    // promotion
    if (chessCache.preComputedRows[targetSquare] == 7 ||
        chessCache.preComputedRows[targetSquare] == 0) {
      if (m.promoteQueen() || m.promoteRook() || m.promoteBishop() ||
          m.promoteKnight()) {
        board[targetSquare] = pieces.PAWN | oppTurn;
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
    };

    // un en passant
    if (m.isEnPassant()) {
      if (turn == pieces.WHITE) {
        int epSquare = targetSquare + 8;
        pawns[turnIndex].addAtTile(epSquare);
        allPieces.setSquare(epSquare);
        board[epSquare] = pieces.WPAWN;
      } else {
        int epSquare = targetSquare - 8;
        pawns[turnIndex].addAtTile(epSquare);
        allPieces.setSquare(epSquare);
        board[epSquare] = pieces.BPAWN;
      }
    }

    // uncaptures
    allPieces.setSquare(startSquare);
    if (capturedPiece == pieces.EMPTY) {
      allPieces.unSetSquare(targetSquare);
    } else if (capturedPiece == pieces.BPAWN) {
      pawns[0].addAtTile(targetSquare);
    } else if (capturedPiece == pieces.WPAWN) {
      pawns[1].addAtTile(targetSquare);
    } else if (capturedPiece == pieces.BKNIGHT) {
      knights[0].addAtTile(targetSquare);
    } else if (capturedPiece == pieces.WKNIGHT) {
      knights[1].addAtTile(targetSquare);
    } else if (capturedPiece == pieces.BBISHOP) {
      bishops[0].addAtTile(targetSquare);
    } else if (capturedPiece == pieces.WBISHOP) {
      bishops[1].addAtTile(targetSquare);
    } else if (capturedPiece == pieces.BROOK) {
      rooks[0].addAtTile(targetSquare);
    } else if (capturedPiece == pieces.WROOK) {
      rooks[1].addAtTile(targetSquare);
    } else if (capturedPiece == pieces.BQUEEN) {
      queens[0].addAtTile(targetSquare);
    } else if (capturedPiece == pieces.WQUEEN) {
      queens[1].addAtTile(targetSquare);
    }

    board[startSquare] = board[targetSquare];
    board[targetSquare] = capturedPiece;

    gameStateHistory.pop();  // removes current state from history
    currentGameState =gameStateHistory.peek();  // sets current state to previous state in history

    makeTurn();
    generatePseudoLegals();
  }

  int heuristicEval () {
    //srand(static_cast<unsigned int>(time(0)));
    //int eval = rand() % 41 - 20;  // -20 to 20 variaty to moves
    int eval = 0;

    // material scores
    eval += pawns[1].amt * chessCache.pawnValue;
    eval -= pawns[0].amt * chessCache.pawnValue;
    eval += knights[1].amt * chessCache.knightValue;
    eval -= knights[0].amt * chessCache.knightValue;
    eval += bishops[1].amt * chessCache.bishopValue;
    eval -= bishops[0].amt * chessCache.bishopValue;
    eval += rooks[1].amt * chessCache.rookValue;
    eval -= rooks[0].amt * chessCache.rookValue;
    eval += queens[1].amt * chessCache.queenValue;
    eval -= queens[0].amt * chessCache.queenValue;

    // guard and threats scores
    eval += whiteAtks.populationCountBAND(allPieces.get());
    eval -= blackAtks.populationCountBAND(allPieces.get());

    // mobility scores
    eval += whiteAtks.populationCount();
    eval -= blackAtks.populationCount();

    // imbalance pieces scores
    // bishop pairs
    if (bishops[1].amt >= 2) {
      eval += 30;
    }
    if (bishops[0].amt >= 2) {
      eval -= 30;
    }
    
    // piece square tables
    if ((whiteAtks.populationCount() + blackAtks.populationCount()) < 50 && plyCount > 16) {  // endgame determin
      for (int i = 0; i < pawns[0].amt; ++i) {
        eval -= chessCache.pawnEndPST[0][pawns[0].pieces[i]];
        eval += chessCache.distances[blackKing][pawns[0].pieces[i]];  // less value if far away from king
      }
      for (int i = 0; i < pawns[1].amt; ++i) {
        eval += chessCache.pawnEndPST[1][pawns[1].pieces[i]];
        eval -= chessCache.distances[whiteKing][pawns[1].pieces[i]];  // less value if far away from king
      }
      eval += chessCache.centerPST[whiteKing];
      eval -= chessCache.centerPST[blackKing];
    } else {
      for (int i = 0; i < pawns[0].amt; ++i) {
        eval -= chessCache.pawnPST[0][pawns[0].pieces[i]];
      }
      for (int i = 0; i < pawns[1].amt; ++i) {
        eval += chessCache.pawnPST[1][pawns[1].pieces[i]];
      }
      eval += chessCache.kingPST[1][whiteKing];
      eval -= chessCache.kingPST[0][blackKing];
    }

    //
    for (int i = 0; i < knights[0].amt; ++i) {
      eval -= chessCache.horsePST[0][knights[0].pieces[i]];
    }
    for (int i = 0; i < knights[1].amt; ++i) {
      eval += chessCache.horsePST[1][knights[1].pieces[i]];
    }
    //
    for (int i = 0; i < bishops[0].amt; ++i) {
      eval -= chessCache.bishopPST[0][bishops[0].pieces[i]];
    }
    for (int i = 0; i < bishops[1].amt; ++i) {
      eval += chessCache.bishopPST[1][bishops[1].pieces[i]];
    }
    //
    for (int i = 0; i < rooks[0].amt; ++i) {
      eval -= chessCache.rookPST[0][rooks[0].pieces[i]];
    }
    for (int i = 0; i < rooks[1].amt; ++i) {
      eval += chessCache.rookPST[1][rooks[1].pieces[i]];
    }
    
    return eval;
  }

  Board() {
    setupFen("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
  };

  ~Board() {
      deallocateMoveHHistory(moveHHistory);
  }

  Board(std::string fen) { setupFen(fen); };
};

struct PREFTData {
  long numPos = 0;
  // int captures = 0;
};

BitBoard cBoardHLight;
PREFTData PERFT(Board &chessBoard, int depth, int &depthCheck) {
  if (depth == 0) {  // leaf node
    PREFTData newData;
    newData.numPos = 1;
    return newData;
  }
  PREFTData newData;

  moveList genMoves;
  chessBoard.generateMoves(genMoves, true);
  chessBoard.orderMoves(genMoves);
  for (unsigned char i = 0; i < genMoves.amt; ++i) {
    //Board prevBoard = chessBoard;
    chessBoard.makeMove(genMoves.moves[i].move);

    //if (!chessBoard.isSynced()) {
    //  prevBoard.display(false, cBoardHLight);
    //  chessBoard.display(false, cBoardHLight);
    //  system("PAUSE");
    //}
    // 
    // system("CLS");
    // chessBoard.display(false, cBoardHLight);
    // system("PAUSE");

    if (depthCheck == depth) {
      std::cout << '\n'
                << chessCache.tileToNotation(genMoves.moves[i].move.moveFrom())
                << chessCache.tileToNotation(genMoves.moves[i].move.moveTo());
    }
    PREFTData branchData = PERFT(chessBoard, depth - 1, depthCheck);
    if (depthCheck == depth) {
      std::cout << ": " << branchData.numPos;
      std::cout << ", score: " << genMoves.moves[i].score;
    }
    newData.numPos += branchData.numPos;
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
    PREFTData finaldata = PERFT(chessBoard, d, finalDepth);
    auto endt = std::chrono::steady_clock::now();
    auto duration =
        std::chrono::duration_cast<std::chrono::milliseconds>(endt - start)
            .count();
    std::cout << '\n' << d << " ply, ";
    std::cout << " " << finaldata.numPos << " nodes,  ";
    std::cout << " " << duration << " ms";
  }
  auto endendt = std::chrono::steady_clock::now();
  auto totalDuraction = std::chrono::duration_cast<std::chrono::milliseconds>(
                            endendt - startstart)
                            .count();
  std::cout << "\ntotal time: " << totalDuraction << "ms\n\n";
  system("PAUSE");
};

struct mmRes {
  int nodes = 0;
  int eval = 0;
  Move best;
};

//system("CLS");
//chessBoard.display(false, cBoardHLight, "  moveCount: " + intToString(genMoves.amt));
//system("PAUSE");
int QuiescenceSearch(Board &chessBoard, int alpha, int beta) {
  moveList genMoves;
  chessBoard.generateMoves(genMoves, false);  // capture only
  if (genMoves.amt == 0) {
    //system("CLS");
    //chessBoard.display(false, cBoardHLight, "  moveCount: " + intToString(genMoves.amt));
    //system("PAUSE");
    return chessBoard.heuristicEval();
  }
  //std::cout << beta << ", " << alpha << "\n";
  chessBoard.orderMoves(genMoves);
  if (chessBoard.turn == pieces.WHITE) {
    int maxEval = chessCache.evalNegativeInf;
    for (unsigned char i = 0; i < genMoves.amt; ++i) {
      chessBoard.makeMove(genMoves.moves[i].move);
      int score = QuiescenceSearch(chessBoard, alpha, beta);
      chessBoard.unMakeMove(genMoves.moves[i].move);
      if (score > maxEval) {
        maxEval = score;
      }
      if (score >= beta) {
        break;
      }
      if (score > alpha) {
        alpha = score;
      }
    }
    return maxEval;
  } else {
    int minEval = chessCache.evalPositiveInf;
    for (unsigned char i = 0; i < genMoves.amt; ++i) {
      chessBoard.makeMove(genMoves.moves[i].move);
      int score = QuiescenceSearch(chessBoard, alpha, beta);
      chessBoard.unMakeMove(genMoves.moves[i].move);
      if (score < minEval) {
        minEval = score;
      }
      if (score <= alpha) {
        break;
      }
      if (score < beta) {
        beta = score;
      }
    }
    return minEval;
  }
}

mmRes alphaBeta(Board &chessBoard, int depth, int alpha, int beta) {
  mmRes result;
  if (depth == 0) {  // leaf
    result.nodes = 1; 
    result.eval = chessBoard.heuristicEval();
    //result.eval = QuiescenceSearch(chessBoard, alpha, beta);
    return result;
  }
  moveList genMoves;
  chessBoard.generateMoves(genMoves, true);
  if (genMoves.amt == 0) {
    if (chessBoard.whiteInCheck()) {  // white checkmated
      result.eval = chessCache.evalWhiteLoss - depth;
    } else if (chessBoard.blackInCheck()) {
      result.eval = chessCache.evalWhiteWins + depth;
    }
    result.nodes = 1;
    return result;
  }
  chessBoard.orderMoves(genMoves);
  if (chessBoard.turn == pieces.WHITE) {
    int maxEval = chessCache.evalNegativeInf;
    for (unsigned char i = 0; i < genMoves.amt; ++i) {
      chessBoard.makeMove(genMoves.moves[i].move);
      mmRes score = alphaBeta(chessBoard, depth - 1, alpha, beta);
      chessBoard.unMakeMove(genMoves.moves[i].move);
      result.nodes += score.nodes;
      if (score.eval > maxEval) {
        result.best = genMoves.moves[i].move;
        maxEval = score.eval;
      }
      if (score.eval >= beta) { // cutoff
        chessBoard.moveHHistory[1][genMoves.moves[i].move.moveFrom()][genMoves.moves[i].move.moveTo()] = (1 << depth);
        break;
      }
      if (score.eval > alpha) {
        alpha = score.eval;
      }
    }
    result.eval = maxEval;
    return result;
  } else {
    int minEval = chessCache.evalPositiveInf;
    for (unsigned char i = 0; i < genMoves.amt; ++i) {
      chessBoard.makeMove(genMoves.moves[i].move);
      mmRes score = alphaBeta(chessBoard, depth - 1, alpha, beta);
      chessBoard.unMakeMove(genMoves.moves[i].move);
      result.nodes += score.nodes;
      if (score.eval < minEval) {
        result.best = genMoves.moves[i].move;
        minEval = score.eval;
      }
      if (score.eval <= alpha) { // cutoff
        chessBoard.moveHHistory[0][genMoves.moves[i].move.moveFrom()][genMoves.moves[i].move.moveTo()] = depth*depth;
        break;
      }
      if (score.eval < beta) {
        beta = score.eval;
      }
    }
    result.eval = minEval;
    return result;
  }
}

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

void startGame(Board &chessBoard) {
  BitBoard cBoardHLight;
  Move previousMoves[400];
  std::string input = " ";
  std::string aiTxt = " ";

  bool blackSide = false;
  int chessBoardPly = 0;  // amt of chess moves
  int moveFrom = 0;
  int moveTo = 0;

  bool whiteAI = false;
  bool blackAI = false;

  do {
    if (!chessBoard.isSynced()) {
      system("PAUSE");
    }
    system("CLS");
    cBoardHLight.clearBoard();
    chessBoard.display(blackSide, cBoardHLight, aiTxt);

    if ((chessBoard.turn == pieces.BLACK && blackAI) ||
        (chessBoard.turn == pieces.WHITE && whiteAI)) {
      int beta = chessCache.evalPositiveInf;
      int alpha = chessCache.evalNegativeInf;
      int depth = 6;
      int chaosCount = (chessBoard.getWhiteAtks().populationCount() + chessBoard.getBlackAtks().populationCount());
      if (chaosCount < 50 && chessBoardPly > 4) {
        depth = 7;
        if (chaosCount < 40) {
          depth = 8;
        }
        if (chaosCount < 20) {
          depth = 9;
        }
      }
      //depth = 2;
      auto start = std::chrono::steady_clock::now();
      mmRes minMaxResult = alphaBeta(chessBoard, depth, alpha, beta);
      if (!minMaxResult.best.isNull()) {
        chessBoard.makeMove(minMaxResult.best);
        auto endt = std::chrono::steady_clock::now();
        auto duration =
            std::chrono::duration_cast<std::chrono::milliseconds>(endt - start)
                .count();
        previousMoves[chessBoardPly] = minMaxResult.best;
        ++chessBoardPly;
        aiTxt = "  EVAL: " + intToString(minMaxResult.eval) +
                ",  nodes: " + intToString(minMaxResult.nodes) +
                ",  time: " + intToString(static_cast<int>(duration)) + "ms" +
                ",  depth: " + intToString(depth);
        continue;
      } else {
        aiTxt = "[NO MOVE GENERATED] EVAL: " + intToString(minMaxResult.eval) +
                ",  nodes: " + intToString(minMaxResult.nodes) +
                ",  depth: " + intToString(depth);
      }
    }

    std::cout << "input: ";
    std::cin >> input;

    input = toLowercase(input);
    if (input == "help") {
      std::cout << "\n -> COMMANDS <-\n"
                << "\"exit\" - exit current game\n"
                << "\"undo\" - undoes a move\n"
                << "\"flip\" - flips board\n"
                << "\"wai\" - toggle white AI\n"
                << "\"bai\" - toggle black AI\n"
                << "\"f\" - enter custom fen\n"
                << "\n"
                << "\"cray\" - highlights check ray bitboard\n"
                << "\"pieces\" - highlights all pieces bitboard\n"
                << "\"watks\" - highlights all white attacks bitboard\n"
                << "\"batks\" - highlights all black attacks bitboard\n"
                << "\"pins\" - highlights all the pins\n"
                << "\"kings\"  - highlights both kings\n"
                << "\"wpawns\" - highlights all white pawns\n"
                << "\"bpawns\" - highlights all black pawns\n"
                << "\"wrooks\" - highlights all white rooks\n"
                << "\"brooks\" - highlights all black rooks\n"
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
    } else if (input == "cray") {
      BitBoard bb = chessBoard.getcray();
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
    } else if (input == "pieces") {
      BitBoard bb = chessBoard.getAllPieces();
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
    } else if (input == "wpawns") {
      BitBoard bb;
      pieceList wPawns = chessBoard.getWPawns();
      for (int i = 0; i < wPawns.amt; i++) {
        bb.setSquare(wPawns.pieces[i]);
      }
      chessBoard.display(blackSide, bb, "  amt: " + intToString(wPawns.amt));
      system("PAUSE");
    } else if (input == "bpawns") {
      BitBoard bb;
      pieceList wPawns = chessBoard.getBPawns();
      for (int i = 0; i < wPawns.amt; i++) {
        bb.setSquare(wPawns.pieces[i]);
      }
      chessBoard.display(blackSide, bb, "  amt: " + intToString(wPawns.amt));
      system("PAUSE");
    } else if (input == "wrooks") {
      BitBoard bb;
      pieceList wRooks = chessBoard.getWRooks();
      for (int i = 0; i < wRooks.amt; i++) {
        bb.setSquare(wRooks.pieces[i]);
      }
      chessBoard.display(blackSide, bb, "  amt: " + intToString(wRooks.amt));
      system("PAUSE");
    } else if (input == "brooks") {
      BitBoard bb;
      pieceList bRooks = chessBoard.getBRooks();
      for (int i = 0; i < bRooks.amt; i++) {
        bb.setSquare(bRooks.pieces[i]);
      }
      chessBoard.display(blackSide, bb, "  amt: " + intToString(bRooks.amt));
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
          if (genMoves.moves[i].move.promoteQueen() ||
              genMoves.moves[i].move.promoteRook() ||
              genMoves.moves[i].move.promoteBishop() ||
              genMoves.moves[i].move.promoteKnight()) {
            Promotes.addConstMove(genMoves.moves[i].move);
            isPromotion = true;
          }
        }
      }
      if (isPromotion) {
        std::cout << "\npromote to: \n"
                  << "[q] " << pieces.toUnicode(pieces.QUEEN) << " Queen\n"
                  << "[r] " << pieces.toUnicode(pieces.ROOK) << " Rook\n"
                  << "[b] " << pieces.toUnicode(pieces.BISHOP) << " Bishop\n"
                  << "[n] " << pieces.toUnicode(pieces.KNIGHT) << " Knight\n";
        std::cin >> input;
        input = toLowercase(input);
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

void allowEmojis() {
  // Sets Encoding to UTF8 - for chess pieces ASCII
  SetConsoleOutputCP(CP_UTF8);  // for visuals
}

int main() {
  Board chessBoard;
  std::string input = " ";
  allowEmojis();
  do {
    system("CLS");
    std::cout << " " << pieces.toUnicode(pieces.PAWN) << " CHESS MENU "
              << pieces.toUnicode(pieces.PAWN);
    setTxtColor(chessCache.greyLetCol);
    std::cout << " V5.8";  // VERSION ~1300 elo
    setTxtColor(15);
    std::cout << "\n______________________\n";
    std::cout << "\n[p] Play";
    setTxtColor(chessCache.greyLetCol);
    std::cout << " (input \"help\" for commands) ";
    setTxtColor(15);
    std::cout << "\n[t] Performance Test"
              << "\n[f] Custom Fen";
    setTxtColor(chessCache.greyLetCol);
    std::cout << " (starting position) ";
    setTxtColor(15);
    std::cout << "\n[e] Exit" << '\n';

    std::cin >> input;
    input = toLowercase(input);

    if (input == "p") {
      startGame(chessBoard);
    } else if (input == "t") {
      // perft test
      perftTest(chessBoard);
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

void setTxtColor(int colorValue) {
  HANDLE hConsole;
  hConsole = GetStdHandle(STD_OUTPUT_HANDLE);

  SetConsoleTextAttribute(hConsole, colorValue);
}

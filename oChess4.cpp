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
    - using smaller variable types like char/short instead of int to save memmory
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
 + legalize pins and checks
 + fix minimax alpha beta pruning
 ? odd gamestate push at loading fen??

=======================================*/

#include <algorithm> // for move sorting
#include <windows.h> // for console visuals
#include <chrono> // for timing
#include <iostream> //

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
  unsigned long long bitBoard = 0; // 64 bits

 public:
  void setSquare(unsigned char square) {
    bitBoard |= (1ULL << square);  // set the bit corresponding to the index square
  };

  void unSetSquare(unsigned char square) {
    bitBoard &= ~(1ULL << square);  // clear the bit corresponding to the index square
  };

  bool isSet(unsigned char square) const {
    return (bitBoard >> square) & 1;  // check if the bit corresponding to the square is set
  };

  void clearBoard() {
    bitBoard = 0;  // clear the entire bitboard
  };

  // Population count (Hamming weight) function
  int populationCount() const {
    unsigned long long x = bitBoard; // copy bitboard for manipulation
    x = x - ((x >> 1) & 0x5555555555555555ULL); // step 1: divide and conquer to sum 2 bits at a time
    x = (x & 0x3333333333333333ULL) + ((x >> 2) & 0x3333333333333333ULL); // step 2: Sum groups of 4 bits
    x = (x + (x >> 4)) & 0x0F0F0F0F0F0F0F0FULL; // step 3: sum groups of 8 bits
    x = x + (x >> 8); // step 4: Sum groups of 16 bits
    x = x + (x >> 16); // step 5: Sum groups of 32 bits
    x = x + (x >> 32); // step 6: Sum all bits in the 64-bit integer
    return x & 0x7F; // return only the least significant 7 bits (to handle overflow)
  }

  int populationCountBAND(unsigned long long x2) const {
    unsigned long long x = bitBoard & x2;  // copy bitboard for manipulation
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

  unsigned long long get() const { return bitBoard; };
  bool isEmpty() const { return bitBoard == 0; };

  BitBoard(){};
  BitBoard(unsigned long long newbb) { bitBoard = newbb; };
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
  unsigned short moveValue = 0b0000000000000000;
  unsigned char flag() const { return moveValue >> 12; };

 public:
  unsigned char moveFrom() const { return moveValue & moveFlags.fromTileMask; };
  unsigned char moveTo() const {
    return (moveValue & moveFlags.toTileMask) >> 6;
  };

  bool isNull() const { return moveValue == moveFlags.Null; };
  bool isCastling() const { return flag() == moveFlags.CastleFlag; };
  bool isPawnTwoUp() const { return flag() == moveFlags.PawnTwoUpFlag; };
  bool isEnPassant() const { return flag() == moveFlags.EnPassantCaptureFlag; };
  bool promoteQueen() const { return flag() == moveFlags.PromoteToQueenFlag; };
  bool promoteRook() const { return flag() == moveFlags.PromoteToRookFlag; };
  bool promoteBishop() const { return flag() == moveFlags.PromoteToBishopFlag; };
  bool promoteKnight() const { return flag() == moveFlags.PromoteToKnightFlag; };
  bool isDoublePawnPush() const { return flag() == moveFlags.PawnTwoUpFlag; }
  void setFlag(unsigned char newFlag) {
    // Clear the existing flag bits
    moveValue &= ~(0b1111 << 12);
    // Set the new flag bits
    moveValue |= (newFlag << 12);
  }
  void clearMove() { moveValue = moveFlags.Null; };

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

  const int centerPST[64] = {
      // center piece square table
      -40, -20, -20, -10, -10, -20, -20, -40,//
      -10, 10, 0, 0, 0, 0, 10, -10,//
      -10, 0, 0, 2, 2, 0, 0, -10,//
      -10, 0, 2, 40, 40, 2, 0, -10,//
      -10, 0, 2, 40, 40, 2, 0, -10,//
      -10, 0, 0, 2, 2, 0, 0, -10,//
      -10, 10, 0, 0, 0, 0, 10 -10,//
      -40, -20, -20, -10, -10, -20, -20, -40,//
  };

  const int earlyKingPST[64] = {
      // kings piece square table
      0, 10, 5,  0,   0,   0,   10,  5,  //
      0, 0,  -10, -40, -10, -10, -10, 0,  //
      0, 0,  -20,   -40, -40, 0,   0,   0,  //
      0, 0,  -40,   -40,   -40, -40, 0,   0,  //
      0, 0,  -40,   -40,   -40, -40, 0,   0,  //
      0, 0,  -20,   -40, -40, 0,   0,   0,  //
      0, 0,  -10, -40, -10, -10, -10, 0,  //
      0, 10, 5,  0,   0,   0,   10,  5,  //
  };


  const int bishopsPST[64] = {
      // bishops piece square table
      -20, -20, 0,  0, 0, 0,  -20,  -20,  //
      -20, 20, 0, 0, 0, 0, 20, -20,  //
      -10, 0, 20, 2, 2, 20, 0, -10,  //
      0, 0, 20, 2, 2, 20, 0, 0,  //
      0, 0, 20, 2, 2, 20, 0, 0,  //
      -10, 0, 20, 2, 2, 20, 0, -10,  //
      -20, 20, 0, 0, 0, 0, 20, -20,  //
      -20, -20, 0, 0, 0, 0, -20, -20,  //
  };

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

  // pre-compute pseudo legal moves for each tile
  Move bPawnMoves[64][8];   // 2 max possible moves 2 diagonal takes + promotions
  Move wPawnMoves[64][8];   // 2 max possible moves 2 diagonal takes + promotions
  Move knightMoves[64][8];  // 8 max possible moves
  Move kingMoves[64][8];    // 8 max possible moves

  // directions
  // [0] is
  Move bishopMoves[4][64][7];  // 13 max, 4 directions, 7 moves max in each direction
  Move rookMoves[4][64][7];  // 14 max, 4 directions, 7 moves max in each direction

  // castling
  Move whiteKingSideCastle;
  Move whiteQueenSideCastle;
  Move blackKingSideCastle;
  Move blackQueenSideCastle;

  // returns bitboard ray between from tile to tile
  BitBoard rays[64][64]; // [fromtile][totile]

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

  // queen side vacant tiles - tiles in where it has to be empty to allow castling
  const unsigned char whiteQueenCastleVacant = 1;
  const unsigned char blackQueenCastleVacant = 57;

  unsigned char notationToTile(std::string notation) const {
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

  // piece values
  const int pawnValue = 100;
  const int knightValue = 300;
  const int bishopValue = 300;
  const int rookValue = 500;
  const int queenValue = 900;

  const int evalPositiveInf = 100000;
  const int evalNegativeInf = -100000;

  const int evalWhiteWins = 10000;
  const int evalWhiteLoss = -10000;

  int value(const unsigned char &piece) const {
    unsigned char pieceType = pieces.type(piece);
    if (pieceType == pieces.PAWN) {
      return pawnValue;
    } else if (pieceType == pieces.KNIGHT) {
      return knightValue;
    } else if (pieceType == pieces.BISHOP) {
      return bishopValue;
    } else if (pieceType == pieces.ROOK) {
      return rookValue;
    } else if (pieceType == pieces.QUEEN) {
      return queenValue;
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

    for (int i = 0; i < 64; ++i) {
      int row = (i / 8);
      int col = (i % 8);
      rowColValues[row][col] = i;
      preComputedRows[i] = row;
      preComputedCols[i] = col;

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
            // cout << "i: " << i << " = destination: " << destination << ",
            // dfar: " << dfar << "\n";
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
            // cout << "i: " << i << " = destination: " << destination << ",
            // dfar: " << dfar << "\n";
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
            // cout << "i: " << i << " = destination: " << destination << ",
            // dfar: " << dfar << "\n";
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
            // cout << "i: " << i << " = destination: " << destination << ",
            // dfar: " << dfar << "\n";
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
          // cout << "i: " << i << " = destination: " << destination << ", dfar:
          // " << dfar << "\n";
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
          // cout << "i: " << i << " = destination: " << destination << ", dfar:
          // " << dfar << "\n";
++dfar;
        }
 else {
     break;
        }
      }

      dfar = 0;
      if (col > 0) {
          for (unsigned char j = 1; j < 8; ++j) {  // left
              int destination = i + j * directionOffsets[6];
              int dCol = destination % 8;
              if (inBounds(destination) && dCol >= 0) {
                  // cout << "i: " << i << " = destination: " << destination << ",
                  // dcol: " << static_cast<int>(dCol) << "\n";
                  rookMoves[2][i][dfar] = Move(i, destination);
                  ++dfar;
                  if (dCol == 0) {  // reached end of left
                      break;
                  }
              }
              else {
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
                  // cout << "i: " << i << " = destination: " << destination << ",
                  // dcol: " << static_cast<int>(dCol) << "\n";
                  rookMoves[3][i][dfar] = Move(i, destination);
                  ++dfar;
                  if (dCol == 7) {  // reached end of right
                      break;
                  }
              }
              else {
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
        bool breakOut = false;
        BitBoard &bb = rays[i][j];
        bb.setSquare(i);

        // things between
        // orthos
        if (breakOut == false) {
          for (int d = 0; d < 4; ++d) {
            for (int r = 0; r < 7; ++r) {
              const Move &addingMove = rookMoves[d][i][r];
              if (!addingMove.isNull()) {
                if (addingMove.moveTo() == j) {
                  breakOut = true;
                  for (int r2 = r; r2 >= 0; --r2) {
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
                  for (int r2 = r; r2 >= 0; --r2) {
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
        bb.setSquare(j);
      }
    }

  };
};

const PreComputedCache chessCache;

struct moveList {
  Move moves[218];  // 218 max moves in chess
  unsigned char amt = 0;

  void addConstMove(const Move &m) {
    if (!m.isNull()) {
      moves[amt] = m;
      ++amt;
    }
  }
};

class pieceList {
 private:
  int pieceMap[64] = {0};  // to keep track of where pieces are
 public:
  int pieces[10] = {0};  // 10 possible of the same piece in chess
  int amt = 0;

  void addAtTile(int tile) {
    pieces[amt] = tile;
    pieceMap[tile] = amt;
    ++amt;
  };

  void removeAtTile(int tile) {
    int pieceIndex = pieceMap[tile];
    pieces[pieceIndex] = pieces[amt - 1];
    pieceMap[pieces[pieceIndex]] = pieceIndex;
    if (amt < 0) {
      std::cout << '\n' << amt << ", invalid amt for pieceList ";
      system("PAUSE");
    }
    if (amt > 0) {
      --amt;
    }
  };

  void MovePiece(int startSquare, int targetSquare) {
    int pieceIndex = pieceMap[startSquare];
    pieces[pieceIndex] = targetSquare;
    pieceMap[targetSquare] = pieceIndex;
  };

  void clear() {
    for (int i = 0; i < 64; ++i) {
      pieceMap[i] = 0;
    };
    for (int i = 0; i < 10; ++i) {
      pieces[i] = 0;
    };
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
      //std::cout << "Stack Underflow\n";
      return 0;  // Returning 0 as error value
    }
    return gameStateHistory[top--];
  }

  unsigned short peek() const {
    if (isEmpty()) {
      //std::cout << "Stack is empty\n";
      return 0;  // Returning 0 as error value
    }
    return gameStateHistory[top];
  }
};

class Board {
 private:
  // consts
  const unsigned short whiteCastleKingsideMask  = 0b1111111111111110;
  const unsigned short whiteCastleQueensideMask = 0b1111111111111101;
  const unsigned short blackCastleKingsideMask  = 0b1111111111111011;
  const unsigned short blackCastleQueensideMask = 0b1111111111110111;

  const unsigned short whiteCastleKingsideBit  = 0b0000000000000001;
  const unsigned short whiteCastleQueensideBit = 0b0000000000000010;
  const unsigned short blackCastleKingsideBit  = 0b0000000000000100;
  const unsigned short blackCastleQueensideBit = 0b0000000000001000;

  const unsigned short whiteCastleMask = whiteCastleKingsideMask & whiteCastleQueensideMask;
  const unsigned short blackCastleMask = blackCastleKingsideMask & blackCastleQueensideMask;

  // prev moves, only used for display
  Move prevMove;

  // index position of the kings
  unsigned char whiteKing = 0;
  unsigned char blackKing = 0;

  // Total plies (half-moves) played in game
  int plyCount = 0;
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
    currentGameState = 0;
    for (int i = 0; i < 400; ++i) {
      gameStateHistory.pop(); // clear history
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
  unsigned int checkCount = 0;
 public:
  int turn = pieces.WHITE;

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
  bool isSynced() { // returns if board matches bitboards and lists
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
      } else if (board[i] != pieces.EMPTY && !allPieces.isSet(i)) { // unset but not empty??
        std::cout << "\nunsync allPieces2[" << i << "]\n";
        return false;
      }
    }
    for (int i = 0; i < rooks[0].amt; ++i) {
      if (board[rooks[0].pieces[i]] != pieces.BROOK) {
        std::cout << "\nunsync list, brook[" << intToString(rooks[0].pieces[i]) << "]\n";
        return false;
      }
    }
    for (int i = 0; i < rooks[1].amt; ++i) {
      if (board[rooks[1].pieces[i]] != pieces.WROOK) {
        std::cout << "\nunsync list, wrook[" << intToString(rooks[1].pieces[i]) << "]\n";
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
    unsigned int scores[218] = {0};
    for (int i = 0; i < moves.amt; ++i) {
      const Move &cMove = moves.moves[i];
      const unsigned char &moveTo = cMove.moveTo();
      const unsigned int &from = board[cMove.moveFrom()];
      const unsigned int &captured = board[moveTo];
      const int &myValue = chessCache.value(from);

      scores[i] = 2;
      if (turn == pieces.WHITE) {
        if (blackAtks.isSet(moveTo)) {
          scores[i] = 1;
          if (captured != pieces.EMPTY) {
            if (myValue > chessCache.value(captured)) { // my high vlaue captures guarded low value
              scores[i] = 0;
            }
          }
        } else {
          const int &captuedValue = chessCache.value(captured);
          if (myValue <= captuedValue) { // my low/equal value captures unprotected piece
            scores[i] += captuedValue - myValue;
          }
        }
      } else {
        if (whiteAtks.isSet(moveTo)) {
          scores[i] = 1;
          if (captured != pieces.EMPTY) {
            if (myValue > chessCache.value(captured)) { // my high vlaue captures guarded low value
              scores[i] = 0;
            }
          } else {
            const int &captuedValue = chessCache.value(captured);
            if (myValue <= captuedValue) { // my low/equal value captures unprotected piece
              scores[i] += captuedValue - myValue;
            }
          }
        }
      }
    }
    
    // Use std::sort to sort moves based on scores
    std::sort(moves.moves, moves.moves + moves.amt, [&](const Move &a, const Move &b) {
        return scores[&a - &moves.moves[0]] < scores[&b - &moves.moves[0]];
    });
  }

  void setupFen(std::string fullFen) {
    resetValues();
    std::string unflippedFen = "";
    std::string fenTurn = "";
    std::string castlingFen = "";
    std::string enPassantTargetSQR = "";
    std::string halfMoveClockFen = "";
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
      int file = chessCache.preComputedCols[enPassSquare] + 1; // +1 cuz 0 means none
      currentGameState |= (file << 4);
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
               std::string line4 = " ") const {
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


        if (!prevMove.isNull() && (prevMove.moveFrom() == i2 || prevMove.moveTo() == i2)) {
          if (pieces.color(board[prevMove.moveTo()]) == pieces.WHITE) {
            setTxtColor(chessCache.prevMWhiteCol);
          } else {
            setTxtColor(chessCache.prevMBlackCol);
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
    //return (board[m.moveTo()] != pieces.EMPTY);
  };

  void addLegal(moveList &ML, const Move &m) const {
    // if empty capture or captures enemy
    const unsigned int &mto = m.moveTo();
    if (!checkRay.isEmpty()) {
      if (!checkRay.isSet(mto)) {
        return;
      }
    }
    if (board[mto] == pieces.EMPTY || turn != pieces.color(board[mto])) {
      if (mto != whiteKing && mto != blackKing) {  // and not king captures
        ML.addConstMove(m);
      }
    }
  };

  void addPLegal(moveList &ML, const Move &m) const {
    // if empty capture or captures enemy
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
          if (!allPieces.isSet(chessCache.wPawnMoves[pieceIndex][1].moveTo())) {
            addLegal(m, chessCache.wPawnMoves[pieceIndex][1]);  // Double Push
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
                     chessCache.preComputedRows[addingMove.moveFrom()] == 4 // en passant row
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
                     chessCache.preComputedRows[addingMove2.moveFrom()] == 4 // en passant row
              ) {
            addingMove2.setFlag(moveFlags.EnPassantCaptureFlag);
            m.addConstMove(addingMove2);  // en passant capture
          }
        }
      }
    } else { // if black turn
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
                     chessCache.preComputedRows[addingMove.moveFrom()] == 3
              ) {
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
                         enPFile2 - 1&&
                     chessCache.preComputedRows[addingMove2.moveFrom()] == 3
              ) {
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
          const Move &addingMove =
              chessCache.knightMoves[knights[1].pieces[i]][j];
          addLegal(m, addingMove);
        }
      }
    } else {
      for (int i = 0; i < knights[0].amt; ++i) {
        for (int j = 0; j < 8; ++j) {
          const Move &addingMove =
              chessCache.knightMoves[knights[0].pieces[i]][j];
          addLegal(m, addingMove);
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
            const Move& addingMove = chessCache.rookMoves[d][rooks[1].pieces[i]][r];
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
            const Move& addingMove = chessCache.rookMoves[d][queens[1].pieces[i]][r];
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
            const Move& addingMove = chessCache.rookMoves[d][rooks[0].pieces[i]][r];
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
            const Move& addingMove = chessCache.rookMoves[d][queens[0].pieces[i]][r];
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
        if (!blackAtks.isSet(addingMove.moveTo())) {
          addPLegal(m, addingMove);
        }
      }
      if (!whiteInCheck()) { // if not in check allow castling
        if (whiteKingSideCastle()) {
          if (board[chessCache.whiteKingCastleTo] == pieces.EMPTY &&
              board[chessCache.whiteKingRookCastleTo] ==
                  pieces.EMPTY) {  // and not attacked
            m.addConstMove(chessCache.whiteKingSideCastle);
          }
        }
        if (whiteQueenSideCastle()) {
          if (board[chessCache.whiteQueenCastleTo] == pieces.EMPTY &&
              board[chessCache.whiteQueenRookCastleTo] == pieces.EMPTY &&
              board[chessCache.whiteQueenCastleVacant] ==
                  pieces.EMPTY) {  // and not attacked
            m.addConstMove(chessCache.whiteQueenSideCastle);
          }
        }
      }
    } else {
      for (int j = 0; j < 8; ++j) {
        const Move &addingMove = chessCache.kingMoves[blackKing][j];
        if (!whiteAtks.isSet(addingMove.moveTo())) {
          addPLegal(m, addingMove);
        }
      }
      if (!blackInCheck()) { // if not in check allow castling
        if (blackKingSideCastle()) {
          if (board[chessCache.blackKingCastleTo] == pieces.EMPTY &&
              board[chessCache.blackKingRookCastleTo] ==
                  pieces.EMPTY) {  // and not attacked
            m.addConstMove(chessCache.blackKingSideCastle);
          }
        }
        if (blackQueenSideCastle()) {
          if (board[chessCache.blackQueenCastleTo] == pieces.EMPTY &&
              board[chessCache.blackQueenRookCastleTo] == pieces.EMPTY &&
              board[chessCache.blackQueenCastleVacant] ==
                  pieces.EMPTY) {  // and not attacked
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
      const Move& addingMove = chessCache.wPawnMoves[pieceIndex][2];
      if (!addingMove.isNull()) {
        whiteAtks.setSquare(addingMove.moveTo());
      }
      const Move &addingMove2 = chessCache.wPawnMoves[pieceIndex][3];
      if (!addingMove2.isNull()) {
        whiteAtks.setSquare(addingMove2.moveTo());
      }
    }
    for (int i = 0; i < pawns[0].amt; ++i) {
      int pieceIndex = pawns[0].pieces[i];
      const Move &addingMove = chessCache.bPawnMoves[pieceIndex][2];
      if (!addingMove.isNull()) {
        blackAtks.setSquare(addingMove.moveTo());
      }
      const Move &addingMove2 = chessCache.bPawnMoves[pieceIndex][3];
      if (!addingMove2.isNull()) {
        blackAtks.setSquare(addingMove2.moveTo());
      }
    }
  };

  // manually written/hard coded for less looping, so its a bit faster
  void genPseudoKnightMoves() {
    //if (turn == pieces.WHITE) {
      for (int i = 0; i < knights[1].amt; ++i) {
        for (int j = 0; j < 8; ++j) {
          const Move& addingMove = chessCache.knightMoves[knights[1].pieces[i]][j];
          if (!addingMove.isNull()) {
            whiteAtks.setSquare(addingMove.moveTo());
          }
        }
      }
    //} else {
      for (int i = 0; i < knights[0].amt; ++i) {
        for (int j = 0; j < 8; ++j) {
          const Move& addingMove = chessCache.knightMoves[knights[0].pieces[i]][j];
          if (!addingMove.isNull()) {
            blackAtks.setSquare(addingMove.moveTo());
          }
        }
      }
    //}
  };

  // manually written/hard coded for less looping, so its a bit faster
  void genPseudoBishopMoves() {
    //if (turn == pieces.WHITE) {
      for (int i = 0; i < bishops[1].amt; ++i) {
        // for each direction
        for (int d = 0; d < 4; ++d) {
          for (int r = 0; r < 7; ++r) {
            const Move& addingMove = chessCache.bishopMoves[d][bishops[1].pieces[i]][r];
            if (!addingMove.isNull()) {
              whiteAtks.setSquare(addingMove.moveTo());
              if (addingMove.moveTo() == blackKing) {
                checkRay = chessCache.rays[addingMove.moveFrom()][addingMove.moveTo()];
                ++checkCount;
              }
              if (allPieces.isSet(addingMove.moveTo())) {
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
            const Move& addingMove = chessCache.bishopMoves[d][queens[1].pieces[i]][r];
            if (!addingMove.isNull()) {
              whiteAtks.setSquare(addingMove.moveTo());
              if (addingMove.moveTo() == blackKing) {
                checkRay = chessCache.rays[addingMove.moveFrom()][addingMove.moveTo()];
                ++checkCount;
              }
              if (allPieces.isSet(addingMove.moveTo())) {
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
            const Move& addingMove =
                chessCache.bishopMoves[d][bishops[0].pieces[i]][r];
            if (!addingMove.isNull()) {
              blackAtks.setSquare(addingMove.moveTo());
              if (addingMove.moveTo() == whiteKing) {
                checkRay = chessCache.rays[addingMove.moveFrom()][addingMove.moveTo()];
                ++checkCount;
              }
              if (allPieces.isSet(addingMove.moveTo())) {
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
            const Move& addingMove = chessCache.bishopMoves[d][queens[0].pieces[i]][r];
            if (!addingMove.isNull()) {
              blackAtks.setSquare(addingMove.moveTo());
              if (addingMove.moveTo() == whiteKing) {
                checkRay = chessCache.rays[addingMove.moveFrom()][addingMove.moveTo()];
                ++checkCount;
              }
              if (allPieces.isSet(addingMove.moveTo())) {
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
    //if (turn == pieces.WHITE) {
      for (int i = 0; i < rooks[1].amt; ++i) {
        // for each direction
        for (int d = 0; d < 4; ++d) {
          for (int r = 0; r < 7; ++r) {
            const Move& addingMove = chessCache.rookMoves[d][rooks[1].pieces[i]][r];
            if (!addingMove.isNull()) {
              whiteAtks.setSquare(addingMove.moveTo());
              if (addingMove.moveTo() == blackKing) {
                checkRay = chessCache.rays[addingMove.moveFrom()][addingMove.moveTo()];
                ++checkCount;
              }
              if (allPieces.isSet(addingMove.moveTo())) {
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
            const Move& addingMove = chessCache.rookMoves[d][queens[1].pieces[i]][r];
            if (!addingMove.isNull()) {
              whiteAtks.setSquare(addingMove.moveTo());
              if (addingMove.moveTo() == blackKing) {
                checkRay = chessCache.rays[addingMove.moveFrom()][addingMove.moveTo()];
                ++checkCount;
              }
              if (allPieces.isSet(addingMove.moveTo())) {
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
            const Move& addingMove = chessCache.rookMoves[d][rooks[0].pieces[i]][r];
            if (!addingMove.isNull()) {
              blackAtks.setSquare(addingMove.moveTo());
              if (addingMove.moveTo() == whiteKing) {
                checkRay = chessCache.rays[addingMove.moveFrom()][addingMove.moveTo()];
                ++checkCount;
              }
              if (allPieces.isSet(addingMove.moveTo())) {
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
            const Move& addingMove = chessCache.rookMoves[d][queens[0].pieces[i]][r];
            if (!addingMove.isNull()) {
              blackAtks.setSquare(addingMove.moveTo());
              if (addingMove.moveTo() == whiteKing) {
                checkRay = chessCache.rays[addingMove.moveFrom()][addingMove.moveTo()];
                ++checkCount;
              }
              if (allPieces.isSet(addingMove.moveTo())) {
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
      const Move& addingMove = chessCache.kingMoves[whiteKing][j];
      if (!addingMove.isNull()) {
        whiteAtks.setSquare(addingMove.moveTo());
      }
    }
    for (int j = 0; j < 8; ++j) {
      const Move& addingMove = chessCache.kingMoves[blackKing][j];
      if (!addingMove.isNull()) {
        blackAtks.setSquare(addingMove.moveTo());
      }
    }
  };

  void generatePseudoLegals() {
    checkCount = 0;
    checkRay.clearBoard();
    if (whiteInCheck()) {
      boolWhiteCheck = true;
    } else  if (blackInCheck()) {
      boolBlackCheck = true;
    } else {
      boolWhiteCheck = false;
      boolBlackCheck = false;
    }
    whiteAtks.clearBoard();
    blackAtks.clearBoard();
    genPseudoPawnMoves();
    genPseudoKnightMoves();
    genPseudoBishopMoves();
    genPseudoRookMoves();
    genPseudoKingMoves();
  }
  void generateMoves(moveList &moves) const {
    if (checkCount < 2) {
      generatePawnMoves(moves);
      generateKnightMoves(moves);
      generateBishopMoves(moves);
      generateRookMoves(moves);
    }
    generateKingMoves(moves);
  };

  void makeMove(Move &m) {
    unsigned short originalCastleState = currentGameState & 15; // 15 = 0b1111
    unsigned short newCastleState = originalCastleState;
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
      if (startSquare == chessCache.whiteKingRook) { // remove king side castling
        newCastleState &= whiteCastleKingsideMask;
      } else if (startSquare == chessCache.whiteQueenRook) { // remove queen side castling
        newCastleState &= whiteCastleQueensideMask;
      };
    } else if (board[startSquare] == pieces.BROOK) {
      rooks[0].MovePiece(startSquare, targetSquare);
      if (startSquare == chessCache.blackKingRook) { // remove king side castling
        newCastleState &= blackCastleKingsideMask;
      } else if (startSquare == chessCache.blackQueenRook) {  // remove queen side castling
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
      int file = chessCache.preComputedCols[startSquare] + 1; // +1 cuz 0 means none
      currentGameState |= (file << 4);
    }

    // castling
    if (m.isCastling()) {
      if (targetSquare == chessCache.whiteKingCastleTo) { // white king side castle
        board[chessCache.whiteKingRookCastleTo] = pieces.WROOK;
        deleteTile(chessCache.whiteKingRook);
        rooks[turnIndex].MovePiece(chessCache.whiteKingRook, chessCache.whiteKingRookCastleTo);
        allPieces.setSquare(chessCache.whiteKingRookCastleTo);

      } else if (targetSquare == chessCache.whiteQueenCastleTo) { // white queen side castle
        board[chessCache.whiteQueenRookCastleTo] = pieces.WROOK;
        deleteTile(chessCache.whiteQueenRook);
        rooks[turnIndex].MovePiece(chessCache.whiteQueenRook, chessCache.whiteQueenRookCastleTo);
        allPieces.setSquare(chessCache.whiteQueenRookCastleTo);

      } else if (targetSquare == chessCache.blackKingCastleTo) { // black king side castle
        board[chessCache.blackKingRookCastleTo] = pieces.BROOK;
        deleteTile(chessCache.blackKingRook);
        rooks[turnIndex].MovePiece(chessCache.blackKingRook, chessCache.blackKingRookCastleTo);
        allPieces.setSquare(chessCache.blackKingRookCastleTo);

      } else if (targetSquare == chessCache.blackQueenCastleTo) { // black king side castle
        board[chessCache.blackQueenRookCastleTo] = pieces.BROOK;
        deleteTile(chessCache.blackQueenRook);
        rooks[turnIndex].MovePiece(chessCache.blackQueenRook, chessCache.blackQueenRookCastleTo);
        allPieces.setSquare(chessCache.blackQueenRookCastleTo);

      }
    }

    // promotion
    if (
       (
            chessCache.preComputedRows[targetSquare] == 7 ||
            chessCache.preComputedRows[targetSquare] == 0
           ) &&
        (board[startSquare] == pieces.BPAWN || board[startSquare] == pieces.WPAWN)
        ) {
      pawns[turnIndex].removeAtTile(targetSquare); // remove at target square where move piece was used
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
      if (targetSquare == chessCache.whiteKingRook) { // remove king side castling
        newCastleState &= whiteCastleKingsideMask;
      } else if (targetSquare == chessCache.whiteQueenRook) { // remove queen side castling
        newCastleState &= whiteCastleQueensideMask;
      };
    } else if (board[targetSquare] == pieces.BROOK) {
      rooks[0].removeAtTile(targetSquare);
      if (targetSquare == chessCache.blackKingRook) { // remove king side castling
        newCastleState &= blackCastleKingsideMask;
      } else if (targetSquare == chessCache.blackQueenRook) {  // remove queen side castling
        newCastleState &= blackCastleQueensideMask;
      };
    } else if (board[targetSquare] == pieces.WQUEEN) {
      queens[1].removeAtTile(targetSquare);
    } else if (board[targetSquare] == pieces.BQUEEN) {
      queens[0].removeAtTile(targetSquare);
    }

    allPieces.setSquare(targetSquare);
    allPieces.unSetSquare(startSquare);

    currentGameState |= newCastleState; // castling
    currentGameState |= (board[targetSquare] << 8); // capture
    gameStateHistory.push(currentGameState);

    board[targetSquare] = board[startSquare];
    board[startSquare] = pieces.EMPTY;
    makeTurn();

    ++plyCount;
    generatePseudoLegals();
  };

  void unMakeMove(Move &m) {
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
      if (targetSquare == chessCache.whiteKingCastleTo) { // white king side castle
        board[chessCache.whiteKingRook] = pieces.WROOK;
        rooks[oppTurnIndex].MovePiece(chessCache.whiteKingRookCastleTo, chessCache.whiteKingRook);
        deleteTile(chessCache.whiteKingRookCastleTo);
        allPieces.setSquare(chessCache.whiteKingRook);

      } else if (targetSquare == chessCache.whiteQueenCastleTo) { // white queen side castle
        board[chessCache.whiteQueenRook] = pieces.WROOK;
        rooks[oppTurnIndex].MovePiece(chessCache.whiteQueenRookCastleTo, chessCache.whiteQueenRook);
        deleteTile(chessCache.whiteQueenRookCastleTo);
        allPieces.setSquare(chessCache.whiteQueenRook);

      } else if (targetSquare == chessCache.blackKingCastleTo) { // black king side castle
        board[chessCache.blackKingRook] = pieces.BROOK;
        rooks[oppTurnIndex].MovePiece(chessCache.blackKingRookCastleTo, chessCache.blackKingRook);
        deleteTile(chessCache.blackKingRookCastleTo);
        allPieces.setSquare(chessCache.blackKingRook);

      } else if (targetSquare == chessCache.blackQueenCastleTo) { // black king side castle
        board[chessCache.blackQueenRook] = pieces.BROOK;
        rooks[oppTurnIndex].MovePiece(chessCache.blackQueenRookCastleTo, chessCache.blackQueenRook);
        deleteTile(chessCache.blackQueenRookCastleTo);
        allPieces.setSquare(chessCache.blackQueenRook);

      }
    }

    // promotion
    if (chessCache.preComputedRows[targetSquare] == 7 ||
        chessCache.preComputedRows[targetSquare] == 0) {
      if (m.promoteQueen() || m.promoteRook() || m.promoteBishop() || m.promoteKnight()) {
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

    gameStateHistory.pop(); // removes current state from history
    currentGameState = gameStateHistory.peek(); // sets current state to previous state in history

    makeTurn();
    generatePseudoLegals();
  }

  int heuristicEval() {
    int eval = 0;

    // guard and threats scores
    eval += whiteAtks.populationCountBAND(allPieces.get());
    eval -= blackAtks.populationCountBAND(allPieces.get());

    // mobility scores
    eval += whiteAtks.populationCount();
    eval -= blackAtks.populationCount();

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

    int whiteEndGameEval = knights[1].amt + bishops[1].amt + queens[1].amt + rooks[1].amt;
    int blackEndGameEval = knights[0].amt + bishops[0].amt + queens[0].amt + rooks[0].amt;

    // endgames
    if (whiteEndGameEval > 3) {
        // control center
        for (int i = 0; i < knights[0].amt; ++i) {
          eval -= chessCache.centerPST[knights[0].pieces[i]];
        }
        for (int i = 0; i < bishops[0].amt; ++i) {
          eval -= chessCache.bishopsPST[bishops[0].pieces[i]];
        }
        for (int i = 0; i < pawns[0].amt; ++i) {
          eval -= chessCache.centerPST[pawns[0].pieces[i]];
        }
        eval -= chessCache.earlyKingPST[blackKing];
        eval += chessCache.earlyKingPST[whiteKing];
    } else {
        // push pawns to end
        for (int i = 0; i < pawns[0].amt; ++i) {
          eval -= (7 - chessCache.preComputedRows[pawns[0].pieces[i]]);
        }
    }
    if (blackEndGameEval > 3) {
        // control center
        for (int i = 0; i < knights[1].amt; ++i) {
          eval += chessCache.centerPST[knights[1].pieces[i]];
        }
        for (int i = 0; i < bishops[1].amt; ++i) {
          eval += chessCache.bishopsPST[bishops[1].pieces[i]];
        }
        for (int i = 0; i < pawns[1].amt; ++i) {
          eval += chessCache.centerPST[pawns[1].pieces[i]];
        }
        eval -= chessCache.earlyKingPST[blackKing];
        eval += chessCache.earlyKingPST[whiteKing];
    } else {
        // push pawns to end
        for (int i = 0; i < pawns[1].amt; ++i) {
          eval += chessCache.preComputedRows[pawns[1].pieces[i]];
        }
    }

    // score threats, defends king better
    if (whiteInCheck()) {
      eval -= chessCache.pawnValue;
    }
    if (blackInCheck()) {
      eval += chessCache.pawnValue;
    }

    // imbalance pieces scores
    // bishop pairs
    if (bishops[1].amt >= 2) {
        eval += 40;
    }
    if (bishops[0].amt >= 2) {
        eval -= 40;
    }

    return eval;
  }

  Board() {
    setupFen("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
  };

  Board(std::string fen) {
      setupFen(fen);
  };
};

struct PREFTData {
  long numPos = 0;
 // int captures = 0;
};

BitBoard cBoardHLight;
PREFTData PERFT(Board &chessBoard, int depth) {
  if (depth == 0) {
    PREFTData newData;
    newData.numPos = 1;
    return newData;
  }
  PREFTData newData;

  moveList genMoves;
  chessBoard.generateMoves(genMoves);
  for (unsigned char i = 0; i < genMoves.amt; ++i) {
    Board prevBoard = chessBoard;
    chessBoard.makeMove(genMoves.moves[i]);

    if (!chessBoard.isSynced()) {
      prevBoard.display(false, cBoardHLight);
      chessBoard.display(false, cBoardHLight);
      system("PAUSE");
    }
    //system("CLS");
    //chessBoard.display(false, cBoardHLight);
    //system("PAUSE");

    PREFTData branchData = PERFT(chessBoard, depth - 1);
    newData.numPos += branchData.numPos;
    chessBoard.unMakeMove(genMoves.moves[i]);
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
    PREFTData finaldata = PERFT(chessBoard, d);
    auto endt = std::chrono::steady_clock::now();
    auto duration =
        std::chrono::duration_cast<std::chrono::milliseconds>(endt - start).count();
    std::cout << '\n' << d << " ply, ";
    std::cout << " " << finaldata.numPos << " nodes,  ";
    std::cout << " " << duration << " ms";
  }
  auto endendt = std::chrono::steady_clock::now();
  auto totalDuraction = std::chrono::duration_cast<std::chrono::milliseconds>(endendt - startstart).count();
  std::cout << "\ntotal time: " << totalDuraction << "ms\n\n";
  system("PAUSE");
};

struct mmRes {
  int nodes = 0;
  int eval = 0;
  Move best;
};

mmRes miniMax(Board &chessBoard, int depth, int alpha, int beta) {
  mmRes result;
  moveList genMoves;
  chessBoard.generateMoves(genMoves);
  chessBoard.orderMoves(genMoves);
  if (genMoves.amt == 0) {
    if (chessBoard.whiteInCheck()) {  // white checkmated
      result.eval = chessCache.evalWhiteLoss - depth;
    } else if (chessBoard.blackInCheck()) {  // black checkmated
      result.eval = chessCache.evalWhiteWins + depth;
    }
    result.nodes = 1;
    return result;
  }
  if (depth == 0) {
    result.nodes = 1;
    result.eval = chessBoard.heuristicEval();
    return result;
  }
  if (chessBoard.turn == pieces.WHITE) {
    int maxEval = chessCache.evalNegativeInf;
    for (unsigned char i = 0; i < genMoves.amt; ++i) {
      Board prevBoard = chessBoard;
      chessBoard.makeMove(genMoves.moves[i]);
      mmRes newResults = miniMax(chessBoard, depth - 1, alpha, beta);
      chessBoard.unMakeMove(genMoves.moves[i]);
      result.nodes += newResults.nodes;
      if (newResults.eval > maxEval) {
        result.best = genMoves.moves[i];
        maxEval = newResults.eval;
      }
      if (newResults.eval >= beta) {
        break;
      }
      if (newResults.eval > alpha) {
        alpha = newResults.eval;
      }
    }
    result.eval = maxEval;
    return result;
  } else {
    int minEval = chessCache.evalPositiveInf;
    for (unsigned char i = 0; i < genMoves.amt; ++i) {
      Board prevBoard = chessBoard;
      chessBoard.makeMove(genMoves.moves[i]);
      mmRes newResults = miniMax(chessBoard, depth - 1, alpha, beta);
      chessBoard.unMakeMove(genMoves.moves[i]);
      result.nodes += newResults.nodes;
      if (newResults.eval < minEval) {
        result.best = genMoves.moves[i];
        minEval = newResults.eval;
      }
      if (newResults.eval <= alpha) {
        break;
      }
      if (newResults.eval < beta) {
        beta = newResults.eval;
      }
    }
    result.eval = minEval;
    return result;
  }
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

    if ((chessBoard.turn == pieces.BLACK && blackAI) || (chessBoard.turn == pieces.WHITE && whiteAI)) {
        int beta = chessCache.evalPositiveInf;
        int alpha = chessCache.evalNegativeInf;
        int depth = 4;
        if ((chessBoard.getBPawns().amt + chessBoard.getBPawns().amt) < 5) {
           depth = 5;
        }
        auto start = std::chrono::steady_clock::now();
        mmRes minMaxResult = miniMax(chessBoard, depth, alpha, beta);
        if (!minMaxResult.best.isNull()) {
          chessBoard.makeMove(minMaxResult.best);
          auto endt = std::chrono::steady_clock::now();
          auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(endt - start).count();
          previousMoves[chessBoardPly] = minMaxResult.best;
          ++chessBoardPly;
          aiTxt = "  EVAL: " + intToString(minMaxResult.eval) +
                  ",  nodes: " + intToString(minMaxResult.nodes) +
                  ",  time: " + intToString(static_cast<int>(duration)) + "ms" +
                  ",  depth: " + intToString(depth);
          continue;
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
                << "\n"
                << "\"cray\" - highlights check ray bitboard\n"
                << "\"pieces\" - highlights all pieces bitboard\n"
                << "\"watks\" - highlights all white attacks bitboard\n"
                << "\"batks\" - highlights all black attacks bitboard\n"
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
    } else if (input == "flip") {
      blackSide = !blackSide;
    } else if (input == "wai") {
      whiteAI = !whiteAI;
    } else if (input == "bai") {
      blackAI = !blackAI;
    } else if (input == "cray") {
      BitBoard bb = chessBoard.getcray();
      chessBoard.display(blackSide, bb,
                         "  popCount: " + intToString(bb.populationCount()));
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
      chessBoard.generateMoves(genMoves);
      for (int i = 0; i < genMoves.amt; ++i) {
        if (genMoves.moves[i].moveFrom() == moveFrom) {
          cBoardHLight.setSquare(genMoves.moves[i].moveTo());
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
        if (genMoves.moves[i].moveFrom() == moveFrom && genMoves.moves[i].moveTo() == moveTo) {
          if (genMoves.moves[i].promoteQueen() ||
              genMoves.moves[i].promoteRook() ||
              genMoves.moves[i].promoteBishop() ||
              genMoves.moves[i].promoteKnight()) {
            Promotes.addConstMove(genMoves.moves[i]);
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
          chessBoard.makeMove(Promotes.moves[1]);
          previousMoves[chessBoardPly] = Promotes.moves[1];
        } else if (input == "b") {
          chessBoard.makeMove(Promotes.moves[2]);
          previousMoves[chessBoardPly] = Promotes.moves[2];
        } else if (input == "n") {
          chessBoard.makeMove(Promotes.moves[3]);
          previousMoves[chessBoardPly] = Promotes.moves[3];
        } else {  // queen
          chessBoard.makeMove(Promotes.moves[0]);
          previousMoves[chessBoardPly] = Promotes.moves[0];
        }
        ++chessBoardPly;
      } else {
        for (int i = 0; i < genMoves.amt; ++i) {
          if (genMoves.moves[i].moveFrom() == moveFrom && genMoves.moves[i].moveTo() == moveTo) {
            chessBoard.makeMove(genMoves.moves[i]);
            previousMoves[chessBoardPly] = genMoves.moves[i];
            ++chessBoardPly;
            break;
          }
        }
      }
    }
  } while (true);
}

std::string getFen() {
  std::cin.ignore();
  std::cout << "Enter FEN: ";
  std::string line;
  char ch;
  while ((ch = std::cin.get()) != '\n') {
    line += ch;
  }
  return line;
}

void allowEmojis() {
  // Sets Encoding to UTF8 - for chess pieces ASCII
  SetConsoleOutputCP(CP_UTF8);  // for visuals
}

int main() {
  Board chessBoard;
  std::string input = " ";

  //chessBoard.setupFen("8/6P1/8/b2k1N1R/1p2b3/n7/2P4P/R3K2R w KQ - 0 1"); // Final Boss Fen
  //chessBoard.setupFen("7r/5PPP/4PKP1/8/8/1pkp4/ppp5/R7 w - - 0 1"); // simple promotion tests
  //chessBoard.setupFen("8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - -"); // ultimate enpassant pin test
  //chessBoard.setupFen("8/2k5/8/4r3/8/2p5/7B/1KR5 b - - 0 1"); // pinned piece blocks pin edge case
  //chessBoard.setupFen("8/8/6b1/1k6/4R3/8/2KP2r1/8 w - - 0 1"); // pinned piece blocks pin edge case2
  //chessBoard.setupFen("r3k2r/p2p1p1p/6b1/3Bn3/3bN3/6B1/P2P1P1P/R3K2R w KQkq - 0 1"); // castling tests
  //chessBoard.setupFen("8/2p5/7r/KP5r/7r/7k/8/8 b - - 0 1"); // en passant leads to checkmate
  //chessBoard.setupFen("8/6bb/8/8/R1p3k1/4P3/P2P4/K7 w - - 0 1"); // pinned en passant legality tests
  //chessBoard.setupFen("8/1p1pkp2/2p1p3/2P1P1P1/2p1p1p1/2P1P3/1P1PKP2/8 w - - 0 1"); // en passant tests
  //chessBoard.setupFen("r3k2r/8/8/8/8/8/8/R3K2R w KQkq - 0 1"); // simple castling tests
  //chessBoard.setupFen("4k3/1p4pp/2p5/8/q3r2Q/3p3P/1P4PK/4R3 b - -"); // cross pin test
  //chessBoard.setupFen("4r3/1b6/4R3/3R4/kr1RK3/1p1R1N2/8/3B3q w - - 0 1"); // many block/pin test
  //chessBoard.setupFen("8/1R4bb/8/8/2p3k1/4P3/P2P4/K7 w - - 0 1"); // en passant pin ray/block test
  //chessBoard.setupFen("7k/7p/1p2p3/3pP3/3P4/6P1/P7/K7"); // pawn tests
  //chessBoard.setupFen("8/4kp1p/8/8/p1pPp1P1/8/1P2K3/8 b - d3 0 1"); // en passant test
  //chessBoard.setupFen("rkrnr3/1P1P4/2P5/3P4/4p3/5p2/4p1p1/3RNRKR w - - 0 1"); // capture promotion test
  //chessBoard.setupFen("4n1k1/6P1/4P1pP/6P1/4N3/8/7K/8"); // only valid move test // Nf6 check
  chessBoard.setupFen("4p3/2pkp3/4p2b/8/8/3B4/3R2K1/8 w - - 0 1"); // double check test // Bb5
  //chessBoard.setupFen("8/8/p1p5/1p5p/1P5p/8/PPP2K1p/4R1rk"); // null move, mate in 1
  //chessBoard.setupFen("8/3p4/P5r1/1KP2qk1/6r1/8/8/1N4R1"); // pin test
  //chessBoard.setupFen("B3rr2/8/1k6/b7/B6b/6R1/3N4/3PK3 w - - 0 1"); // a1 bishop can block check, block attacker test
  //chessBoard.setupFen("7R/k7/P2n3R/PPnb1b2/4r3/8/1q6/3K2B1"); // rh6 block checkmate test
  //chessBoard.setupFen("8/8/7k/7r/K6r/7r/1P6/1Q6"); // block checkmate using pawn test
  //chessBoard.setupFen("2k5/8/4q3/6q1/3Q4/5Q2/8/1K6");
  //chessBoard.setupFen("5r2/8/6k1/6pp/8/3Q4/2K5/8 w - - 0 1");

  allowEmojis();

  do {
    system("CLS");
    std::cout << " " << pieces.toUnicode(pieces.PAWN) << " CHESS MENU "
              << pieces.toUnicode(pieces.PAWN);
    setTxtColor(chessCache.greyLetCol);
    std::cout << " V5.1";  // VERSION
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
      chessBoard.setupFen(getFen());
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

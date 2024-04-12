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
    - using char variable type for smaller memmory / so my laptop doesnt get
angry
    - using ++i pre-increment insted of post-increment i++ in loops for bit
faster loops


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

#include <windows.h>  // only for colored visuals

#include <chrono>  // only for timing performance tests
#include <iostream>

void setTxtColor(int colorValue);

std::string intToString(int num) {
  std::string result;
  while (num != 0) {
    char digit = '0' + (num % 10);  // Convert the digit to ASCII
    result = digit + result;        // Append the digit to the result string
    num /= 10;                      // Move to the next digit
  }
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
  const unsigned long long oneULL = 1ULL;
  unsigned long long bitBoard = 0;  // 64 bits to represent highlighted squares
 public:
  void setSquare(unsigned char square) {
    bitBoard |= (oneULL << square);  // set to byte using bitwise or
  };

  void unSetSquare(unsigned char square) {
    bitBoard &= ~(oneULL << square);  // clear byte using bitwise and
  };

  bool isSet(unsigned char square) const {
    // check if the bit corresponding to the square is set
    return (bitBoard >> square) & 1;
  };

  void clearBoard() { bitBoard = 0; };
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

  unsigned char type(const unsigned char piece) const {
    return (piece & TYPE_MASK);
  }

  unsigned char color(const unsigned char piece) const {
    return (piece & COLOR_MASK);
  }

  const std::string getPieceTypeUnicode(const unsigned char pieceType) const {
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
  const int NoFlag = 0b0000;                // 0
  const int EnPassantCaptureFlag = 0b0001;  // 1
  const int CastleFlag = 0b0010;            // 2
  const int PawnTwoUpFlag = 0b0011;         // 3

  const int PromoteToQueenFlag = 0b0100;   // 4
  const int PromoteToKnightFlag = 0b0101;  // 5
  const int PromoteToRookFlag = 0b0110;    // 6
  const int PromoteToBishopFlag = 0b0111;  // 7

  // other
  const unsigned short Null = 0;
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
  bool isDoublePawnPush() const { return flag() == moveFlags.PawnTwoUpFlag; }

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

  // visual colors
  const unsigned char hlightCol = 240;  // White background Black text
  const unsigned char wBlackCol = 128;  // Gray background black letter
  const unsigned char wWhiteCol = 143;  // Gray background white letter
  const unsigned char bWhiteCol = 15;   // Black background white latter
  const unsigned char bBlackCol = 8;    // Black background gray letter
  const unsigned char greyLetCol = 8;   // Black background gray letter

  // pre-compute pseudo legal moves for each tile
  Move bPawnMoves[64][4];   // 2 max possible moves 2 diagonal takes
  Move wPawnMoves[64][4];   // 2 max possible moves 2 diagonal takes
  Move knightMoves[64][8];  // 8 max possible moves
  Move kingMoves[64][8];    // 8 max possible moves

  // directions
  // [0] is
  Move bishopMoves[4][64]
                  [7];  // 13 max, 4 directions, 7 moves max in each direction
  Move rookMoves[4][64]
                [7];  // 14 max, 4 directions, 7 moves max in each direction

  Move whiteKingSideCastle;
  Move whiteQueenSideCastle;
  Move blackKingSideCastle;
  Move blackQueenSideCastle;

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

  bool inBounds(int tile) const { return tile >= 0 && tile < 64; };

  PreComputedCache() {
    int dfar = 0;
    //
    for (int i = 0; i < 64; ++i) {
      unsigned char row = (i / 8);
      unsigned char col = (i % 8);
      rowColValues[row][col] = i;
      preComputedRows[i] = row;
      preComputedCols[i] = col;

      // white pawns
      if (row < 7) {
        wPawnMoves[i][0] = Move(i, i + 8);
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
        bPawnMoves[i][0] = Move(i, i - 8);
        if (col < 7) {  // capture
          bPawnMoves[i][3] = Move(i, i - 7);
        }
        if (col > 0) {  // capture
          bPawnMoves[i][2] = Move(i, i - 9);
        }
        if (row == 6) {
          bPawnMoves[i][1] = Move(i, i - 16);
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
            // cout << "i: " << i << " = destination: " << destination << ",
            // dcol: " << static_cast<int>(dCol) << "\n";
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
            // cout << "i: " << i << " = destination: " << destination << ",
            // dcol: " << static_cast<int>(dCol) << "\n";
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
  };
};

const PreComputedCache chessCache;

struct moveList {
  Move moves[218];  // 218 max moves in chess
  unsigned char amt = 0;

  void addConstMove(const Move m) {
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
  unsigned char amt = 0;

  void addAtTile(int tile) {
    pieces[amt] = tile;
    pieceMap[tile] = amt;
    ++amt;
  };

  void removeAtTile(int tile) {
    int pieceIndex = pieceMap[tile];
    pieces[pieceIndex] = pieces[amt - 1];
    pieceMap[pieces[pieceIndex]] = pieceIndex;
    --amt;
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
  unsigned short gameStateHistory[200] = {0b0000000000000000};  // Array to hold game states
  int top;

 public:
  gameStateStack() : top(-1) {}

  bool isEmpty() { return top == -1; }

  bool isFull() { return top == 200 - 1; }

  void push(unsigned short gameState) {
    if (isFull()) {
      //std::cout << "Stack Overflow\n";
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

  unsigned short peek() {
    if (isEmpty()) {
      //std::cout << "Stack is empty\n";
      return 0;  // Returning 0 as error value
    }
    return gameStateHistory[top];
  }
};

class Board {
 private:
  // index position of the kings
  unsigned char whiteKing = 0;
  unsigned char blackKing = 0;

  // Total plies (half-moves) played in game
  int plyCount = 0;
  int turn = pieces.WHITE;
  int oppTurn = pieces.BLACK; // opposite of turn

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

  // bitboards
  BitBoard allPieces;

  // board
  int board[64];

  void resetValues() {
    gameStateStack newGameHistory;
    unsigned short initialGameState = 0b0000000000000000;
    currentGameState = initialGameState;
    gameStateHistory = newGameHistory;
    gameStateHistory.push(initialGameState);

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
  };

 public:
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
    } else if (fenTurn == "b") {
      turn = pieces.BLACK;
    }

    for (char c : castlingFen) {
      if (c == 'K') {
        // whiteKingSideCastle = true;
      } else if (c == 'Q') {
        // whiteQueenSideCastle = true;
      } else if (c == 'k') {
        // blackKingSideCastle = true;
      } else if (c == 'q') {
        // blackQueenSideCastle = true;
      }
    }

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
  };

  void display(bool whiteSide, BitBoard &highlights, std::string line1 = " ",
               std::string line2 = " ", std::string line3 = " ",
               std::string line4 = " ") const {
    for (int i = 0; i < 64; ++i) {
      int i2 = i;
      if (whiteSide) {
        i2 = 63 - i;
      }

      int row = chessCache.preComputedRows[i2];
      int col = chessCache.preComputedCols[i2];
      if (chessCache.preComputedCols[i] == 0) {
        setTxtColor(chessCache.greyLetCol);
        std::cout << (row + 1) << '|';
      }

      if (highlights.isSet(i2)) {
        setTxtColor(chessCache.hlightCol);
      } else {
        int colorIndex = (row + col) % 2;
        int pieceColor = pieces.color(board[i2]) == 0 ? 0 : 1;
        if (colorIndex) {
          setTxtColor(pieceColor == 0 ? chessCache.wWhiteCol
                                      : chessCache.wBlackCol);
        } else {
          setTxtColor(pieceColor == 0 ? chessCache.bWhiteCol
                                      : chessCache.bBlackCol);
        }
      }
      std::cout << pieces.getPieceTypeUnicode(pieces.type(board[i2])) << " ";
      if ((i + 1) % 8 == 0) {
        setTxtColor(chessCache.greyLetCol);
        if (chessCache.preComputedRows[i] == 0) {
          if (turn) {  // if blacks turn
            std::cout << "  turn: b,";
          } else {
            std::cout << "  turn: w,";
          }
          std::cout << "  ply: " << plyCount
                    << ",  wking: " << intToString(whiteKing)
                    << ",  bking: " << intToString(blackKing);
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
      turn = pieces.WHITE;
    } else {
      turn = pieces.BLACK;
    }
  };

  bool isCapture(const Move &m) const {
    if (m.isNull()) {
      return false;
    }
    return (board[m.moveTo()] != pieces.EMPTY);
  };

  void addLegal(moveList &m, const Move &move) const {
    if (board[move.moveTo()] == pieces.EMPTY ||
        pieces.color(board[move.moveFrom()]) !=
            pieces.color(board[move.moveTo()])) {
      m.addConstMove(move);
    }
  };

  // manually written/hard coded for less looping, so its a bit faster
  void generatePawnMoves(moveList &m) const {
    if (turn == pieces.WHITE) {
      for (int i = 0; i < pawns[1].amt; ++i) {
        int pieceIndex = pawns[1].pieces[i];
        addLegal(m, chessCache.wPawnMoves[pieceIndex][0]);
        addLegal(m, chessCache.wPawnMoves[pieceIndex][1]);
        if (isCapture(chessCache.wPawnMoves[pieceIndex][2])) {
          addLegal(m, chessCache.wPawnMoves[pieceIndex][2]);
        }
        if (isCapture(chessCache.wPawnMoves[pieceIndex][3])) {
          addLegal(m, chessCache.wPawnMoves[pieceIndex][3]);
        }
      }
    } else {
      for (int i = 0; i < pawns[0].amt; ++i) {
        int pieceIndex = pawns[0].pieces[i];
        addLegal(m, chessCache.bPawnMoves[pieceIndex][0]);
        addLegal(m, chessCache.bPawnMoves[pieceIndex][1]);
        if (isCapture(chessCache.bPawnMoves[pieceIndex][2])) {
          addLegal(m, chessCache.bPawnMoves[pieceIndex][2]);
        }
        if (isCapture(chessCache.bPawnMoves[pieceIndex][3])) {
          addLegal(m, chessCache.bPawnMoves[pieceIndex][3]);
        }
      }
    }
  };

  // manually written/hard coded for less looping, so its a bit faster
  void generateKnightMoves(moveList &m) const {
    if (turn == pieces.WHITE) {
      for (int i = 0; i < knights[1].amt; ++i) {
        for (int j = 0; j < 8; ++j) {
          Move addingMove = chessCache.knightMoves[knights[1].pieces[i]][j];
          addLegal(m, addingMove);
        }
      }
    } else {
      for (int i = 0; i < knights[0].amt; ++i) {
        for (int j = 0; j < 8; ++j) {
          Move addingMove = chessCache.knightMoves[knights[0].pieces[i]][j];
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
            Move addingMove =
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
            Move addingMove = chessCache.bishopMoves[d][queens[1].pieces[i]][r];
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
            Move addingMove =
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
            Move addingMove = chessCache.bishopMoves[d][queens[0].pieces[i]][r];
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
            Move addingMove = chessCache.rookMoves[d][rooks[1].pieces[i]][r];
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
            Move addingMove = chessCache.rookMoves[d][queens[1].pieces[i]][r];
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
            Move addingMove = chessCache.rookMoves[d][rooks[0].pieces[i]][r];
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
            Move addingMove = chessCache.rookMoves[d][queens[0].pieces[i]][r];
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
        Move addingMove = chessCache.kingMoves[whiteKing][j];
        addLegal(m, addingMove);
      }
    } else {
      for (int j = 0; j < 8; ++j) {
        Move addingMove = chessCache.kingMoves[blackKing][j];
        addLegal(m, addingMove);
      }
    }
  };

  void generateMoves(moveList &moves) const {
    generatePawnMoves(moves);
    generateKnightMoves(moves);
    generateBishopMoves(moves);
    generateRookMoves(moves);
    generateKingMoves(moves);
  };

  void makeMove(Move &m) {
    currentGameState = 0;
    int startSquare = m.moveFrom();
    int targetSquare = m.moveTo();

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
    } else if (board[startSquare] == pieces.BROOK) {
      rooks[0].MovePiece(startSquare, targetSquare);
    } else if (board[startSquare] == pieces.WQUEEN) {
      queens[1].MovePiece(startSquare, targetSquare);
    } else if (board[startSquare] == pieces.BQUEEN) {
      queens[0].MovePiece(startSquare, targetSquare);
    } else if (startSquare == whiteKing) {
      whiteKing = targetSquare;
    } else if (startSquare == blackKing) {
      blackKing = targetSquare;
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
    } else if (board[targetSquare] == pieces.BROOK) {
      rooks[0].removeAtTile(targetSquare);
    } else if (board[targetSquare] == pieces.WQUEEN) {
      queens[1].removeAtTile(targetSquare);
    } else if (board[targetSquare] == pieces.BQUEEN) {
      queens[0].removeAtTile(targetSquare);
    }

    allPieces.setSquare(targetSquare);
    allPieces.unSetSquare(startSquare);

    currentGameState |= (pieces.type(board[targetSquare]) << 8);

    board[targetSquare] = board[startSquare];
    board[startSquare] = pieces.EMPTY;
    makeTurn();

    ++plyCount;
  };

  void unMakeMove(Move &m) {
    --plyCount;
    int startSquare = m.moveFrom();
    int targetSquare = m.moveTo();

    unsigned char capturedPieceType = (currentGameState >> 8) & 63;  // 63 = 111111
    unsigned char capturedPiece = pieces.EMPTY;
    if (capturedPieceType != pieces.EMPTY) {
      capturedPiece = capturedPieceType | turn;
    };

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
  }

  Board() {
    setupFen("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
  };

  Board(std::string fen) { setupFen(fen); };
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
    chessBoard.makeMove(genMoves.moves[i]);

    //system("CLS");
    //chessBoard.display(false, cBoardHLight);

    PREFTData branchData = PERFT(chessBoard, depth - 1);
    // newData.captures += branchData.captures;
    newData.numPos += branchData.numPos;
    chessBoard.unMakeMove(genMoves.moves[i]);
  }
  return newData;
}

void perftTest(Board &chessBoard) {
  std::cout << "\nstarting PREFT\n";
  auto startstart = std::chrono::steady_clock::now();
  for (int d = 0; d < 10; d++) {
    auto start = std::chrono::steady_clock::now();
    PREFTData finaldata = PERFT(chessBoard, d);
    auto endt = std::chrono::steady_clock::now();
    auto duration =
        std::chrono::duration_cast<std::chrono::milliseconds>(endt - start).count();
    std::cout << "depth: " << d << " ply  ";
    std::cout << "result: " << finaldata.numPos << " nodes  ";
    std::cout << "Time: " << duration << " ms\n";
  }
  auto endendt = std::chrono::steady_clock::now();
  auto totalDuraction = std::chrono::duration_cast<std::chrono::milliseconds>(endendt - startstart).count();
  std::cout << "total time: " << totalDuraction << "ms\n\n";
  system("PAUSE"); 
};

void startGame(Board &chessBoard) {
  BitBoard cBoardHLight;
  Move previousMoves[200];
  std::string input = " ";

  int chessBoardPly = 0;  // amt of chess movse
  int moveFrom = 0;
  int moveTo = 0;

  do {
    system("CLS");
    cBoardHLight.clearBoard();
    chessBoard.display(false, cBoardHLight);

    std::cout << "input: ";
    std::cin >> input;

    input = toLowercase(input);
    if (input == "help") {
      std::cout << "\n -> COMMANDS <-\n"
                << "\"exit\" - exit current game\n"
                << "\"undo\" - undoes a move\n"
                << "\n\n";
      system("PAUSE");
    } else if (input == "undo") {
      if (chessBoardPly > 0) {
        --chessBoardPly;
        chessBoard.unMakeMove(previousMoves[chessBoardPly]);
      }
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
      chessBoard.display(false, cBoardHLight,
                         "  #moves: " + intToString(genMoves.amt));

      std::cout << "moveTo: ";
      std::cin >> input;
      moveTo = chessCache.notationToTile(input);
      for (int i = 0; i < genMoves.amt; ++i) {
        if (genMoves.moves[i].moveFrom() == moveFrom &&
            genMoves.moves[i].moveTo() == moveTo) {
          chessBoard.makeMove(genMoves.moves[i]);
          previousMoves[chessBoardPly] = genMoves.moves[i];
          ++chessBoardPly;
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

  allowEmojis();

  do {
    system("CLS");
    std::cout << " " << pieces.getPieceTypeUnicode(1) << " CHESS MENU "
              << pieces.getPieceTypeUnicode(1);
    setTxtColor(chessCache.greyLetCol);
    std::cout << " V4.3";  // VERSION
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

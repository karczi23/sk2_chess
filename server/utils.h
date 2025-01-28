#ifndef UTILS_H
#define UTILS_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/select.h>
#include <signal.h>
#include <errno.h>
#include <ctype.h>

#define MAX_GAMES 50

typedef struct
{
    int row;
    int col;
} Tile;

typedef struct
{
    int player1_socket;
    int player2_socket;
    char board[8][8];
    int current_player;
    int turn;
    int game_id;
    int is_active;
    Tile white_king;
    Tile black_king;
    int white_checked;
    int black_checked;
} ChessGame;

typedef struct
{
    ChessGame games[MAX_GAMES];
    int active_games;
} GameManager;

typedef struct
{
    int from_row;
    int from_col;
    int to_row;
    int to_col;
} Move;

void init_game_manager(GameManager *gm);
int find_available_game(GameManager *gm);
ChessGame *find_game_by_socket(GameManager *gm, int socket);
int is_diagonal_move(Move *move);
int is_straight_move(Move *move);
int is_king_one_square_move(Move *move);
int is_knight_move(Move *move);
int is_pawn_one_square_move(Move *move);
int check_validity(char piece, Move *move);
int check_diagonals(char board[8][8], Tile *tile, int isPlayerWhite);
int check_straights(char board[8][8], Tile *tile, int isPlayerWhite);
int check_knight(char board[8][8], Tile *tile, int isPlayerWhite);
int is_king_checked(ChessGame *game, int isPlayerWhite);
int is_pawn_takes_move_validation(Move *move);
int is_pawn_takes(int isPlayerWhite, Move *move, char board[8][8]);
void move_king(ChessGame *game, Move *move);
int can_king_be_moved(ChessGame *game, int isPlayerWhite);
int can_be_taken(ChessGame *game, Move *move, int isPlayerWhite);
int can_be_blocked(ChessGame *game, Move *move, int isPlayerWhite);
#endif // UTILS_H
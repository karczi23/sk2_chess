#include "utils.h"
#include <stdlib.h>
#include <ctype.h>

#define MAX(x, y) (((x) > (y)) ? (x) : (y))
#define MIN(x, y) (((x) < (y)) ? (x) : (y))

int is_diagonal_move(Move *move)
{
    return abs(move->from_col - move->to_col) == abs(move->from_row - move->to_row);
}

int is_straight_move(Move *move)
{
    return move->from_col == move->to_col || move->from_row == move->to_row;
}

int is_king_one_square_move(Move *move)
{
    return abs(move->from_col - move->to_col) <= 1 &&
           abs(move->from_row - move->to_row) <= 1;
}

int is_knight_move(Move *move)
{
    int col_diff = abs(move->from_col - move->to_col);
    int row_diff = abs(move->from_row - move->to_row);
    return (col_diff == 2 && row_diff == 1) || (col_diff == 1 && row_diff == 2);
}

int is_pawn_one_square_move(Move *move)
{
    return move->from_col == move->to_col && abs(move->from_row - move->to_row) == 1;
}

int is_pawn_takes_move_validation(Move *move)
{
    int col_diff = abs(move->from_col - move->to_col);
    int row_diff = abs(move->from_row - move->to_row);
    return col_diff == 1 && row_diff == 1;
}

int is_pawn_takes(int isPlayerWhite, Move *move, char board[8][8])
{
    char opposing_piece = board[move->to_row][move->to_col];
    int is_opposing_piece_white = opposing_piece < 96;
    return isPlayerWhite != is_opposing_piece_white;
}

void move_king(ChessGame *game, Move *move)
{
    char(*board)[8] = game->board;
    char piece = board[move->from_row][move->from_col];
    Tile tile;
    tile.col = move->to_col;
    tile.row = move->to_row;
    if (piece == 'K')
    {
        game->white_king = tile;
    }
    if (piece == 'k')
    {
        game->black_king = tile;
    }
}

int can_king_be_moved(ChessGame *game, int isPlayerWhite)
{
    Tile king = isPlayerWhite ? game->white_king : game->black_king;

    int directions[8][2] = {
        {-1, -1}, {-1, 0}, {-1, 1}, {0, -1}, {0, 1}, {1, -1}, {1, 0}, {1, 1}};

    for (int i = 0; i < 8; i++)
    {
        int new_row = king.row + directions[i][0];
        int new_col = king.col + directions[i][1];

        if (new_row >= 0 && new_row < 8 && new_col >= 0 && new_col < 8)
        {
            char original_piece = game->board[new_row][new_col];

            char king_char = isPlayerWhite ? 'K' : 'k';
            int can_move_here = (original_piece == '.' ||
                                 (isPlayerWhite ? (original_piece >= 'a') : (original_piece < 'a')));

            if (can_move_here)
            {
                game->board[king.row][king.col] = '.';
                game->board[new_row][new_col] = king_char;

                Tile original_pos = king;
                if (isPlayerWhite)
                {
                    game->white_king.row = new_row;
                    game->white_king.col = new_col;
                }
                else
                {
                    game->black_king.row = new_row;
                    game->black_king.col = new_col;
                }

                int is_checked = is_king_checked(game, isPlayerWhite);

                game->board[king.row][king.col] = king_char;
                game->board[new_row][new_col] = original_piece;
                if (isPlayerWhite)
                {
                    game->white_king = original_pos;
                }
                else
                {
                    game->black_king = original_pos;
                }

                if (!is_checked)
                {
                    return 1;
                }
            }
        }
    }

    return 0;
}

int check_validity(char piece, Move *move)
{
    switch (piece)
    {
    case 'k':
        return is_king_one_square_move(move);
    case 'q':
        return is_diagonal_move(move) || is_straight_move(move);
    case 'b':
        return is_diagonal_move(move);
    case 'n':
        return is_knight_move(move);
    case 'r':
        return is_straight_move(move);
    case 'p':
        return is_pawn_one_square_move(move) || is_pawn_takes_move_validation(move);
    default:
        return 0;
    }
}

Tile get_board_indices(char *position)
{
    Tile tile;
    tile.col = position[0] - 'a';
    tile.row = '8' - position[1];
    return tile;
}

int check_diagonals(char board[8][8], Tile *tile, int isPlayerWhite)
{
    char queen = isPlayerWhite ? 'q' : 'Q';
    char bishop = isPlayerWhite ? 'b' : 'B';
    int directions[4][2] = {
        {1, 1},   // up-right
        {1, -1},  // up-left
        {-1, -1}, // down-left
        {-1, 1}   // down-right
    };
    for (int dir = 0; dir < 4; dir++)
    {
        int col = tile->col;
        int row = tile->row;

        do
        {
            col += directions[dir][0];
            row += directions[dir][1];
            char piece = board[row][col];
            if (piece == queen || piece == bishop)
            {
                return 1;
            }
            if (piece != '.')
            {
                break;
            }
        } while (col > 0 && col < 7 && row > 0 && row < 7);
    }

    return 0;
}

int check_straights(char board[8][8], Tile *tile, int isPlayerWhite)
{
    char queen = isPlayerWhite ? 'q' : 'Q';
    char rook = isPlayerWhite ? 'r' : 'R';
    int directions[4][2] = {
        {1, 0},  // down
        {0, 1},  // right
        {-1, 0}, // up
        {0, -1}  // left
    };
    for (int dir = 0; dir < 4; dir++)
    {
        int col = tile->col;
        int row = tile->row;

        // Move along the diagonal until edge of board
        do
        {
            col += directions[dir][0];
            row += directions[dir][1];
            char piece = board[row][col];
            if (piece == queen || piece == rook)
            {
                return 1;
            }
            if (piece != '.')
            {
                break;
            }
        } while (col > 0 && col < 7 && row > 0 && row < 7);
    }

    return 0;
}

int check_knight(char board[8][8], Tile *tile, int isPlayerWhite)
{
    char knight = isPlayerWhite ? 'n' : 'N';
    int directions[8][2] = {
        {2, 1},
        {1, 2},
        {2, -1},
        {1, -2},
        {-2, 1},
        {-1, 2},
        {-2, -1},
        {-1, -2}};
    for (int dir = 0; dir < 8; dir++)
    {
        int col = tile->col + directions[dir][0];
        int row = tile->row + directions[dir][1];

        if (col >= 0 && col < 8 && row >= 0 && row < 8)
        {
            char piece = board[row][col];
            if (piece == knight)
            {
                return 1;
            }
        }
    }
    return 0;
}

int is_king_checked(ChessGame *game, int isPlayerWhite)
{
    Tile tile = isPlayerWhite ? game->white_king : game->black_king;

    int diag = check_diagonals(game->board, &tile, isPlayerWhite);

    int straight = check_straights(game->board, &tile, isPlayerWhite);

    int knight = check_knight(game->board, &tile, isPlayerWhite);

    return diag || straight || knight;
}

int can_be_taken(ChessGame *game, Move *move, int isPlayerWhite)
{
    Tile tile;
    tile.col = move->to_col;
    tile.row = move->to_row;

    int diag = check_diagonals(game->board, &tile, isPlayerWhite);

    int straight = check_straights(game->board, &tile, isPlayerWhite);

    int knight = check_knight(game->board, &tile, isPlayerWhite);

    return diag || straight || knight;
}

int get_step(int a, int b)
{
    if (b > a)
        return 1;
    if (b < a)
        return -1;
    return 0;
}

int can_be_blocked(ChessGame *game, Move *move, int isPlayerWhite)
{
    Tile tile = isPlayerWhite ? game->white_king : game->black_king;
    Tile attacking_tile;
    attacking_tile.col = move->to_col;
    attacking_tile.row = move->to_row;

    int x_step = get_step(attacking_tile.row, tile.row);
    int y_step = get_step(attacking_tile.col, tile.col);

    int current_x = attacking_tile.row;
    int current_y = attacking_tile.col;

    while (current_x != tile.row || current_y != tile.col)
    {
        current_x += x_step;
        current_y += y_step;

        if (current_x == tile.row && current_y == tile.col)
        {
            return 0;
        }
        // printf("(%d,%d) ", current_x, current_y);
        Tile new_tile;
        new_tile.col = current_y;
        new_tile.row = current_x;
        int diag = check_diagonals(game->board, &new_tile, !isPlayerWhite);

        int straight = check_straights(game->board, &new_tile, !isPlayerWhite);

        int knight = check_knight(game->board, &new_tile, !isPlayerWhite);

        if (diag || straight || knight)
        {
            return 1;
        }
    }
    return 0;
}

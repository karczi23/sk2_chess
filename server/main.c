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
#include "utils.h"

#define PORT 8080
#define BUFFER_SIZE 1024

int port, game_time, increment;

// Global variables for cleanup
static int server_fd;
static GameManager *global_game_manager;
volatile sig_atomic_t server_running = 1;

// Signal handler function
void handle_shutdown()
{
    printf("\nShutting down server gracefully...\n");
    server_running = 0;
}

void send_error(int socket, char *message)
{
    char buffer[BUFFER_SIZE];
    memset(buffer, 0, BUFFER_SIZE);
    sprintf(buffer, "e %s\n", message);
    send(socket, buffer, strlen(buffer), 0);
}

// Cleanup function
void cleanup_server()
{
    if (global_game_manager != NULL)
    {
        // Close all active game connections
        for (int i = 0; i < MAX_GAMES; i++)
        {
            if (global_game_manager->games[i].is_active)
            {
                if (global_game_manager->games[i].player1_socket > 0)
                {
                    send(global_game_manager->games[i].player1_socket,
                         "Server shutting down. Game over.\n", 32, 0);
                    close(global_game_manager->games[i].player1_socket);
                }
                if (global_game_manager->games[i].player2_socket > 0)
                {
                    send(global_game_manager->games[i].player2_socket,
                         "Server shutting down. Game over.\n", 32, 0);
                    close(global_game_manager->games[i].player2_socket);
                }
            }
        }
    }
    // Close server socket
    if (server_fd > 0)
    {
        close(server_fd);
    }

    printf("Server shutdown complete.\n");
}

// Initialize the game manager
void init_game_manager(GameManager *gm)
{
    gm->active_games = 0;
    for (int i = 0; i < MAX_GAMES; i++)
    {
        gm->games[i].player1_socket = -1;
        gm->games[i].player2_socket = -1;
        gm->games[i].is_active = 0;
        gm->games[i].game_id = i;
    }
}

// Find an available game slot
int find_available_game(GameManager *gm)
{
    for (int i = 0; i < MAX_GAMES; i++)
    {
        if (!gm->games[i].is_active)
        {
            return i;
        }
    }
    return -1;
}

// Find game by socket
ChessGame *find_game_by_socket(GameManager *gm, int socket)
{
    for (int i = 0; i < MAX_GAMES; i++)
    {
        if (gm->games[i].is_active &&
            (gm->games[i].player1_socket == socket ||
             gm->games[i].player2_socket == socket))
        {
            return &gm->games[i];
        }
    }
    return NULL;
}

// Initialize the chess board
void init_board(ChessGame *game)
{
    // Initialize pieces (simplified version)
    // First row for black pieces
    game->board[0][0] = 'r'; // rook
    game->board[0][1] = 'n'; // knight
    game->board[0][2] = 'b'; // bishop
    game->board[0][3] = 'q'; // queen
    game->board[0][4] = 'k'; // king
    game->board[0][5] = 'b';
    game->board[0][6] = 'n';
    game->board[0][7] = 'r';

    // Black pawns
    for (int i = 0; i < 8; i++)
    {
        game->board[1][i] = 'p';
    }

    // Empty squares
    for (int i = 2; i < 6; i++)
    {
        for (int j = 0; j < 8; j++)
        {
            game->board[i][j] = '.';
        }
    }

    // White pawns
    for (int i = 0; i < 8; i++)
    {
        game->board[6][i] = 'P';
    }

    // Last row for white pieces
    game->board[7][0] = 'R';
    game->board[7][1] = 'N';
    game->board[7][2] = 'B';
    game->board[7][3] = 'Q';
    game->board[7][4] = 'K';
    game->board[7][5] = 'B';
    game->board[7][6] = 'N';
    game->board[7][7] = 'R';

    game->current_player = 1; // White starts

    Tile white;
    white.col = 4;
    white.row = 7;
    game->white_king = white;

    Tile black;
    black.col = 4;
    black.row = 0;
    game->black_king = black;
}

// Send the current board state to a player
void send_board(int socket, ChessGame *game)
{
    char buffer[BUFFER_SIZE];
    memset(buffer, 0, BUFFER_SIZE);

    sprintf(buffer, "\nGame #%d\n", game->game_id);
    send(socket, buffer, strlen(buffer), 0);

    memset(buffer, 0, BUFFER_SIZE);
    sprintf(buffer, "board ");
    int pos = 6;
    for (int i = 0; i < 8; i++)
    {
        for (int j = 0; j < 8; j++)
        {
            buffer[pos++] = game->board[i][j];
        }
        buffer[pos++] = '\n';
    }
    send(socket, buffer, strlen(buffer), 0);
}

Move convert(char *move)
{
    Move move_obj;

    move_obj.from_col = move[0] - 'a';
    move_obj.from_row = '8' - move[1];
    move_obj.to_col = move[2] - 'a';
    move_obj.to_row = '8' - move[3];

    return move_obj;
}

// Helper functions to make the code more readable

int verify_move(int socket, ChessGame *game, Move *move)
{
    char buffer[BUFFER_SIZE];
    memset(buffer, 0, BUFFER_SIZE);
    // check boundaries
    if (move->from_col < 0 || move->from_col > 7 || move->from_row < 0 || move->from_row > 7 ||
        move->to_col < 0 || move->to_col > 7 || move->to_row < 0 || move->to_row > 7)
    {
        // send(socket, "Invalid move coordinates!\n", 25, 0);
        send_error(socket, "Invalid move coordinates!");
        return 0;
    }

    if (move->from_col == move->to_col && move->from_row == move->to_row)
    {
        // send(socket, "You have to make a move!\n", 24, 0);
        send_error(socket, "You have to make a move!");
        return 0;
    }

    char piece = game->board[move->from_row][move->from_col];

    if (piece == '.')
    {
        // send(socket, "You have to move an existing piece!\n", 35, 0);
        send_error(socket, "You have to move an existing piece!");
        return 0;
    }

    int is_player1 = (socket == game->player1_socket);

    // check if player white moves black pieces
    if (is_player1 && piece > 96)
    {
        // send(socket, "You can't move black's pieces!\n", 30, 0);
        send_error(socket, "You can't move black's pieces!");
        return 0;
    }

    if (!is_player1 && piece < 96)
    {
        // send(socket, "You can't move white's pieces!\n", 30, 0);
        send_error(socket, "You can't move white's pieces!");
        return 0;
    }

    char lower_piece = tolower(piece);

    if (lower_piece == 'p' && is_pawn_takes_move_validation(move))
    {
        return is_pawn_takes(is_player1, move, game->board);
    }

    if (!check_validity(tolower(lower_piece), move))
    {
        // send(socket, "This move cannot be played by this piece!\n", 41, 0);
        send_error(socket, "This move cannot be played by this piece!");
        return 0;
    }

    return 1;
}

int apply_move(Move *move, char board[8][8])
{
    char piece_taken = board[move->to_row][move->to_col];
    board[move->to_row][move->to_col] = board[move->from_row][move->from_col];
    board[move->from_row][move->from_col] = '.';
    return piece_taken;
}

void revert_move(Move *move, char board[8][8], char piece_taken)
{
    board[move->from_row][move->from_col] = board[move->to_row][move->to_col];
    board[move->to_row][move->to_col] = piece_taken;
}

void finish_game(ChessGame *game, int winner)
{
    game->is_active = 0;
    int winner_socket = winner ? game->player1_socket : game->player2_socket;
    int loser_socket = winner ? game->player2_socket : game->player1_socket;
    send(winner_socket, "You win! Game over.\n", 21, 0);
    close(winner_socket);
    send(loser_socket, "You lost. Game over.\n", 22, 0);
    close(loser_socket);
}

// Handle moves
int handle_move(int socket, GameManager *gm, char *move)
{
    char buffer[BUFFER_SIZE];
    memset(buffer, 0, BUFFER_SIZE);
    ChessGame *game = find_game_by_socket(gm, socket);
    Move move_obj = convert(move);
    if (!game)
    {
        send(socket, "Game not found!\n", 15, 0);
        return 0;
    }

    // Check if it's this player's turn
    int is_player1 = (socket == game->player1_socket);
    if ((is_player1 && game->current_player != 1) ||
        (!is_player1 && game->current_player != 2))
    {
        // send(socket, "Not your turn!\n", 14, 0);
        send_error(socket, "Not your turn!");
        return 0;
    }

    // Basic move validation (example: "a2a4")
    // sprintf(buffer, "move length: %ld, move: %s", strlen(move), move);
    // send(socket, buffer, strlen(buffer), 0);
    printf("%s\n", move);
    if (strlen(move) != 4)
    {
        // send(socket, "Invalid move format! Use format: a2a4\n", 37, 0);
        send_error(socket, "Invalid move format! Use format: a2a4");
        return 0;
    }

    // Basic boundary check
    if (!verify_move(socket, game, &move_obj))
    {
        // send(socket, "This move is illegal \n", 21, 0);
        send_error(socket, "This move is illegal");
        return 0;
    };

    printf("before: %c, after: %c\n", game->board[move_obj.from_row][move_obj.from_col], game->board[move_obj.to_row][move_obj.to_col]);
    // Make the move
    // game->board[move_obj.to_row][move_obj.to_col] = game->board[move_obj.from_row][move_obj.from_col];
    // game->board[move_obj.from_row][move_obj.from_col] = '.';
    move_king(game, &move_obj);

    char piece_taken = apply_move(&move_obj, game->board);

    int is_piece_taken_black = piece_taken > 96;

    if (piece_taken != '.' && is_player1 != is_piece_taken_black)
    {
        revert_move(&move_obj, game->board, piece_taken);
        // send(socket, "You can't take your own pieces!\n", 33, 0);
        send_error(socket, "You can't take your own pieces!");
        return 0;
    }

    printf("checking for checks for player: %s\n", is_player1 ? "white" : "black");
    if (is_king_checked(game, is_player1))
    {
        printf("%s checked!\n", is_player1 ? "white" : "black");
        revert_move(&move_obj, game->board, piece_taken);
        send_error(socket, "This move is illegal as this piece is protecting your king from check");
        send_board(socket, game);
        return 0;
    }

    printf("checking for checks for player: %s\n", !is_player1 ? "white" : "black");
    if (is_king_checked(game, !is_player1))
    {
        printf("%s checked!\n", !is_player1 ? "white" : "black");
        int a = can_king_be_moved(game, !is_player1);
        int b = can_be_taken(game, &move_obj, is_player1);
        int c = can_be_blocked(game, &move_obj, !is_player1);
        if (!a && !b && !c)
        {
            send_board(game->player1_socket, game);
            send_board(game->player2_socket, game);
            finish_game(game, is_player1);
        }
        send(game->player1_socket, "Check!\n", 8, 0);
        send(game->player2_socket, "Check!\n", 8, 0);
    }

    // Switch turns
    game->current_player = (game->current_player == 1) ? 2 : 1;

    // Send updated board to both players
    send_board(game->player1_socket, game);
    send_board(game->player2_socket, game);

    return 1;
}

void parse_args(int argc, char **argv)
{
    if (argc < 1)
    {
        printf("usage: %s -p PORT\n", argv[0]);
        printf("OPTIONS\n");
        printf("  -p --port    PORT\n");
        printf("  --game-time  SECONDS\n");
        printf("  --increment SECONDS\n");
        exit(1);
    }

    for (int i = 1; i < argc - 1; i++)
    {
        if (strcmp(argv[i], "-p") == 0 || strcmp(argv[i], "--port") == 0)
        {
            port = atoi(argv[++i]);
        }
    }
}

int main(int argc, char **argv)
{
    setbuf(stdout, NULL); // Add this line at the start of main
    parse_args(argc, argv);

    int server_fd, new_socket;
    struct sockaddr_in address;
    int opt = 1;
    int addrlen = sizeof(address);
    GameManager game_manager;
    fd_set readfds;
    char buffer[BUFFER_SIZE];

    global_game_manager = &game_manager;

    // Set up signal handlers
    struct sigaction sa;
    sa.sa_handler = handle_shutdown;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;

    if (sigaction(SIGINT, &sa, NULL) == -1)
    {
        perror("sigaction");
        exit(1);
    }
    if (sigaction(SIGTERM, &sa, NULL) == -1)
    {
        perror("sigaction");
        exit(1);
    }
    init_game_manager(&game_manager);

    // Create socket
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0)
    {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    // Set socket options
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt)))
    {
        perror("setsockopt failed");
        cleanup_server();
        exit(EXIT_FAILURE);
    }

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(port);

    // Bind socket
    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0)
    {
        perror("Bind failed");
        cleanup_server();
        exit(EXIT_FAILURE);
    }

    // Listen for connections
    if (listen(server_fd, MAX_GAMES * 2) < 0)
    {
        perror("Listen failed");
        cleanup_server();
        exit(EXIT_FAILURE);
    }

    printf("Chess server started on port %d. Waiting for players...\n", port);

    struct timeval timeout;

    while (server_running)
    {
        FD_ZERO(&readfds);
        FD_SET(server_fd, &readfds);

        int max_sd = server_fd;

        // Add all active player sockets to the set
        for (int i = 0; i < MAX_GAMES; i++)
        {
            if (game_manager.games[i].is_active)
            {
                if (game_manager.games[i].player1_socket > 0)
                {
                    FD_SET(game_manager.games[i].player1_socket, &readfds);
                    max_sd = (game_manager.games[i].player1_socket > max_sd) ? game_manager.games[i].player1_socket : max_sd;
                }
                if (game_manager.games[i].player2_socket > 0)
                {
                    FD_SET(game_manager.games[i].player2_socket, &readfds);
                    max_sd = (game_manager.games[i].player2_socket > max_sd) ? game_manager.games[i].player2_socket : max_sd;
                }
            }
        }

        timeout.tv_sec = 1;
        timeout.tv_usec = 0;

        int activity = select(max_sd + 1, &readfds, NULL, NULL, &timeout);
        if (activity < 0 && errno != EINTR)
        {
            perror("select error");
            continue;
        }

        if (!server_running)
        {
            break;
        }

        // Handle new connections
        if (FD_ISSET(server_fd, &readfds))
        {
            if ((new_socket = accept(server_fd, (struct sockaddr *)&address,
                                     (socklen_t *)&addrlen)) < 0)
            {
                perror("accept");
                continue;
            }

            // Find or create a game for the new player
            int game_idx = -1;
            for (int i = 0; i < MAX_GAMES; i++)
            {
                if (game_manager.games[i].is_active &&
                    game_manager.games[i].player2_socket == -1)
                {
                    game_idx = i;
                    break;
                }
            }

            if (game_idx == -1)
            {
                game_idx = find_available_game(&game_manager);
                if (game_idx != -1)
                {
                    // Initialize new game
                    game_manager.games[game_idx].is_active = 1;
                    game_manager.games[game_idx].player1_socket = new_socket;
                    game_manager.games[game_idx].player2_socket = -1;
                    init_board(&game_manager.games[game_idx]);
                    game_manager.active_games++;

                    char msg[100];
                    sprintf(msg, "Welcome! You are Player White in Game #%d. Waiting for opponent...\n",
                            game_idx);
                    send(new_socket, msg, strlen(msg), 0);
                    send_board(new_socket, &game_manager.games[game_idx]);
                }
            }
            else
            {
                // Add second player to existing game
                game_manager.games[game_idx].player2_socket = new_socket;
                char msg[100];
                sprintf(msg, "Welcome! You are Player Black in Game #%d\n", game_idx);
                send(new_socket, msg, strlen(msg), 0);
                send_board(new_socket, &game_manager.games[game_idx]);

                // Notify both players that game is starting
                sprintf(msg, "Game #%d is starting!\n", game_idx);
                send(game_manager.games[game_idx].player1_socket, msg, strlen(msg), 0);
                send(game_manager.games[game_idx].player2_socket, msg, strlen(msg), 0);
            }
        }

        // Handle player moves
        for (int i = 0; i < MAX_GAMES; i++)
        {
            if (!game_manager.games[i].is_active)
                continue;

            ChessGame *game = &game_manager.games[i];

            // Check for player 1 activity
            if (game->player1_socket > 0 &&
                FD_ISSET(game->player1_socket, &readfds))
            {
                memset(buffer, 0, BUFFER_SIZE);
                int valread = read(game->player1_socket, buffer, BUFFER_SIZE);
                if (valread <= 0)
                {
                    printf("Player 1 disconnected from game %d\n", i);
                    close(game->player1_socket);
                    if (game->player2_socket > 0)
                    {
                        send(game->player2_socket,
                             "x Opponent disconnected. Game over.\n", 32, 0);
                        close(game->player2_socket);
                    }
                    game->is_active = 0;
                    game_manager.active_games--;
                }
                else
                {
                    buffer[valread - 1] = '\0';
                    handle_move(game->player1_socket, &game_manager, buffer);
                }
            }

            // Check for player 2 activity
            if (game->player2_socket > 0 &&
                FD_ISSET(game->player2_socket, &readfds))
            {
                memset(buffer, 0, BUFFER_SIZE);
                int valread = read(game->player2_socket, buffer, BUFFER_SIZE);
                if (valread <= 0)
                {
                    printf("Player 2 disconnected from game %d\n", i);
                    close(game->player2_socket);
                    if (game->player1_socket > 0)
                    {
                        send(game->player1_socket,
                             "x Opponent disconnected. Game over.\n", 32, 0);
                        close(game->player1_socket);
                    }
                    game->is_active = 0;
                    game_manager.active_games--;
                }
                else
                {
                    buffer[valread - 1] = '\0';
                    handle_move(game->player2_socket, &game_manager, buffer);
                }
            }
        }
    }
    cleanup_server();
    return 0;
}
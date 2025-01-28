import tkinter as tk
from tkinter import messagebox, ttk
import socket
from PIL import Image, ImageTk
import traceback
import os

class LoginWindow:
    def __init__(self):
        self.root = tk.Tk()
        self.root.title("Chess Game Connection")
        self.root.geometry("300x150")
        
        # Center the window
        self.root.eval('tk::PlaceWindow . center')
        
        # Create and pack widgets
        frame = ttk.Frame(self.root, padding="10")
        frame.grid(row=0, column=0, sticky=(tk.W, tk.E, tk.N, tk.S))
        
        # IP Address
        ttk.Label(frame, text="IP Address:").grid(row=0, column=0, sticky=tk.W, pady=5)
        self.ip_var = tk.StringVar(value="localhost")
        self.ip_entry = ttk.Entry(frame, textvariable=self.ip_var)
        self.ip_entry.grid(row=0, column=1, sticky=(tk.W, tk.E), pady=5)
        
        # Port
        ttk.Label(frame, text="Port:").grid(row=1, column=0, sticky=tk.W, pady=5)
        self.port_var = tk.StringVar(value="4567")
        self.port_entry = ttk.Entry(frame, textvariable=self.port_var)
        self.port_entry.grid(row=1, column=1, sticky=(tk.W, tk.E), pady=5)
        
        # Connect Button
        self.connect_btn = ttk.Button(frame, text="Connect", command=self.connect)
        self.connect_btn.grid(row=2, column=0, columnspan=2, pady=20)
        
        # Status Label
        self.status_var = tk.StringVar()
        self.status_label = ttk.Label(frame, textvariable=self.status_var, foreground="red")
        self.status_label.grid(row=3, column=0, columnspan=2)
        
        self.root.mainloop()

    def connect(self):
        try:
            ip = self.ip_var.get()
            port = int(self.port_var.get())
            
            # Disable connect button and show connecting status
            self.connect_btn.configure(state='disabled')
            self.status_var.set("Connecting...")
            self.root.update()
            
            print(f"Attempting to connect to {ip}:{port}")
            
            # Create TCP socket
            sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            sock.settimeout(10)  # 10 seconds timeout
            
            # Connect to server
            sock.connect((ip, port))
            print("Connected to server")
            
            # Try to receive initial message
            print("Waiting for server response...")
            init_msg = sock.recv(1024).decode()
            print(f"Received from server: {init_msg}")

            # If this is White player, wait for opponent
            if "Waiting for opponent" in init_msg:
                self.status_var.set("Waiting for opponent...")
                self.root.update()
                start_msg = sock.recv(1024).decode()
                print(f"Received from server: {start_msg}")

            # If connection successful, destroy login and start game
            self.root.destroy()
            game = ChessClient(sock, init_msg)
            
        except ValueError as e:
            error_msg = f"Invalid port number: {str(e)}"
            print(error_msg)
            self.status_var.set(error_msg)
            self.connect_btn.configure(state='normal')
        except socket.timeout as e:
            error_msg = "Connection timed out"
            print(error_msg)
            self.status_var.set(error_msg)
            self.connect_btn.configure(state='normal')
        except socket.error as e:
            error_msg = f"Connection failed: {str(e)}"
            print(error_msg)
            self.status_var.set(error_msg)
            self.connect_btn.configure(state='normal')
        except Exception as e:
            error_msg = f"Unexpected error: {str(e)}"
            print(error_msg)
            self.status_var.set(error_msg)
            self.connect_btn.configure(state='normal')

class ChessClient:
    def __init__(self, sock=None, init_msg=None):
        if sock is None:
            LoginWindow()
            return

        self.root = tk.Tk()
        self.root.title("Chess Game")

        # Use the connected socket
        self.sock = sock

        # Determine player color from initial message
        self.is_white = "White" in init_msg
        self.root.title(f"Chess Game - {'White' if self.is_white else 'Black'} Player")

        # Game state
        self.selected_square = None
        self.board = [['.' for _ in range(8)] for _ in range(8)]
        self.pieces = {}

        # Load pieces first
        if not self.load_pieces():
            return

        # Create the board
        self.create_board()

        # Set up initial position
        self.setup_initial_position()

        # Create status label
        self.status_var = tk.StringVar(value="Game starting...")
        self.status_label = ttk.Label(self.root, textvariable=self.status_var)
        self.status_label.grid(row=1, column=0, pady=5)

        # Start listening for server messages
        self.root.after(200, self.check_server_messages)

        # Handle window closing
        self.root.protocol("WM_DELETE_WINDOW", self.on_closing)

        self.root.mainloop()

    def setup_initial_position(self):
        # Initial position setup
        initial_position = [
            ['r', 'n', 'b', 'q', 'k', 'b', 'n', 'r'],  # Black back rank
            ['p', 'p', 'p', 'p', 'p', 'p', 'p', 'p'],  # Black pawns
            ['.', '.', '.', '.', '.', '.', '.', '.'],
            ['.', '.', '.', '.', '.', '.', '.', '.'],
            ['.', '.', '.', '.', '.', '.', '.', '.'],
            ['.', '.', '.', '.', '.', '.', '.', '.'],
            ['P', 'P', 'P', 'P', 'P', 'P', 'P', 'P'],  # White pawns
            ['R', 'N', 'B', 'Q', 'K', 'B', 'N', 'R']   # White back rank
        ]

        # If playing as black, flip the board
        if not self.is_white:
            initial_position = list(reversed(initial_position))

        # Update the board state and display
        for i in range(8):
            for j in range(8):
                piece = initial_position[i][j]
                self.board[i][j] = piece if piece != '.' else ''
                
                # Get display coordinates
                display_i = i if self.is_white else 7 - i
                display_j = j if self.is_white else 7 - j
                
                # Update square
                square = self.squares[(i, j)]
                if piece != '.' and piece in self.pieces:
                    square.configure(image=self.pieces[piece], compound='center')
                    square.image = self.pieces[piece]  # Keep a reference!
                else:
                    placeholder_image = ImageTk.PhotoImage(Image.new('RGBA', (60, 60), (255, 255, 255, 0)))
                    square.configure(image=placeholder_image)
                    square.image = placeholder_image  # Keep a reference!


    def create_board(self):
        self.squares = {}
        colors = ['white', 'gray']

        # Create main frame
        main_frame = ttk.Frame(self.root, padding="5")
        main_frame.grid(row=0, column=0)

        # Create board frame
        board_frame = ttk.Frame(main_frame, padding="5")
        board_frame.grid(row=0, column=0)

        for i in range(8):
            board_frame.grid_columnconfigure(i, weight=1)
            board_frame.grid_rowconfigure(i, weight=1)

        # for i in range(8):
        #     board_frame.grid_columnconfigure(i, weight=1)
        #     board_frame.grid_rowconfigure(i, weight=1)

        # Smaller square size
        square_size = 70  # Reduced from 60 to 40

        for row in range(8):
            for col in range(8):
                # Adjust coordinates based on player color
                display_row = row if self.is_white else 7 - row
                display_col = col if self.is_white else 7 - col
                
                color = colors[(row + col) % 2]
                # Create single label for both background and piece
                square = tk.Label(
                    board_frame,
                    # width=10,  # Reduced width in characters
                    # height=5,  # Reduced height in characters
                    bg=color,
                    relief='solid',
                    borderwidth=1
                )
                square.grid(row=display_row, column=display_col, sticky='nsew')
                
                # Bind click event to the whole square
                square.bind('<Button-1>', lambda e, r=row, c=col: self.square_clicked(r, c))
                self.squares[(row, col)] = square
                
                # Configure grid weights to maintain square shape
                board_frame.grid_columnconfigure(display_col, weight=1)
                board_frame.grid_rowconfigure(display_row, weight=1)

        # Add coordinate labels with smaller font
        label_font = ('TkDefaultFont', 8)
        for i in range(8):
            # Rank numbers (1-8)
            rank = str(8 - i) if self.is_white else str(i + 1)
            ttk.Label(board_frame, text=rank, font=label_font).grid(row=i, column=8, padx=2)
            
            # File letters (a-h)
            file = chr(ord('a') + i) if self.is_white else chr(ord('h') - i)
            ttk.Label(board_frame, text=file, font=label_font).grid(row=8, column=i, pady=2)

        # Create status label
        self.status_var = tk.StringVar(value="Game starting...")
        self.status_label = ttk.Label(main_frame, textvariable=self.status_var)
        self.status_label.grid(row=1, column=0, pady=5)

    def load_pieces(self):
        piece_files = {
            'P': 'white_pawn.png',
            'R': 'white_rook.png',
            'N': 'white_knight.png',
            'B': 'white_bishop.png',
            'Q': 'white_queen.png',
            'K': 'white_king.png',
            'p': 'black_pawn.png',
            'r': 'black_rook.png',
            'n': 'black_knight.png',
            'b': 'black_bishop.png',
            'q': 'black_queen.png',
            'k': 'black_king.png'
        }

        print("Starting to load pieces...")

        success = True
        for piece, file in piece_files.items():
            try:
                image_path = f"chess_pieces/{file}"
                print(f"Loading: {image_path}")
                
                image = Image.open(image_path)
                if image.mode != 'RGBA':
                    image = image.convert('RGBA')
                
                # Make pieces smaller (30x30 pixels)
                image = image.resize((60, 60))
                self.pieces[piece] = ImageTk.PhotoImage(image)
                print(f"Loaded {piece}")
            except Exception as e:
                print(f"Error loading {file}: {str(e)}")
                success = False
                break

        return success

    def update_board(self, board_rows):
        try:
            print("\nUpdating board with state:")
            for row in board_rows:
                print(row)
            
            # Update the board state
            for i in range(8):
                for j in range(8):
                    piece = board_rows[i][j]
                    
                    # Update internal board state
                    self.board[i][j] = piece if piece != '.' else ''
                    
                    # Get display coordinates based on player color
                    display_i = i if self.is_white else 7 - i
                    display_j = j if self.is_white else 7 - j
                    
                    # Update square
                    square = self.squares[(i, j)]
                    if piece != '.' and piece in self.pieces:
                        print(f"Placing piece '{piece}' at position ({i}, {j})")
                        # Use the same configuration that worked in the test
                        square.configure(
                            image=self.pieces[piece],
                            compound='center'
                        )
                        square.image = self.pieces[piece]  # Keep a reference
                        
                        # Reset background color to the correct chess square color
                        color = 'white' if (i + j) % 2 == 0 else 'gray'
                        square.configure(bg=color)
                    else:
                        placeholder_image = ImageTk.PhotoImage(Image.new('RGBA', (60, 60), (255, 255, 255, 0)))
                        square.configure(image=placeholder_image)
                        square.image = placeholder_image  # Keep a reference!

                        # Reset background color
                        color = 'white' if (i + j) % 2 == 0 else 'gray'
                        square.configure(bg=color)

        except Exception as e:
            print(f"Error updating board: {str(e)}")
            traceback.print_exc()

    def check_server_messages(self):
        try:
            self.sock.setblocking(0)
            msg = self.sock.recv(1024).decode()
            # print(f"Received message: {msg}")
            # print("fiut")
            
            # Handle multi-line messages
            messages = msg.strip().split('\n')
            
            # Look for board message
            for i, message in enumerate(messages):
                if message.startswith('board '):
                    # Get the next 8 lines that contain exactly 8 characters
                    board_rows = []
                    board_rows.append(message[6:])
                    for j in range(i + 1, len(messages)):
                        row = messages[j].strip()
                        if len(row) == 8 and all(c in '.RNBQKPrnbqkp' for c in row):
                            board_rows.append(row)
                            print("append")
                            print(row)
                            if len(board_rows) == 8:
                                break
                    
                    if len(board_rows) == 8:
                        print("Processing board state:")
                        for row in board_rows:
                            print(row)
                        self.update_board(board_rows)
                elif message.startswith('e '):
                    error_msg = message[2:]
                    print(f"Error: {error_msg}")
                    messagebox.showerror("Error", error_msg)
                elif message.startswith('x '):
                    msg = message[2:]
                    print(f"Game over: {msg}")
                    messagebox.showinfo("Game over", msg)
                    self.root.destroy()
                    return
                elif "Game #" in message and not message.startswith('board'):
                    self.status_var.set(message)
                
        except socket.error as e:
            if e.errno != 11:  # 11 is EAGAIN (no data available)
                print(f"Socket error: {e}")
                messagebox.showerror("Error", "Lost connection to server")
                self.root.destroy()
                return
        except Exception as e:
            print(f"Error: {str(e)}")
            traceback.print_exc()
        finally:
            if self.root.winfo_exists():
                self.root.after(300, self.check_server_messages)

    def square_clicked(self, row, col):
        if not self.selected_square:
            # First click - select piece
            if self.board[row][col]:
                self.selected_square = (row, col)
                self.squares[(row, col)].configure(bg='yellow')
        else:
            # Second click - make move
            from_row, from_col = self.selected_square
            # Clear previous selection
            orig_color = 'white' if (from_row + from_col) % 2 == 0 else 'gray'
            self.squares[self.selected_square].configure(bg=orig_color)
            
            # Convert to chess notation (a2a3 format)
            move = f"{chr(from_col + ord('a'))}{8-from_row}{chr(col + ord('a'))}{8-row}"
            
            # Validate move format
            if not self.validate_move_format(move):
                messagebox.showerror("Error", "Invalid move format")
                self.selected_square = None
                return
                
            print(f"Sending move: {move}")
            
            try:
                # Add newline to the move string
                move = move + "\n"
                bytes_sent = self.sock.send(move.encode('utf-8'))
                print(f"Sent {bytes_sent} bytes")
            except socket.error as e:
                print(f"Socket error while sending move: {e}")
                messagebox.showerror("Error", "Lost connection to server")
                self.root.destroy()
            
            self.selected_square = None

    def validate_move_format(self, move):
        """Validate that the move string is in the correct format (e.g., 'a2a3')"""
        if len(move) != 4:
            return False
        
        # Check format: letter(a-h), number(1-8), letter(a-h), number(1-8)
        valid_letters = 'abcdefgh'
        valid_numbers = '12345678'
        
        return (move[0] in valid_letters and
                move[1] in valid_numbers and
                move[2] in valid_letters and
                move[3] in valid_numbers)

    def test_piece_display(self):
        """Test function to display a single piece"""
        try:
            print("\nTesting piece display...")
            test_piece = 'K'  # White king
            test_position = (4, 4)  # Center of board
            
            if test_piece in self.pieces:
                print(f"Attempting to display {test_piece} at position {test_position}")
                square = self.squares[test_position]
                
                # Configure the square to display the piece
                square.configure(
                    image=self.pieces[test_piece],
                    compound='center'
                )
                square.image = self.pieces[test_piece]  # Keep a reference!
                
                # Update the internal board state
                self.board[test_position[0]][test_position[1]] = test_piece
                
                print("Test piece should now be visible")
                print(f"Square configuration: {square.configure()}")
                
                # Change background to highlight the test square
                square.configure(bg='yellow')
                
            else:
                print(f"Test piece {test_piece} not found in available pieces: {list(self.pieces.keys())}")
        except Exception as e:
            print(f"Error in test display: {str(e)}")
            traceback.print_exc()

    def on_closing(self):
        try:
            self.sock.close()
        except:
            pass
        self.root.destroy()

if __name__ == "__main__":
    ChessClient()
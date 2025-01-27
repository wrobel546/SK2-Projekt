#include <iostream>
#include <thread>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <arpa/inet.h>
#include <vector>

#define ROWS 6
#define COLS 7

class Game {
public:
    Game(int clientSocket1, int clientSocket2) : clientSocket1(clientSocket1), clientSocket2(clientSocket2), currentPlayer('X'), gameOver(false) {
        initializeBoard();
    }

    void initializeBoard() {
        board = std::vector<std::vector<char>>(ROWS, std::vector<char>(COLS, ' '));
    }

    bool makeMove(char col, char player, int len) {
        if (len > 1) return false;
        if (col < '0' || col >= '7') {
            return false;
        }
        int column = col - '0';
        for (int i = ROWS - 1; i >= 0; i--) {
            if (board[i][column] == ' ') {
                board[i][column] = player;
                return true;
            }
        }
        return false;
    }

    bool checkWin(char player) {
        // Horizontal check
        for (int i = 0; i < ROWS; i++) {
            for (int j = 0; j <= COLS - 4; j++) {
                if (board[i][j] == player && board[i][j + 1] == player &&
                    board[i][j + 2] == player && board[i][j + 3] == player) {
                    return true;
                }
            }
        }

        // Vertical check
        for (int i = 0; i <= ROWS - 4; i++) {
            for (int j = 0; j < COLS; j++) {
                if (board[i][j] == player && board[i + 1][j] == player &&
                    board[i + 2][j] == player && board[i + 3][j] == player) {
                    return true;
                }
            }
        }

        // Diagonal checks
        for (int i = 3; i < ROWS; i++) {
            for (int j = 0; j <= COLS - 4; j++) {
                if (board[i][j] == player && board[i - 1][j + 1] == player &&
                    board[i - 2][j + 2] == player && board[i - 3][j + 3] == player) {
                    return true;
                }
            }
        }

        for (int i = 0; i <= ROWS - 4; i++) {
            for (int j = 0; j <= COLS - 4; j++) {
                if (board[i][j] == player && board[i + 1][j + 1] == player &&
                    board[i + 2][j + 2] == player && board[i + 3][j + 3] == player) {
                    return true;
                }
            }
        }

        return false;
    }

    bool checkDraw() {
        for (int j = 0; j < COLS; j++) {
            if (board[0][j] == ' ') {
                return false;
            }
        }
        return true;
    }

    std::string getBoardAsString() {
        std::string boardStr;
        for (int i = 0; i < ROWS; i++) {
            for (int j = 0; j < COLS; j++) {
                boardStr += '|';
                boardStr += board[i][j];
            }
            boardStr += "|\n";
        }
        boardStr += "===============\n";
        boardStr += " 0 1 2 3 4 5 6 \n";
        return boardStr;
    }

    int clientSocket1, clientSocket2;
    char currentPlayer;
    bool gameOver;
    std::vector<std::vector<char>> board;
};

bool askPlayAgain(int clientSocket1, int clientSocket2) {
    char buffer[256];
    const char* prompt = " ";
    char response1, response2;

    write(clientSocket1, prompt, strlen(prompt));
    write(clientSocket2, prompt, strlen(prompt));

    // Odczytujemy odpowiedzi obu graczy
    read(clientSocket1, &response1, sizeof(response1));
    read(clientSocket2, &response2, sizeof(response2));

    // Jeśli obaj gracze odpowiedzą "t" (tak), gra jest kontynuowana
    if (response1 == 't' && response2 == 't') {
        return true;  // Zagraj ponownie
    }
    if (response1 == 'n'){
        const char* prompt = "Przeciwnik zrezygnował z gry. ";
        write(clientSocket2, prompt, strlen(prompt));
    }
    if (response2 == 'n'){
        const char* prompt = "Przeciwnik zrezygnował z gry. ";
        write(clientSocket1, prompt, strlen(prompt));
    }
    return false;  // Zakończ grę
}

bool isSocketConnected(int socket) {
    char buffer;
    int result = recv(socket, &buffer, sizeof(buffer), MSG_PEEK | MSG_DONTWAIT);
    if (result == 0) {
        // Klient rozłączył się
        return false;
    }
    if (result < 0 && (errno == EAGAIN || errno == EWOULDBLOCK)) {
        // Brak danych, ale połączenie jest aktywne
        return true;
    }
    if (result < 0) {
        // Błąd na gnieździe
        return false;
    }
    return true;
}

void handleGame(int clientSocket1, int clientSocket2) {
    bool gameOver = false;
    char buffer[256];
    int n;

    while (!gameOver) {
        Game game(clientSocket1, clientSocket2);
        char col;
        char buff[256];

        std::string boardStr = game.getBoardAsString();
        std::string clearScreen = "\033[H\033[J";

        write(clientSocket1, clearScreen.c_str(), clearScreen.size());
        write(clientSocket1, boardStr.c_str(), boardStr.size());
        write(clientSocket2, clearScreen.c_str(), clearScreen.size());
        write(clientSocket2, boardStr.c_str(), boardStr.size());

        while (!game.gameOver) {
            bzero(buff,256);
            boardStr = game.getBoardAsString();
            if (!isSocketConnected(clientSocket1)) {
                std::cerr << "Gracz 1 rozłączył się. Kończenie gry." << std::endl;
                const char* msg = "Gracz 1 opuscił rozgrywkę. Gra zostaje zakończona.";
                write(clientSocket2, msg, strlen(msg));
                close(clientSocket2);
                break;
            }

            if (!isSocketConnected(clientSocket2)) {
                std::cerr << "Gracz 2 rozłączył się. Kończenie gry." << std::endl;
                const char* msg = "Gracz 2 opuscił rozgrywkę. Gra zostaje zakończona.";
                write(clientSocket1, msg, strlen(msg));
                close(clientSocket1);
                break;
            }
            
            write(clientSocket1, clearScreen.c_str(), clearScreen.size());
            write(clientSocket1, boardStr.c_str(), boardStr.size());
            write(clientSocket2, clearScreen.c_str(), clearScreen.size());
            write(clientSocket2, boardStr.c_str(), boardStr.size());


            if (game.currentPlayer == 'X') {
                write(clientSocket1, "Twoja tura! Wybierz kolumnę (0-6): ", 36);
                int n = read(clientSocket1, &buff, sizeof(buff));
                if ( n <= 0) {
                    std::cerr << "Gracz 1 rozłączony. Kończenie gry." << std::endl;
                    // Gracz 1 się rozłączył - wysyłamy wiadomość do gracza 2
                    const char* msg = "Gracz 1 opuscił rozgrywkę. Gra zostaje zakończona.";
                    write(clientSocket2, msg, strlen(msg));
                    // Czekamy 3 sekundy, aby gracz 2 otrzymał wiadomość
                    sleep(3);
                    // Rozłączamy się z graczem 2
                    close(clientSocket2);
                    game.gameOver = true;
                    break;  // Zakończ grę\

                }
            } else {
                write(clientSocket2, "Twoja tura! Wybierz kolumnę (0-6): ", 36);
                n = read(clientSocket2, &buff, sizeof(buff));
                if (n <= 0) {
                    std::cerr << "Gracz 2 rozłączony. Kończenie gry." << std::endl;
                    // Gracz 2 się rozłączył - wysyłamy wiadomość do gracza 1
                    const char* msg = "Gracz 2 opuscił rozgrywkę. Gra zostaje zakończona.";
                    write(clientSocket1, msg, strlen(msg));
                    // Czekamy 3 sekundy, aby gracz 1 otrzymał wiadomość
                    sleep(3);
                    // Rozłączamy się z graczem 1
                    close(clientSocket1);
                    game.gameOver = true;
                    break;  // Zakończ grę
                }
            }
            while(true){
                col = buff[0];
                //std::cout<<strlen(buff)<<'\n';
                int len = strlen(buff);
                if (game.makeMove(col, game.currentPlayer, len)) {
                    std::string updatedBoardStr = game.getBoardAsString();
                    if (!isSocketConnected(clientSocket1)) {
                        std::cerr << "Gracz 1 rozłączył się. Kończenie gry." << std::endl;
                        const char* msg = "Gracz 1 opuscił rozgrywkę. Gra zostaje zakończona.";
                        write(clientSocket2, msg, strlen(msg));
                        close(clientSocket2);
                        break;
                    }

                    if (!isSocketConnected(clientSocket2)) {
                        std::cerr << "Gracz 2 rozłączył się. Kończenie gry." << std::endl;
                        const char* msg = "Gracz 2 opuscił rozgrywkę. Gra zostaje zakończona.";
                        write(clientSocket1, msg, strlen(msg));
                        close(clientSocket1);
                        break;
                    }

                    write(clientSocket1, clearScreen.c_str(), clearScreen.size());
                    write(clientSocket1, updatedBoardStr.c_str(), updatedBoardStr.size());
                    write(clientSocket2, clearScreen.c_str(), clearScreen.size());
                    write(clientSocket2, updatedBoardStr.c_str(), updatedBoardStr.size());

                    
                    if (game.checkWin(game.currentPlayer)) {
                        snprintf(buffer, sizeof(buffer), "Gracz %c wygrywa! Czy chcesz zagrac ponownie? (t/n): \n", game.currentPlayer);
                        write(clientSocket1, buffer, strlen(buffer));
                        write(clientSocket2, buffer, strlen(buffer));
                        game.gameOver = true;
                    } else if (game.checkDraw()) {
                        snprintf(buffer, sizeof(buffer), "Remis! Czy chcesz zagrac ponownie? (t/n): \n");
                        write(clientSocket1, buffer, strlen(buffer));
                        write(clientSocket2, buffer, strlen(buffer));
                        game.gameOver = true;
                    } else {
                        game.currentPlayer = (game.currentPlayer == 'X') ? 'O' : 'X';
                    }
                    break;
                } else {
                    snprintf(buffer, sizeof(buffer), "Nieprawidłowy ruch, spróbuj ponownie. Wybierz kolumnę (0-6): \n");
                    if(game.currentPlayer == 'X'){
                        write(clientSocket1, buffer, strlen(buffer));
                        int n = read(clientSocket1, &buff, sizeof(buff));
                        if (n <= 0) {
                            std::cerr << "Gracz 1 rozłączony. Kończenie gry." << std::endl;
                            // Gracz 2 się rozłączył - wysyłamy wiadomość do gracza 1
                            const char* msg = "Gracz 2 opuscił rozgrywkę. Gra zostaje zakończona.";
                            write(clientSocket2, msg, strlen(msg));
                            // Czekamy 3 sekundy, aby gracz 1 otrzymał wiadomość
                            sleep(3);
                            // Rozłączamy się z graczem 1
                            close(clientSocket2);
                            game.gameOver = true;
                            break;  // Zakończ grę
                        }
                        //std::cout << col <<'\n';
                    }
                    else{ 
                        write(clientSocket2, buffer, strlen(buffer));
                        int n = read(clientSocket2, &buff, sizeof(buff));
                        if (n <= 0) {
                            std::cerr << "Gracz 2 rozłączony. Kończenie gry." << std::endl;
                            // Gracz 2 się rozłączył - wysyłamy wiadomość do gracza 1
                            const char* msg = "Gracz 2 opuscił rozgrywkę. Gra zostaje zakończona.";
                            write(clientSocket1, msg, strlen(msg));
                            // Czekamy 3 sekundy, aby gracz 1 otrzymał wiadomość
                            sleep(3);
                            // Rozłączamy się z graczem 1
                            close(clientSocket1);
                            game.gameOver = true;
                            break;  // Zakończ grę
                        }
                        //std::cout << col <<'\n';
                       
                    }
                }
            }
            
        }

        if (!askPlayAgain(clientSocket1, clientSocket2)) {
            break;  // Zakończ grę, jeśli gracze nie chcą grać ponownie
        }
        
    }

    close(clientSocket1);
    close(clientSocket2);
}

int main() {
    int serverSocket, clientSocket1, clientSocket2;
    struct sockaddr_in serverAddr, clientAddr;
    socklen_t addrSize;
    
    serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSocket < 0) {
        std::cerr << "Błąd podczas tworzenia gniazda!" << std::endl;
        return 1;
    }

    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(3333);
    serverAddr.sin_addr.s_addr = INADDR_ANY;

    if (bind(serverSocket, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) < 0) {
        std::cerr << "Błąd podczas wiązania!" << std::endl;
        return 1;
    }

    listen(serverSocket, 10);
    std::cout << "Serwer uruchomiony na porcie 1234" << std::endl;
    while(true){
        while (true) {
            addrSize = sizeof(clientAddr);
            clientSocket1 = accept(serverSocket, (struct sockaddr*)&clientAddr, &addrSize);
            if (clientSocket1 < 0) {
                std::cerr << "Błąd podczas akceptowania połączenia!" << std::endl;
                continue;
            }

            clientSocket2 = accept(serverSocket, (struct sockaddr*)&clientAddr, &addrSize);
            if (clientSocket2 < 0) {
                std::cerr << "Błąd podczas akceptowania połączenia!" << std::endl;
                continue;
            }

            std::thread gameThread(handleGame, clientSocket1, clientSocket2);
            gameThread.detach();
        }
    }
    close(serverSocket);
    return 0;
}

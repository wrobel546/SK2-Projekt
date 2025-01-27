#include <iostream>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <arpa/inet.h>

int main(int argc, char* argv[]) {
    int my_socket;
    struct sockaddr_in server_addr;

    if (argc != 3) {
        std::cerr << "Użycie: " << argv[0] << " <adres_serwera> <port>" << std::endl;
        return 1;
    }

    my_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (my_socket < 0) {
        std::cerr << "Błąd podczas tworzenia gniazda!" << std::endl;
        return 1;
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(3333);
    server_addr.sin_addr.s_addr = inet_addr(argv[1]);

    if (connect(my_socket, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        std::cerr << "Błąd połączenia z serwerem!" << std::endl;
        return 1;
    }

    char buffer[256];
    while (true) {
        memset(buffer, 0, sizeof(buffer));
        int n = read(my_socket, buffer, sizeof(buffer));
        
        if (n < 0) {
            std::cerr << "Błąd odczytu!" << std::endl;
            break;
        }
        
        if (n == 0) {
            //std::cerr << "Połączenie zostało zamknięte przez serwer. Najprawdopodobniej twój przeciwnik opuscił rozgrywkę." << std::endl;
            sleep(2);
            break;  // Koniec gry, rozłączenie z serwerem
        }
        
        
        std::cout << buffer;

        if (strstr(buffer, "Twoja tura!") != NULL) {
            char col[256];
            bzero(col,256);
            std::cin >> col;
            write(my_socket, &col, sizeof(col));
            
            
        }

        if (strstr(buffer, "Nieprawidłowy")!= NULL){
            char col[256];
            bzero(col,256);
            std::cin >> col;
            write(my_socket, &col, sizeof(col));
        }

        // Sprawdzamy, czy gra się skończyła
        if (strstr(buffer, "wygrywa!") != NULL || strstr(buffer, "Remis!") != NULL) {
            char response;
            //std::cout << "Czy chcesz zagrać ponownie? (t/n): ";
            std::cin >> response;
            write(my_socket, &response, sizeof(response));
            if (response == 'n') {
                break;
            }
        }

    }

    close(my_socket);
    return 0;
}

#include <iostream>
#include <cstring>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <map>

const int BUFFER_SIZE = 1024;

std::map<std::string, std::string> KV_DATASTORE;

void handle_client(int client_socket) {
    char buffer[BUFFER_SIZE];
    ssize_t bytes_received;

    while ((bytes_received = recv(client_socket, buffer, BUFFER_SIZE, 0)) > 0) {
        buffer[bytes_received] = '\0';

        char *token = strtok(buffer, "\n");
        while(token != NULL) {
            if (strcmp(token, "WRITE") == 0) {
                char *key = strtok(NULL, "\n");
                char *value = strtok(NULL, "\n") + 1;

                if (key != NULL && value != NULL) {
                    KV_DATASTORE[key] = value;
                    send(client_socket, "FIN\n", 4, 0);
                } else {
                    send(client_socket, "ERROR\n", 6, 0);
                    token = NULL;
                    close(client_socket);

                }
            } else if (strcmp(token, "READ") == 0) {
                char *key = strtok(NULL, "\n");
                if (key != NULL) {
                    auto it = KV_DATASTORE.find(key);
                    if (it != KV_DATASTORE.end()) {
                        send(client_socket, (it->second + "\n").c_str(), it->second.size() + 1, 0);
                    } else {
                        send(client_socket, "NULL\n", 5, 0);
                    }
                } else {
                    send(client_socket, "ERROR\n", 6, 0);
                    token = NULL;
                    close(client_socket);
                }
            } else if (strcmp(token, "DELETE") == 0) {
                char *key = strtok(NULL, "\n");
                if (key != NULL) {
                    size_t erased = KV_DATASTORE.erase(key);
                    if (erased > 0) {
                        send(client_socket, "FIN\n", 4, 0);
                    } else {
                        send(client_socket, "NULL\n", 5, 0);
                    }
                } else {
                    send(client_socket, "ERROR\n", 6, 0);
                    token = NULL;
                    close(client_socket);
                }
            } else if (strcmp(token, "COUNT") == 0) {
                send(client_socket, (std::to_string(KV_DATASTORE.size()) + "\n").c_str(), std::to_string(KV_DATASTORE.size()).size() + 1, 0);
            } else if (strcmp(token, "END") == 0) {
                send(client_socket, "\n", 1, 0);
                close(client_socket);
                break;
            } else {
                send(client_socket, "INVALID\n", 8, 0);
                token = NULL;
                close(client_socket);
            }
            token = strtok(NULL, "\n");
        }
    }

    close(client_socket);
}

int main(int argc, char *argv[]) {
    int server_socket, client_socket;
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_len = sizeof(client_addr);
    int portno;

    if (argc != 2) {
        std::cerr << "Usage: " << argv[0] << " <port>" << std::endl;
        return 1;
    }

    portno = atoi(argv[1]);

    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket < 0) {
        std::cerr << "Error opening socket" << std::endl;
        return 1;
    }
    
    int reuse_addr=1;

    if (setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, &reuse_addr, sizeof(int)) < 0) {
        std::cerr << "Error setting socket option" << std::endl;
        return 1;
    }

    memset((char *)&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(portno);

    if (bind(server_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        std::cerr << "Error on binding" << std::endl;
        return 1;
    }

    listen(server_socket, 5);
    std::cout << "Server listening on port " << portno << "..." << std::endl;

    while (true) {
        client_socket = accept(server_socket, (struct sockaddr *)&client_addr, &client_len);
        if (client_socket < 0) {
            std::cerr << "Error on accept" << std::endl;
            continue;
        }

        handle_client(client_socket);
    }

    return 0;
}

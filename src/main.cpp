#include <iostream>
#include <cstdlib>
#include <string>
#include <cstring>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <vector>


// I implement a event loop but if you want you can use 
// threads for the same job in a different way.
void handle_client(int client_fd) {
  char buffer[1024];

   while (true) {
        memset(buffer, 0, sizeof(buffer));
        int n = read(client_fd, buffer, sizeof(buffer));

        if (n <= 0) {
            close(client_fd);
            return;  
        }

        std::string req(buffer);
        if (req.find("PING") != std::string::npos) {
            std::string resp = "+PONG\r\n";
            write(client_fd, resp.c_str(), resp.size());
        }
    }
}

int parseInteherUntilCRLF(const std::string& s, int& pos) {
  int start = pos;
  while(s[pos] != '\r') pos++;
  int number = std::stoi(s.substr(start, pos - start));
  pos += 2;
  return number;
}

void skipCRLF(const std::string& s, int& pos) {
  if(s[pos] == '\r' && s[pos+1] == '\n') {
    pos += 2;
  }
}

std::vector<std::string> parser(char* buffer) {
  std::string data(buffer);
  std::vector<std::string> values;

  int pos = 0;

  if(data[pos] != '*') return{};
  pos++;

  int len = parseInteherUntilCRLF(data, pos);

  for(int i = 0;i < len; i++) {
    if(data[pos] != '$') return {};
    pos++;
    int word_len = parseInteherUntilCRLF(data, pos);

    std::string word = data.substr(pos, word_len);
    pos += word_len;

    values.push_back(word);
    skipCRLF(data, pos);
  }

  return values;

}


int main(int argc, char **argv) {
  // Flush after every std::cout / std::cerr
  std::cout << std::unitbuf;
  std::cerr << std::unitbuf;
  
  // Creates a new socket in Linux Kernel
  int server_fd = socket(AF_INET, SOCK_STREAM, 0);
  if (server_fd < 0) {
   std::cerr << "Failed to create server socket\n";
   return 1;
  }
  
  // Since the tester restarts your program quite often, setting SO_REUSEADDR
  // ensures that we don't run into 'Address already in use' errors
  int reuse = 1;
  if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse)) < 0) {
    std::cerr << "setsockopt failed\n";
    return 1;
  }
  
  struct sockaddr_in server_addr;
  server_addr.sin_family = AF_INET;
  server_addr.sin_addr.s_addr = INADDR_ANY;
  server_addr.sin_port = htons(6379);
  
  if (bind(server_fd, (struct sockaddr *) &server_addr, sizeof(server_addr)) != 0) {
    std::cerr << "Failed to bind to port 6379\n";
    return 1;
  }
  
  int connection_backlog = 5;
  if (listen(server_fd, connection_backlog) != 0) {
    std::cerr << "listen failed\n";
    return 1;
  }
  
  struct sockaddr_in client_addr;
  int client_addr_len = sizeof(client_addr);
  std::cout << "Waiting for a client to connect...\n";

  // You can use print statements as follows for debugging, they'll be visible when running tests.
  std::cout << "Logs from your program will appear here!\n";
  

  // --------------------------------------------------
  // -------------------- MY CODE ---------------------
  // --------------------------------------------------

  char buffer[1024];


  fd_set current_fds;
  FD_ZERO(&current_fds);
  FD_SET(server_fd, &current_fds);

  int max_fd = server_fd;

  while(true){
    fd_set ready_fds = current_fds;

    int activity = select(max_fd + 1, &ready_fds, NULL, NULL, NULL);
    if(activity < 0) {
      std::cerr << "select error\n";
      break;
    }

    if(FD_ISSET(server_fd, &ready_fds)) {
      int new_client = accept(server_fd, (struct sockaddr*)&client_addr, (socklen_t*)&client_addr_len);
      std::cout << "New client connected: fd = " << new_client << "\n";


      FD_SET(new_client, &current_fds);
      if(new_client > max_fd) {
        max_fd = new_client;
      }
    }

    for(int fd = 0; fd <= max_fd; fd++) {
      if(fd != server_fd && FD_ISSET(fd, &ready_fds)) {

        memset(buffer, 0, sizeof(buffer));
        int bytes_read = read(fd, buffer, sizeof(buffer));

        if(bytes_read <= 0) {
          std::cout << "Client disconnected: fd = " << fd << "\n";
          close(fd);
          FD_CLR(fd, &current_fds);
        } else {
          std::string request(buffer);
          if(request.find("PING") != std::string::npos) {
            std::string responde("+PONG\r\n");
            write(fd, responde.c_str(), responde.size());
          } else {
            std::vector<std::string> words = parser(buffer);
            if(words[0] == "ECHO") {
              std::string value = "$" + std::to_string(words[1].size()) + "\r\n" + words[1] + "\r\n";
              write(fd, value.c_str(), value.size());
            }
          }
        }
      }
    }
  }
  

  // --------------------------------------------------
  // --------------- MY CODE ENDS HERE ----------------
  // --------------------------------------------------
  
  close(server_fd);

  return 0;
}

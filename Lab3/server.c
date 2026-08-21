#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define PORT 8080
#define MAX 100

// Structure to send all results back to the client
struct Statistics {
  int min;
  int max;
  int sum;
  float avg;
};

int main() {
  int server_fd, new_socket;
  struct sockaddr_in address;
  socklen_t addrlen = sizeof(address);

  // 1. Socket creation
  server_fd = socket(AF_INET, SOCK_STREAM, 0);
  if (server_fd == -1) {
    perror("Socket creation failed");
    exit(EXIT_FAILURE);
  }

  // 2. Server address setup
  address.sin_family = AF_INET;
  address.sin_addr.s_addr = INADDR_ANY;
  address.sin_port = htons(PORT);

  // 3. Bind socket to port
  if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
    perror("Bind failed");
    close(server_fd);
    exit(EXIT_FAILURE);
  }

  // 4. Listen for incoming connections
  if (listen(server_fd, 5) < 0) {
    perror("Listen failed");
    close(server_fd);
    exit(EXIT_FAILURE);
  }

  printf("Server Listening on Port %d...\n", PORT);

  // 5. Keep accepting new client connections in a loop
  while (1) {
    new_socket = accept(server_fd, (struct sockaddr *)&address, &addrlen);
    if (new_socket < 0) {
      perror("Accept failed");
      continue;
    }

    printf("\nConnected to client: IP = %s, Port = %d\n",
           inet_ntoa(address.sin_addr), ntohs(address.sin_port));

    int n;
    int arr[MAX];

    if (recv(new_socket, &n, sizeof(n), 0) <= 0) {
      printf("Client disconnected.\n");
      close(new_socket);
      continue;
    }

    recv(new_socket, arr, sizeof(int) * n, 0);

    printf("Integers received from client: ");
    for (int i = 0; i < n; i++) {
      printf("%d ", arr[i]);
    }
    printf("\n");

    struct Statistics result;
    result.min = arr[0];
    result.max = arr[0];
    result.sum = 0;

    for (int i = 0; i < n; i++) {
      if (arr[i] < result.min)
        result.min = arr[i];
      if (arr[i] > result.max)
        result.max = arr[i];
      result.sum += arr[i];
    }
    result.avg = (float)result.sum / n;

    printf("Computed: Minimum = %d, Maximum = %d, Sum = %d, Average = %.2f\n",
           result.min, result.max, result.sum, result.avg);

    send(new_socket, &result, sizeof(result), 0);

    close(new_socket);
    printf("Client disconnected.\n");
  }

  close(server_fd);
  return 0;
}
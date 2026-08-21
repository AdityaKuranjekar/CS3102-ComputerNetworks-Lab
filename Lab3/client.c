#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define MAX 100

// Structure to receive results from server
struct Statistics {
  int min;
  int max;
  int sum;
  float avg;
};

int main() {
  int sock;
  struct sockaddr_in server_address;
  char ip[50];
  int port, n;
  int arr[MAX];
  struct Statistics result;

  // 1. Get connection details from user
  printf("Enter Server IP address: ");
  scanf("%s", ip);

  printf("Enter Server Port number: ");
  scanf("%d", &port);

  // 2. Socket creation
  sock = socket(AF_INET, SOCK_STREAM, 0);
  if (sock < 0) {
    perror("Socket creation failed");
    exit(EXIT_FAILURE);
  }

  // 3. Server address setup
  server_address.sin_family = AF_INET;
  server_address.sin_port = htons(port);

  // Convert IPv4 and IPv6 addresses from text to binary form
  if (inet_pton(AF_INET, ip, &server_address.sin_addr) <= 0) {
    perror("Invalid address / Address not supported");
    exit(EXIT_FAILURE);
  }

  // 4. Connect to server
  if (connect(sock, (struct sockaddr *)&server_address,
              sizeof(server_address)) < 0) {
    perror("Connection failed");
    exit(EXIT_FAILURE);
  }

  printf("Connected to server.\n\n");

  // 5. Read integers from user
  printf("Enter number of integers N: ");
  scanf("%d", &n);

  printf("Enter %d values: ", n);
  for (int i = 0; i < n; i++) {
    scanf("%d", &arr[i]);
  }

  // 6. Send N and the array to server
  send(sock, &n, sizeof(n), 0);
  send(sock, arr, sizeof(int) * n, 0);

  // 7. Receive computed results
  recv(sock, &result, sizeof(result), 0);

  // 8. Display output
  printf("\nMinimum = %d\n", result.min);
  printf("Maximum = %d\n", result.max);
  printf("Sum = %d\n", result.sum);
  printf("Average = %.2f\n", result.avg);

  // 9. Close socket
  close(sock);
  return 0;
}
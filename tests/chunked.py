import socket
import time

# Replace with your server URL (including protocol and port)
SERVER_ADDRESS = ("localhost", 400)


# Data to be sent in chunks
data = "This is some data to be sent in chunks using a chunked POST request."
# Define chunk size
CHUNK_SIZE = 1024
# Variable to control wait time between chunks (in seconds)
WAIT_TIME = 1  # Adjust for your desired delay


data_bytes = data.encode()
def send_chunked_request(data, server_address, chunk_size, wait_time):
  """Sends a chunked POST request to the specified server address with the given data.

  Args:
      data: The data to be sent in chunks (bytes).
      server_address: A tuple containing server IP address and port.
      chunk_size: The size of each chunk in bytes.
      wait_time: The time to wait between sending chunks (in seconds).
  """

  with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as sock:
    sock.connect(server_address)

    # Send initial request line (hardcoded for POST method)
    request_line = f"POST /pages/upload HTTP/1.1\r\n"  # Replace "/path/to/resource" with your actual path
    sock.sendall(request_line.encode())

    # Send headers (hardcoded for basic headers)
    headers = f"Host: {server_address[0]}\r\nTransfer-Encoding: chunked\r\nContent-Type: text/plain\r\n\r\n"
    sock.sendall(headers.encode())

    while data:
      chunk = data[:chunk_size]
      data = data[chunk_size:]

      chunk_hex = hex(len(chunk))[2:].upper()  # Convert chunk size to hex string (uppercase)
      chunk_data = f"{chunk_hex}\r\n{chunk.decode()}\r\n"  # Prepend chunk size + CRLF

      sock.sendall(chunk_data.encode())

      # Wait between chunks (skip if wait_time is zero)
      if wait_time > 0:
          time.sleep(wait_time)

    # Send final empty chunk
    sock.sendall(b"0\r\n\r\n")

    # Receive server response (optional processing here)
    response = sock.recv(1024).decode()
    print(f"Server response:\n{response}")

if __name__ == "__main__":
  send_chunked_request(data_bytes, SERVER_ADDRESS, CHUNK_SIZE, WAIT_TIME)





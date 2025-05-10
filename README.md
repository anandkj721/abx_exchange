# ABX Exchange Client

A C++ client application for the ABX mock exchange server. This client connects to the server, requests stock ticker data, handles missing packet sequences, and generates a JSON file (`output.json`) with all packets in sequence order.

## Prerequisites
- **Node.js**: Version 16.17.0 or higher
- **MinGW-w64**: For compiling C++ (e.g., via MSYS2)
- **json.hpp**: Nlohmann JSON library (download from https://github.com/nlohmann/json/releases/download/v3.11.3/json.hpp)

## Setup
1. Unzip `abx_exchange_server.zip` and navigate to the folder:
 cd path\to\abx_exchange_server node main.js

This starts the server on `localhost:3000`.
2. Place `client.cpp` and `json.hpp` in a folder (e.g., `abx_exchange`).
3. Compile the client:
 g++ -o client client.cpp -std=c++11 -lws2_32

4. Run the client:
   client.exe

5. Check `output.json` for the generated results.

## Files
- `client.cpp`: Main C++ client code.
- `json.hpp`: JSON library (download from link above).
- `output.json`: Generated output (not included in repo).

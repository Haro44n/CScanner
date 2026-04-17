CScanner PRO
CScanner PRO is a high-performance OSINT tool written in C++. It’s a native, compiled, and heavily multi-threaded alternative to the original Python-based Sherlock.

The main goal of this project is to eliminate the bottlenecks of the Python interpreter (GIL) and provide maximum scanning speed through hardware-level concurrency.

====Performance vs Sherlock====
C++ Native: No interpreter overhead. Faster execution and lower CPU usage.

True Concurrency: Uses 60+ independent threads (pthreads) for simultaneous requests.

Low Memory Footprint: Efficiently handles thousands of targets without RAM bloat.

Smart Detection: Implements Sherlock's logic (Status Code + String match) but optimized for high-throughput I/O.

====Prerequisites====
The project depends on:

libcurl - for network operations.

nlohmann/json - for site database parsing.

Installation (Linux)
Bash
sudo apt-get install libcurl4-openssl-dev nlohmann-json3-dev
====Build====
Use g++ with -O3 optimization flag for best performance:

Bash
g++ -O3 -std=c++17 main.cpp -o cscanner -lcurl -lpthread
====Configuration====
The scanner uses a data.json file. The format is fully compatible with the Sherlock database:

JSON
{
  "GitHub": {
    "url": "https://github.com/{}",
    "errorType": "status_code"
  },
  "Instagram": {
    "url": "https://www.instagram.com/{}/",
    "errorType": "message",
    "errorMsg": "Sorry, this page isn't available."
  }
}
====Usage====
Make sure data.json is in the same directory.

Run the binary: ./cscanner
Enter the nickname when prompted.

⚠️ Disclaimer ⚠️
This tool is for educational purposes and authorized security auditing only. Don't be a jerk. Respect the Terms of Service of the platforms you scan.

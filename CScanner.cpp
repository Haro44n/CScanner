#include <iostream>
#include <string>
#include <vector>
#include <fstream>
#include <thread>
#include <mutex>
#include <atomic>
#include <curl/curl.h>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

struct SiteConfig {
    std::string name;
    std::string urlTemplate;
    std::string errorType;
    json errorMsg;
};

std::mutex mtx;
std::atomic<int> completed_count(0);
int total_sites = 0;

size_t WriteCallback(void* contents, size_t size, size_t nmemb, std::string* userp) {
    userp->append((char*)contents, size * nmemb);
    return size * nmemb;
}

bool check_site(const SiteConfig& site, const std::string& nickname) {
    CURL* curl = curl_easy_init();
    if (!curl) return false;

    std::string fullUrl = site.urlTemplate;
    size_t pos = fullUrl.find("{}");
    if (pos != std::string::npos) fullUrl.replace(pos, 2, nickname);

    std::string readBuffer;
    curl_easy_setopt(curl, CURLOPT_URL, fullUrl.c_str());
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 8L);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);

    curl_easy_setopt(curl, CURLOPT_USERAGENT, "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/119.0.0.0 Safari/537.36");
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);

    if (site.errorType == "status_code") {
        curl_easy_setopt(curl, CURLOPT_NOBODY, 1L);
    }

    CURLcode res = curl_easy_perform(curl);
    long response_code;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &response_code);
    curl_easy_cleanup(curl);

    if (res != CURLE_OK) return false;

    if (site.errorType == "status_code") {
        return (response_code == 200);
    }
    else if (site.errorType == "message") {
        if (response_code != 200) return false;
        if (site.errorMsg.is_string()) {
            return (readBuffer.find(site.errorMsg.get<std::string>()) == std::string::npos);
        }
        else if (site.errorMsg.is_array()) {
            for (auto& msg : site.errorMsg) {
                if (readBuffer.find(msg.get<std::string>()) != std::string::npos) return false;
            }
            return true;
        }
    }

    return (response_code == 200);
}

void scanner_worker(std::string nickname, const std::vector<SiteConfig>& sites, int start, int end) {
    for (int i = start; i < end; ++i) {
        bool found = check_site(sites[i], nickname);

        completed_count++;

        if (found) {
            std::lock_guard<std::mutex> lock(mtx);

            std::string printUrl = sites[i].urlTemplate;
            size_t pos = printUrl.find("{}");
            if (pos != std::string::npos) printUrl.replace(pos, 2, nickname);

            std::cout << "\r[+] [FOUND] " << sites[i].name << " | " << printUrl << "        " << std::endl;
        }

        if (completed_count % 5 == 0) {
            std::lock_guard<std::mutex> lock(mtx);
            std::cout << "\r[*] Progress: " << completed_count << "/" << total_sites << " [" << (completed_count * 100 / total_sites) << "%]   " << std::flush;
        }
    }
}

int main() {
    curl_global_init(CURL_GLOBAL_ALL);

    std::string nickname;
    std::cout << "================================================\n";
    std::cout << "   CScanner \n";
    std::cout << "================================================\n";
    std::cout << "[?] Nickname to analyze: ";
    std::cin >> nickname;

    std::ifstream file("data.json");
    if (!file.is_open()) {
        std::cerr << "[!] ERROR: Place data.json in the program folder!\n";
        return 1;
    }

    json data;
    file >> data;

    std::vector<SiteConfig> sites;
    for (auto& it : data.items()) {
        if (it.key().find("$") == 0) continue;

        SiteConfig s;
        s.name = it.key();
        s.urlTemplate = it.value().value("url", "");
        s.errorType = it.value().value("errorType", "status_code");
        s.errorMsg = it.value()["errorMsg"];

        if (!s.urlTemplate.empty()) sites.push_back(s);
    }

    total_sites = (int)sites.size();
    std::cout << "[*] Database loaded: " << total_sites << " platforms.\n";
    std::cout << "[*] Starting...\n\n";

    int num_threads = 60;
    std::vector<std::thread> threads;
    int chunk = total_sites / num_threads;

    for (int i = 0; i < num_threads; ++i) {
        int start = i * chunk;
        int end = (i == num_threads - 1) ? total_sites : (i + 1) * chunk;
        threads.push_back(std::thread(scanner_worker, nickname, std::ref(sites), start, end));
    }

    for (auto& t : threads) t.join();

    std::cout << "\n\n================================================\n";
    std::cout << "[*] Done! Checked " << total_sites << " sites.\n";

    curl_global_cleanup();
    system("pause");
    return 0;
}
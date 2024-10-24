#include "cli.hpp"

#include <iostream>
#include <fstream>
#include <sstream>
#include <signal.h>
#include <unistd.h>
#include <sys/time.h>
#include <fcntl.h>
#include <cstring>

// How many times it will loop to try to read activity time
#define MAX_ATTEMPTS 5

// How long it will wait between activity time reads
#define SLEEP_INTERVAL_SECONDS 1

#define PRINT_BORDER \
    "**********************************************"

Cli::Cli() {
    _activePath = "/Users/Shared/activity.count";
    _pidPath = "/Users/Shared/activity.pid";
    _activePathModTime = getModifiedTime();
    sendSigHup();
    install_one_off_watch();
    print();
}

Cli::~Cli() {
}

std::string Cli::readActiveTime() {
    std::ifstream in(_activePath.c_str(), std::ios_base::binary);
    if (!in) {
        std::cerr << "Failed to open active time file." << std::endl;
        return "";
    }
    std::stringstream ss;
    ss << in.rdbuf();
    in.close();
    return ss.str();
}

void Cli::sendSigHup() {
    pid_t pid;
    std::stringstream ss;
    std::ifstream in(_pidPath, std::ios_base::binary);
    if (!in) {
        std::cerr << "Failed to open pid file." << std::endl;
        return;
    }
    ss << in.rdbuf();
    in.close();
    pid = std::stoul(ss.str());
    kill(pid, SIGHUP);
}

void Cli::print() {
    static std::vector<std::string> weekDays = {"S", "M", "T", "W", "T", "F", "S"};
    std::string activeTime = readActiveTime();
    std::string number = "";
    unsigned long long num = 0;
    unsigned long long totalActiveMinutes = 0;
    std::vector<unsigned long long> nums;
    std::vector<std::string> weekCounts;
    std::vector<std::string> lines;
    std::string line;
    if (activeTime == "") {
        std::cerr << "Failed to fetch active time";
        return;
    }
    for (const char & c : activeTime) {
        if (c != ' ') {
            number += c;
        } else {
            if (number != "") {
                num = std::stoull(number);
                nums.push_back(num);
                totalActiveMinutes += num;
                number = "";
                num = 0;
            }
        }
    }
    for (unsigned long long & activeTime: nums) {
        weekCounts.push_back(minutesToHour(activeTime));
    }
    lines.push_back("Weekly total hours: " + minutesToHour(totalActiveMinutes));
    lines.push_back("Weekly breakdown hours:");
    for (const auto & entry : weekCounts) {
        line += entry + "|";
    }
    lines.push_back(line);
    line.clear();

    for (auto & c: weekDays) {
        line += c + "   |";
    }
    lines.push_back(line);
    wrap_lines(lines);
    for (const auto & line: lines)
        std::cout << line << std::endl;
}

void Cli::install_one_off_watch() {
    int attempts = 0;
    std::time_t currentModTime;
    do {
        currentModTime = getModifiedTime();
        if (currentModTime > _activePathModTime)
            break;
        /*
         * There seems to be a race to the _activePath file between the
         * file write of the counter and the access of this tool. So give
         * it some time to finish.
         */
        sleep(1);
        attempts++;
    } while ( attempts <= MAX_ATTEMPTS );
    if (attempts > MAX_ATTEMPTS) {
        std::cerr << "Daemon was unresponsive: " << strerror(errno) <<
            std::endl;
        exit(EXIT_FAILURE);
    }
}

std::time_t Cli::getModifiedTime() {
    try {
        if (std::filesystem::exists(_activePath)) {
            // Get the last write time as a filesystem::file_time_type
            auto ftime = std::filesystem::last_write_time(_activePath);

            // Convert to a system clock time_point
            auto sctp = std::chrono::time_point_cast<
                std::chrono::system_clock::duration>(
                    ftime - std::filesystem::file_time_type::clock::now() +
                    std::chrono::system_clock::now());

            // Convert the time_point to time_t (a C-style time)
            return std::chrono::system_clock::to_time_t(sctp);
        } else {
            // File doesn't exist, return the current time
            return std::time(nullptr);
        }
    } catch (const std::filesystem::filesystem_error& e) {
        std::cerr << "Filesystem error: " << e.what() << std::endl;
        exit(EXIT_FAILURE);
    }
}

std::string Cli::minutesToHour(unsigned long long input) {
    unsigned long long activeTimeHours = input / 60;
    unsigned int activeTimeMinutes = input % 60;
    double activeTimeHoursMinutes = (double)activeTimeHours +
        ((double)activeTimeMinutes / 60.00);
    std::stringstream ss;
    ss << std::fixed << std::setfill('0') << std::setw(4) <<
        std::setprecision(1) << activeTimeHoursMinutes;
    return ss.str();
}

void Cli::wrap_lines(std::vector<std::string> &lines) {
    int maxLineSize = 0;
    std::stringstream ss;
    for (const auto & line: lines) {
        if (line.size() > maxLineSize) {
            maxLineSize = line.size();
        }
    }
    for (int i = 0; i < maxLineSize; i++)
        ss << '*';
    std::string border = ss.str() + "****";
    for (auto & line: lines) {
        while (line.size() < maxLineSize) {
            line.append(" ");
        }
        line = "* " + line + " *";
    }
    lines.insert(lines.begin(), border);
    lines.push_back(border);
    return;
}
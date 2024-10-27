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

// How long it will wait between activity time read attempts
#define SLEEP_INTERVAL_SECONDS 1

#define BASE_FILE_PATH "/Users/Shared/"
#define ACTIVITY_COUNT BASE_FILE_PATH "activity.count"
#define ACTIVITY_PID BASE_FILE_PATH "activity.pid"

Cli::Cli() {
    _activePath = ACTIVITY_COUNT;
    _pidPath = ACTIVITY_PID;
    _activePathModTime = getModifiedTime();
    sendSigHup();
    installOneOffWatch();
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
    try {
        pid = std::stoul(ss.str());
        int result = kill(pid, 0);
        if (result) {
            std::cerr << "Pid doesn't exist " << pid << std::endl;
            exit(EXIT_FAILURE);
        }
        kill(pid, SIGHUP);
    } catch (std::invalid_argument const& e) {
        std::cerr << "Unexpected file contents of pid file : " << e.what() <<
            std::endl;
        exit(EXIT_FAILURE);
    }

}

void Cli::print() {
    static std::vector<std::string> weekDays = {
        "S", "M", "T", "W", "T", "F", "S"};
    std::string activeTime = readActiveTime();
    std::string number = "";
    unsigned long long num = 0;
    unsigned long long dailyMinutes = 0;
    unsigned long long dailyHours = 0;
    unsigned long long weeklyMinutes = 0;
    unsigned long long weeklyHours = 0;
    std::vector<unsigned long long> nums;
    std::vector<std::string> weekCounts;
    std::vector<std::string> lines;
    std::string line;
    std::string nextLine = "";
    if (activeTime == "") {
        std::cerr << "Failed to fetch active time" << std::endl;
        return;
    }
    for (const char & c : activeTime) {
        if (c != ' ') {
            number += c;
        } else {
            if (number != "") {
                num = std::stoull(number);
                nums.push_back(num);
                weeklyMinutes += num;
                number = "";
                num = 0;
            }
        }
    }
    for (unsigned long long & activeTime: nums) {
        weekCounts.push_back(minutesToHour(activeTime));
    }
    weeklyHours = weeklyMinutes / 60;
    weeklyMinutes %= 60;
    dailyMinutes = nums[getDayNum()];
    dailyHours = dailyMinutes / 60;
    dailyMinutes %= 60;
    lines.push_back("Day total: " + std::to_string(dailyHours) +
        + "h " + std::to_string(dailyMinutes) + "m");
    lines.push_back("Week total: " + std::to_string(weeklyHours) +
        "h " + std::to_string(weeklyMinutes) + "m");
    for (int i = 0; i < weekCounts.size()-1; i++) {
        line += weekCounts[i] + "|";
        nextLine += weekDays[i] +
            buildRepeatingChar(' ', weekCounts[i].size() -1) + "|";
    }
    line += weekCounts.back();
    nextLine += weekDays.back();
    lines.push_back(line);
    lines.push_back(nextLine);
    wrapLines(lines);
    for (const auto & line: lines)
        std::cout << line << std::endl;
}

void Cli::installOneOffWatch() {
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
            auto ftime = std::filesystem::last_write_time(_activePath);
            auto sctp = std::chrono::time_point_cast<
                std::chrono::system_clock::duration>(
                    ftime - std::filesystem::file_time_type::clock::now() +
                    std::chrono::system_clock::now());

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
    double activeTimeHours = (double) input / 60.0;
    std::stringstream oss;
    oss << std::fixed << std::setprecision(1) << activeTimeHours;
    std::string result = oss.str();
    return result;
}

void Cli::wrapLines(std::vector<std::string> &lines) {
    int maxLineSize = 0;
    for (const auto & line: lines) {
        if (line.size() > maxLineSize) {
            maxLineSize = line.size();
        }
    }
    // Add 4 since we add 2 '*' and 2 ' '.
    std::string border = buildRepeatingChar('*', maxLineSize + 4);
    for (auto & line: lines) {
        while (line.size() < maxLineSize) {
            line.append(" ");
        }
        line = "* " + line + " *";
    }
    lines.insert(lines.begin(), border);
    lines.insert(lines.begin() + 3, border);
    lines.push_back(border);
    return;
}

int Cli::getDayNum() {
    std::time_t now = std::time(nullptr);
    auto local_time = std::localtime(&now);
    return local_time->tm_wday;
}

std::string Cli::buildRepeatingChar(char c, int i) {
    std::ostringstream oss;
    for (; i > 0; i--)
        oss << c;
    return oss.str();
}
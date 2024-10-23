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

// How long it will wait between acitivty time reads
#define SLEEP_INTERVAL_SECONDS 1

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
    std::string activeTime = readActiveTime();
    if (activeTime == "") {
        std::cerr << "Failed to fetch active time";
        return;
    }
    // File has unit type of Minutes
    unsigned long long activeTimeLong = std::stoull(activeTime);
    unsigned long long activeTimeHours = activeTimeLong / 60;
    unsigned long long activeTimeSeconds = activeTimeLong % 60;
    std::cout << activeTimeHours << " hours, " << activeTimeSeconds << " minutes" << std::endl;
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
        std::cerr << "Daemon was unresponsive try again in a bit: " << strerror(errno) <<
            std::endl;
        exit(EXIT_FAILURE);
    }
    std::cout << attempts << std::endl;
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
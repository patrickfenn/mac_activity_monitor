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
    "-----------------------------------------"

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
    static std::vector<char> weekDays = {'S', 'M', 'T', 'W', 'T', 'F', 'S'};
    std::string activeTime = readActiveTime();
    std::string number = "";
    unsigned long long num = 0;
    unsigned long long totalHours = 0;
    unsigned long long totalMinutes = 0;
    double totalHoursMinutes = 0;
    unsigned long long activeTimeHours = 0;
    unsigned long long activeTimeMinutes = 0;
    double activeTimeHoursMinutes = 0.0;
    std::vector<unsigned long long> nums;
    std::vector<std::string> weekCounts;
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
                totalMinutes += num;
                nums.push_back(num);
                number = "";
                num = 0;
            }
        }
    }
    for (unsigned long long & activeTime: nums) {
        // File has unit type of Minutes
        activeTimeHours = activeTime / 60;
        activeTimeMinutes = activeTime % 60;
        activeTimeHoursMinutes = activeTimeHours + ((double)activeTimeMinutes / 60.00);
        weekCounts.push_back(std::to_string(activeTimeHoursMinutes));
    }
    totalHours = totalMinutes / 60;
    totalHoursMinutes = totalHours + ((double)totalMinutes / 60.00);
    std::cout << PRINT_BORDER << std::endl;
    std::cout << "Weekly hours: " << formatDoubleString(std::to_string(totalHoursMinutes))  << std::endl;
    std::cout << "Weekly breakdown hours: " << std::endl;
    for (const auto & entry : weekCounts) {
        std::cout << formatDoubleString(entry) << ' ';
    }
    std::cout << std::endl;
    for (const auto & c: weekDays) {
        std::cout << c << "     ";
    }
    std::cout << std::endl;
    std::cout << PRINT_BORDER << std::endl;
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

std::string Cli::formatDoubleString(const std::string& input) {
    // Convert string to double
    double value = std::stod(input);
    std::ostringstream formattedStream;
    // Ensure at least 2 digits before the decimal and 2 after with fixed precision
    formattedStream << std::fixed << std::setfill('0') << std::setw(5) <<
        std::setprecision(2) << value;

    // Return the formatted string
    return formattedStream.str();
}
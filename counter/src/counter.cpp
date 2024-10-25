#include "counter.hpp"

#include <iostream>
#include <IOKit/IOKitLib.h>
#include <IOKit/pwr_mgt/IOPMLib.h>
#include <CoreFoundation/CoreFoundation.h>
#include <thread>
#include <chrono>
#include <fstream>
#include <csignal>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <cstdlib>

#define INCREMENT_INTERVAL_SECONDS 60 // Increments every minute

#define BASE_FILE_PATH "/Users/Shared/"
#define ACTIVITY_COUNT BASE_FILE_PATH "activity.count"
#define ACTIVITY_PID BASE_FILE_PATH "activity.pid"
#define ACTIVITY_LOG BASE_FILE_PATH "activity.log"

extern char **environ;

Counter::Counter() {
    _activePath = ACTIVITY_COUNT;
    _pidPath = ACTIVITY_PID;
    char* dayToResetChar = std::getenv("DAY_TO_RESET");
    char* hourToResetChar = std::getenv("HOUR_TO_RESET");
    char* maxIdleSecondsChar = std::getenv("MAX_IDLE_SECONDS");

    if (dayToResetChar)
        _dayToReset = std::string(dayToResetChar);
    else
        _dayToReset = "1";
    if (hourToResetChar)
        _hourToReset = std::string(hourToResetChar);
    else
        _hourToReset = "00";
    if (maxIdleSecondsChar) {
        std::string maxIdleSecondsStr = std::string(maxIdleSecondsChar);
        _maxIdleSeconds = std::stoul(maxIdleSecondsStr);
    } else {
        _maxIdleSeconds = 600;
    }
    reset();
}

Counter::~Counter() {
}

Counter* Counter::getInstance() {
    static Counter instance;
    return &instance;
}

void Counter::handleSignal(int signal) {
    if (signal == SIGHUP) {
        getInstance()->write();
    } else if (signal == SIGSTOP) {
        getInstance()->write();
        exit(EXIT_SUCCESS);
    }
}

void Counter::loop() {
    std::time_t now;
    while (true) {
        now = std::time(nullptr);
        local_time = std::localtime(&now);
        if (now >= _nextReset) {
            reset();
            updateNextReset();
        }
        if (getSystemIdleTime() < _maxIdleSeconds) {
            increment();
        }
        std::this_thread::sleep_for(std::chrono::seconds(
            INCREMENT_INTERVAL_SECONDS));
    }
}

void Counter::increment() {
    _activeTime[local_time->tm_wday]++;
}

void Counter::reset() {
    _activeTime = std::vector<unsigned long long>(7,0);
}

void Counter::start() {
    daemonize();
    read();
    updateNextReset();
    std::signal(SIGHUP, handleSignal);
    std::signal(SIGSTOP, handleSignal);
    std::thread loopThread(&Counter::loop, this);
    std::cout << "Counter started" << std::endl;
    loopThread.join();
}


// Function to redirect file descriptors to log file location
void Counter::redirectFds() {
    int fd = open(ACTIVITY_LOG, O_RDWR | O_APPEND);
    if (fd != -1) {
        dup2(fd, STDIN_FILENO);
        dup2(fd, STDOUT_FILENO);
        dup2(fd, STDERR_FILENO);
        if (fd > 2) {
            close(fd);
        }
    }
}

void Counter::daemonize() {
    pid_t pid;

    // Fork off the parent process
    pid = fork();

    if (pid < 0) {
        std::cerr << "Failed to fork" << std::endl;
        exit(EXIT_FAILURE);
    }

    // If we got a good PID, exit the parent process
    if (pid > 0) {
        std::cout << "Daemon process PID: " << pid << std::endl;
        // Write the PID to a file for later tracking
        std::ofstream out(_pidPath.c_str());
        if (out.is_open()) {
            out << pid << std::endl;
            out.close();
        }
        exit(EXIT_SUCCESS);
    }

    redirectFds();
}

unsigned long Counter::getSystemIdleTime() {
    mach_port_t masterPort;
    masterPort = kIOMainPortDefault;

    io_iterator_t iterator;
    io_registry_entry_t entry = 0;
    unsigned long idleTime = 0;

    kern_return_t result = IOServiceGetMatchingServices(masterPort,
        IOServiceMatching("IOHIDSystem"), &iterator);
    if (result == KERN_SUCCESS) {
        entry = IOIteratorNext(iterator);
        if (entry) {
            CFMutableDictionaryRef properties = nullptr;
                result = IORegistryEntryCreateCFProperties(entry,
            &properties, kCFAllocatorDefault, 0);
            if (result == KERN_SUCCESS) {
                CFNumberRef idleTimeNumber = (CFNumberRef)CFDictionaryGetValue(
                    properties, CFSTR("HIDIdleTime"));
                if (idleTimeNumber) {
                    int64_t idleTimeNanoseconds;
                    CFNumberGetValue(idleTimeNumber,
                        kCFNumberSInt64Type, &idleTimeNanoseconds);
                    idleTime = (unsigned long)(
                        idleTimeNanoseconds / 1000000000);
                }
                CFRelease(properties);
            }
            IOObjectRelease(entry);
        }
        IOObjectRelease(iterator);
    }

    return idleTime;
}

bool Counter::write() {
    std::ofstream out(_activePath, std::ios_base::binary);
    if (!out) {
        std::cerr << "Failed to open active time file." << std::endl;
        return false;
    }
    for (const auto & dailyActiveTime: _activeTime)
        out << dailyActiveTime << ' ';
    out.close();
    return true;
}

bool Counter::read() {
    std::stringstream ss;
    std::string line;
    std::string number;
    unsigned long long num;
    std::ifstream in(_activePath, std::ios_base::binary);
    if (!in) {
        std::cerr << "Failed to open active time file." << std::endl;
        return 0;
    }
    ss << in.rdbuf();
    in.close();
    _activeTime.clear();
    line = ss.str();
    for (const char & c : line) {
        if (c != ' ') {
            number += c;
        } else {
            if (number != "") {
                num = std::stoull(number);
                _activeTime.push_back(num);
                number = "";
                num = 0;
            }
        }
    }
    if (_activeTime.size() != 7) {
        std::cerr << "Was unable to read in a full week of times." << std::endl;
        _activeTime.clear();
        return 0;
    }
    return 1;
}

void Counter::updateNextReset() {
    // Convert input day and hour to integers
    int targetDay = std::stoi(_dayToReset);  // 0 = Sunday, ..., 6 = Saturday
    int targetHour = std::stoi(_hourToReset);      // "00" = 0, "23" = 23, etc.

    // Get the current time
    auto now = std::chrono::system_clock::now();
    std::time_t nowTimeT = std::chrono::system_clock::to_time_t(now);
    std::tm currentTime = *std::localtime(&nowTimeT);

    // Get current day of the week (0 = Sunday, ..., 6 = Saturday)
    int currentDay = currentTime.tm_wday;
    int currentHour = currentTime.tm_hour;

    // Calculate the number of days forward to the target day
    int daysForward = (targetDay - currentDay + 7) % 7;

    // If today is the target day, check the hour
    if (daysForward == 0 && targetHour <= currentHour) {
        // If it's today but the target hour has already passed, move to the next week
        daysForward = 7;
    }

    // Calculate the target time for the next occurrence
    std::tm nextOccurrenceTime = currentTime;
    nextOccurrenceTime.tm_wday = currentDay + daysForward;
    nextOccurrenceTime.tm_hour = targetHour;
    nextOccurrenceTime.tm_min = 0;
    nextOccurrenceTime.tm_sec = 0;

    // Adjust the date for the day difference
    nextOccurrenceTime.tm_mday += daysForward;

    // Convert the next occurrence time to time_t (seconds since epoch)
    std::time_t nextOccurrenceEpoch = std::mktime(&nextOccurrenceTime);

    // Return the next occurrence as a time_t
    _nextReset = nextOccurrenceEpoch;
}
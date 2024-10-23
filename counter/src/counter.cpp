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

// Knobs
#define ACTIVE_TIME_PATH "/Users/patrickfenn/active"
#define PID_PATH "/Users/patrickfenn/counter.pid"
#define DAY_TO_RESET "Sunday" // Resets Sunday at...
#define HOUR_TO_RESET "00" // Midnight (24 hour)
#define MAX_IDLE_SECONDS 600 // After 10 minutes active time stops incrementing
#define INCREMENT_INTERVAL_SECONDS 60 // Increments every minute


extern char **environ;

Counter::Counter() {
    _activeTimeFilePath = ACTIVE_TIME_PATH;
    _dayToReset = DAY_TO_RESET;
    _hourToReset = HOUR_TO_RESET;
    _maxIdleSeconds = MAX_IDLE_SECONDS;
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
    while (true) {
        if (resetTime())
            getInstance()->reset();
        if (getSystemIdleTime() < _maxIdleSeconds)
            getInstance()->increment();
        std::this_thread::sleep_for(std::chrono::seconds(
            INCREMENT_INTERVAL_SECONDS));
    }
}

bool Counter::resetTime() {
    // Get the current local time
    std::time_t now = std::time(nullptr);
    std::tm* local_time = std::localtime(&now);

    static std::string days_of_week[] = {
        "Sunday", "Monday", "Tuesday", "Wednesday", "Thursday",
        "Friday", "Saturday"};

    std::string current_day = days_of_week[local_time->tm_wday];
    int current_hour = local_time->tm_hour;

    if (current_day == DAY_TO_RESET &&
        std::to_string(current_hour) == HOUR_TO_RESET) {

        return true;
    }

    return false;
}

void Counter::increment() {
    _activeTime++;
}

void Counter::reset() {
    _activeTime = 0;
}

void Counter::start() {
    daemonize();
    // Cli expects for file to be there so a watch can be installed.
    if (!read()) {
        _activeTime = 0;
        write();
    }
    std::signal(SIGHUP, handleSignal);
    std::signal(SIGSTOP, handleSignal);
    std::thread loopThread(&Counter::loop, this);
    loopThread.join();
    std::cout << "Counter started." << std::endl;
}


// Function to redirect file descriptors to /dev/null
void Counter::redirectFds() {
    int fd = open("/dev/null", O_RDWR);
    if (fd != -1) {
        dup2(fd, STDIN_FILENO);
        dup2(fd, STDOUT_FILENO);
        dup2(fd, STDERR_FILENO);
        if (fd > 2) {
            close(fd);
        }
    }
}

// Function to create the daemon using posix_spawn
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
        std::ofstream PID_PATH(PID_PATH);
        if (PID_PATH.is_open()) {
            PID_PATH << pid << std::endl;
            PID_PATH.close();
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
    std::ofstream out(_activeTimeFilePath, std::ios_base::binary);
    if (!out) {
        std::cerr << "Failed to open active time file." << std::endl;
        return false;
    }
    out << _activeTime;
    out.close();
    return true;
}

bool Counter::read() {
    std::stringstream ss;
    std::ifstream in(_activeTimeFilePath, std::ios_base::binary);
    if (!in) {
        std::cerr << "Failed to open active time file." << std::endl;
        return false;
    }
    ss << in.rdbuf();
    in.close();
    _activeTime = std::stoull(ss.str());
    return 1;
}
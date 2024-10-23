#include "cli.hpp"

#include <iostream>
#include <fstream>
#include <sstream>
#include <signal.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/event.h>
#include <sys/time.h>
#include <fcntl.h>
#include <cstring>

#define ACTIVE_TIME_PATH "/Users/patrickfenn/active"
#define PID_PATH "/Users/patrickfenn/counter.pid"
#define MAX_ATTEMPTS 5

Cli::Cli() {
    sendSigHup();
    install_one_off_watch();
    print();
}

Cli::~Cli() {
}

std::string Cli::readActiveTime() {
    std::ifstream in(ACTIVE_TIME_PATH, std::ios_base::binary);
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
    std::ifstream in(PID_PATH, std::ios_base::binary);
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
    int kq = kqueue();
    if (kq == -1) {
        std::cerr << "Failed to create kqueue: " << strerror(errno) << std::endl;
        exit(EXIT_FAILURE);
    }

    int fd = open(ACTIVE_TIME_PATH, O_EVTONLY);
    if (fd == -1) {
        std::cerr << "Failed to open file: " << strerror(errno) << std::endl;
        close(kq);
        exit(EXIT_FAILURE);
    }

    struct kevent change;
    EV_SET(&change, fd, EVFILT_VNODE, EV_ADD | EV_ENABLE,
           NOTE_WRITE | NOTE_EXTEND | NOTE_DELETE | NOTE_ATTRIB, 0, nullptr);

    struct kevent event;
    int attempts = 0;
    int nev = -1;
    do {
        kevent(kq, &change, 1, &event, 1, nullptr);
        if (nev == -1) {
            /*
             * There seems to be a race to the ACTIVE_TIME_PATH file between the
             * file write of the counter and the access of this tool. So give
             * it some time to finish.
             */
            if (errno == EPERM || errno == ETIMEDOUT || errno == EWOULDBLOCK) {
                attempts++;
                if (attempts == MAX_ATTEMPTS) {
                    std::cerr << "Issue with file open" << std::endl;
                    exit(EXIT_FAILURE);
                } else {
                    usleep(100);
                }
            } else {
                std::cerr << "kevent error: " << strerror(errno) << std::endl;
            }
        } else if (nev > 0) {
            if (event.fflags & NOTE_WRITE) {
                std::cout << "File modified!" << std::endl;
            }
        }
    } while ( !(event.fflags & NOTE_WRITE) );
    close(fd);
    close(kq);
}
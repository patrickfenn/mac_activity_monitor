#include <string>
#include <vector>
#include <ctime>

#define COUNTER Counter::getInstance()

class Counter {
public:
    Counter();
    ~Counter();
    static Counter* getInstance();
    static void handleSignal(int signal);
    void loop();
    void increment();
    void reset();
    void start();
    void redirectFds();
    void daemonize();
private:
    static unsigned long getSystemIdleTime();
    bool write();
    bool read();
    void updateNextReset();
    std::string _activePath;
    std::string _pidPath;
    std::string _dayToReset;
    std::string _hourToReset;
    unsigned long _maxIdleSeconds;
    std::vector<unsigned long long> _activeTime;
    std::tm* local_time;
    std::time_t _nextReset;
};
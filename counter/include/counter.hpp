#include <string>

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
    bool resetTime();
    std::string _activeTimeFilePath;
    std::string _dayToReset;
    std::string _hourToReset;
    unsigned long _maxIdleSeconds;
    unsigned long long _activeTime;
};
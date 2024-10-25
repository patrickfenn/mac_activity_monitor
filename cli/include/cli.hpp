#include <string>
#include <vector>

class Cli {
public:
    Cli();
    ~Cli();
private:
    std::string _activePath;
    std::string _pidPath;
    std::time_t _activePathModTime;
    std::string readActiveTime();
    void sendSigHup();
    void print();
    void installOneOffWatch();
    std::time_t getModifiedTime();
    std::string minutesToHour(unsigned long long input);
    void wrapLines(std::vector<std::string> &lines);
    int getDayNum();
};
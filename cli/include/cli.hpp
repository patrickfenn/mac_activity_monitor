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
    void install_one_off_watch();
    std::time_t getModifiedTime();
    std::string minutesToHour(unsigned long long input);
    void wrap_lines(std::vector<std::string> &lines);
};
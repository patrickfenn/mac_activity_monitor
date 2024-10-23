#include <string>

class Cli {
public:
    Cli();
    ~Cli();
private:
    std::string _activePath;
    std::string _pidPath;
    std::string readActiveTime();
    void sendSigHup();
    void print();
    void install_one_off_watch();
};
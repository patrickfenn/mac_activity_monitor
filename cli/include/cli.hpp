#include <string>

class Cli {
public:
    Cli();
    ~Cli();
private:
    std::string readActiveTime();
    void sendSigHup();
    void print();
    void install_one_off_watch();
};
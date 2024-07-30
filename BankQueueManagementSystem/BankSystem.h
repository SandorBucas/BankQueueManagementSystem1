#include <vector>
#include <queue>
#include <string>
#include <map>
#include <memory>
#include <mutex>
#include <condition_variable>
#include <fstream>
#include <ctime>

struct Client {
    std::string name;
    int time;
    int priority;
    std::vector<std::string> departments;
};

struct ClientComparator {
    bool operator()(const Client& lhs, const Client& rhs) const {
        return lhs.priority < rhs.priority;
    }
};

struct Department {
    std::string name;
    int employees;
    int activeEmployees;
    std::priority_queue<Client, std::vector<Client>, ClientComparator> clientsQueue;
    std::mutex mtx;
    std::condition_variable cv;

    Department() : employees(0), activeEmployees(0) {}
};

class BankSystem {
private:
    std::vector<std::unique_ptr<Department>> departments;
    std::map<std::string, Department*> departmentsMap;
    std::vector<Client> clients;
    bool bankOpen;
    void serveClient(Client client, Department& department);
    void logEvent(const std::string& message);
    std::string getCurrentTime();

public:
    BankSystem(const std::string& inputFile);
    void openBank();
    void processClients();
    void closeBank();
    void loadConfig(const std::string& inputFile);
    std::ofstream logFile;
    std::mutex logMutex;
};

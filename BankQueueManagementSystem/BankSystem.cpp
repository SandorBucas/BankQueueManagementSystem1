#include "BankSystem.h"
#include "rapidjson/filereadstream.h"
#include "rapidjson/document.h"
#include <cstdio>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <iostream>
#include <mutex>
#include <ctime>

using namespace rapidjson;

BankSystem::BankSystem(const std::string& inputFile) {
    try {
        loadConfig(inputFile);
    }
    catch (const std::exception& e) {
        std::cerr << "Error during initialization: " << e.what() << std::endl;
        throw;
    }
    bankOpen = false;
}

void BankSystem::loadConfig(const std::string& inputFile) {
    FILE* fp;

#ifdef _WIN32
    fopen_s(&fp, inputFile.c_str(), "r");
#else
    fp = fopen(inputFile.c_str(), "r");
#endif

    if (fp == nullptr) {
        throw std::runtime_error("Unable to open input file.");
    }

    char readBuffer[65536];
    FileReadStream is(fp, readBuffer, sizeof(readBuffer));
    Document d;
    d.ParseStream(is);
    fclose(fp);

    if (!d.IsObject()) {
        throw std::runtime_error("Invalid JSON format.");
    }

    const Value& departmentsArr = d["departments"];
    if (!departmentsArr.IsArray()) {
        throw std::runtime_error("Invalid departments format.");
    }
    for (SizeType i = 0; i < departmentsArr.Size(); i++) {
        auto dept = std::make_unique<Department>();
        dept->name = departmentsArr[i]["name"].GetString();
        dept->employees = departmentsArr[i]["employees"].GetInt();
        dept->activeEmployees = dept->employees;
        departmentsMap[dept->name] = dept.get();
        departments.push_back(std::move(dept));
    }

    const Value& clientsArr = d["clients"];
    if (!clientsArr.IsArray()) {
        throw std::runtime_error("Invalid clients format.");
    }
    for (SizeType i = 0; i < clientsArr.Size(); i++) {
        Client client;
        client.name = clientsArr[i]["name"].GetString();
        client.time = clientsArr[i]["time"].GetInt();
        client.priority = clientsArr[i]["priority"].GetInt();
        const Value& deps = clientsArr[i]["departments"];
        for (SizeType j = 0; j < deps.Size(); j++) {
            client.departments.push_back(deps[j].GetString());
        }
        clients.push_back(client);
    }
}

void BankSystem::openBank() {
    for (const auto& department : departments) {
        logEvent("Employees of department " + department->name + " are taking their positions.");
    }
    logEvent("Bank opened");
    bankOpen = true;
}

void BankSystem::processClients() {
    std::vector<std::thread> threads;
    for (Client& client : clients) {
        threads.emplace_back([this, &client] {
            for (const std::string& deptName : client.departments) {
                Department* department = departmentsMap[deptName];
                {
                    std::unique_lock<std::mutex> lock(department->mtx);
                    while (department->activeEmployees == 0) {
                        department->cv.wait(lock);
                    }
                    department->activeEmployees--;
                    department->clientsQueue.push(client);
                    logEvent("Client " + client.name + " came to the department " + deptName);
                    department->cv.notify_one();
                }

                logEvent("Client " + client.name + " is served by the department " + deptName);

                std::this_thread::sleep_for(std::chrono::milliseconds(client.time));

                logEvent("Client " + client.name + " left the department " + deptName);

                {
                    std::unique_lock<std::mutex> lock(department->mtx);
                    department->activeEmployees++;
                    department->cv.notify_one();
                }
            }
            });
    }

    for (std::thread& thread : threads) {
        thread.join();
    }
}

void BankSystem::closeBank() {
    bankOpen = false;
    for (auto& department : departments) {
        std::unique_lock<std::mutex> lock(department->mtx);
        department->cv.notify_all();
    }
    logEvent("Bank closed");
    for (const auto& department : departments) {
        logEvent("Employees of department " + department->name + " have left their positions.");
    }
}

void BankSystem::logEvent(const std::string& message) {
    {
        std::lock_guard<std::mutex> guard(logMutex);
        std::string currentTime = getCurrentTime();
        std::cout << "[" << currentTime << "] " << message << std::endl;
        if (logFile.is_open()) {
            logFile << message << std::endl;
        }
    }
    std::cout.flush();
}

std::string BankSystem::getCurrentTime() {
    auto now = std::chrono::system_clock::now();
    auto now_c = std::chrono::system_clock::to_time_t(now);
    auto now_ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()) % 1000;
    std::tm now_tm;

#ifdef _WIN32
    localtime_s(&now_tm, &now_c);
#else
    localtime_r(&now_c, &now_tm);
#endif

    std::ostringstream oss;
    oss << std::put_time(&now_tm, "%Y-%m-%d %H:%M:%S")
        << "." << std::setw(3) << std::setfill('0') << now_ms.count();

    return oss.str();
}

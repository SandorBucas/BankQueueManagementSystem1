#include <iostream>
#include "BankSystem.h"

int main() {
    std::string inputPath = "data/input.json";  
    BankSystem bank(inputPath);
    bank.openBank();
    bank.processClients();
    bank.closeBank();
    std::cout << "Press Enter to exit..." << std::endl;
    std::cin.get();
    return 0;
}

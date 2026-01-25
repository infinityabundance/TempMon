#include <iostream>

int main() {
    std::cerr << "[TempMon][ERROR] Missing GTK/AppIndicator development packages." << std::endl;
    std::cerr << "[TempMon][ERROR] Install gtk3 and libappindicator development packages, then rebuild." << std::endl;
    return 1;
}

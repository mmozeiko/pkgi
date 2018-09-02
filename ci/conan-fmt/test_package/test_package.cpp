#include <string>
#include "fmt/format.h"

int main() {
    std::string thing("World");
    fmt::print("Hello {}!\n", thing);
    return 0;
}

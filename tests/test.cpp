#include <string>
#include <sstream>
#include "miro/engine.hpp"
using namespace miro;
using namespace std::string_literals;

std::string change_to_beer(std::string_view s, miro::context&) {
    return "beer";
}

int main() {
    miro::engine engine{};
    miro::context ctx{};
    ctx.add_entity("what", "world");
    ctx.add_entity("mod", "!");
    std::stringstream out;

    std::stringstream ss {"Hello {{what}}. Yes, {{what}}{{mod}}"s};
    engine.render(ss, out, ctx);
}



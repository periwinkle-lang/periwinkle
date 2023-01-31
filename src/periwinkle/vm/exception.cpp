#include "exception.h"

std::string vm::SyntaxException::toString() const
{
    std::stringstream ss;
    ss << "Синтаксична помилка: ";
    ss << message << " (знайдено на " << lineno << " рядку)";
    return ss.str();
}

void vm::throwSyntaxException(const SyntaxException& exception, const std::string& code)
{
    std::cerr << exception.toString() << std::endl;
    auto line = utils::getLineFromString(code, exception.getLineno());
    std::cerr << utils::indent(4) << line << std::endl;
    auto offset = utils::utf8Size(line.substr(0, exception.getPosition()));
    std::cerr << utils::indent(4 + offset) << "^" << std::endl;
}

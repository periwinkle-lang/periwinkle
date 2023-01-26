#include "exception.h"

std::string vm::SyntaxException::toString()
{
    std::stringstream ss;
    ss << "Синтаксична помилка: ";
    ss << message << " (знайдено на " << lineno << " рядку)";
    return ss.str();
}

void vm::throwSyntaxException(SyntaxException exception, const std::string& code, size_t position)
{
    std::cerr << exception.toString() << std::endl;
    auto line = utils::getLineFromString(code, exception.getLineno());
    auto positionInLine = utils::positionInLineFromPosition(code, position);
    std::cerr << utils::indent(4) << line << std::endl;
    auto offset = utils::utf8Size(line.substr(0, positionInLine));
    std::cerr << utils::indent(4 + offset) << "^" << std::endl;
}

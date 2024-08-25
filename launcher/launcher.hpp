#ifndef LAUNCHER_HPP
#define LAUNCHER_HPP

#include <span>
#include <string_view>

int launcher(std::span<const std::wstring_view> wargs) noexcept;
int launcher(std::span<const std::string_view> args) noexcept;

#endif

#ifndef KERNEL_UTILITY_HPP
#define KERNEL_UTILITY_HPP

#include "DString.hpp"

DString get_peer_ip(int fd);

bool check_ip(const DString &str);

#endif // KERNEL_UTILITY_HPP

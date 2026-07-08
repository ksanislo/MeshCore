/*
 * @Description: Printable
 * @Author: LILYGO_L
 * @Date: 2025-08-09 11:52:13
 * @LastEditTime: 2025-08-09 13:54:57
 * @License: GPL 3.0
 */
#pragma once

#include <stdlib.h>

class Print;

/** The Printable class provides a way for new classes to allow themselves to be printed.
 By deriving from Printable and implementing the printTo method, it will then be possible
 for users to print out instances of this class by passing them into the usual
 Print::print and Print::println methods.
 */

class Printable
{
public:
    virtual ~Printable() {}
    virtual size_t printTo(Print &p) const = 0;
};

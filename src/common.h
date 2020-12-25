#pragma once

#include "api/peer_connection_interface.h"

class NotImplementedException : public std::logic_error
{
public:
    NotImplementedException() : std::logic_error{ "Function not yet implemented." } {}
};


template<typename... Args>
void check(bool everything_OK, Args&&... args)
{
    throw NotImplementedException();
}



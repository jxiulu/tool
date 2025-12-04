// app uuid generator

#pragma once

#include <boost/uuid.hpp>
#include <boost/uuid/random_generator.hpp>

namespace setman
{

inline boost::uuids::uuid generate_uuid()
{
    static boost::uuids::random_generator gen;
    return gen();
}

} // namespace setman

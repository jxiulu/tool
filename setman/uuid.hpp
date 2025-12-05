// app uuid generator

#pragma once

#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>

namespace setman
{

inline boost::uuids::uuid generate_uuid()
{
    static boost::uuids::random_generator gen;
    return gen();
}

} // namespace setman

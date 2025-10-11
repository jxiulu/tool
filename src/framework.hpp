#include <algorithm>
#include <chrono>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

#include <cctype>
#include <curl/curl.h>
#include <curl/easy.h>
#include <memory>
#include <utility>

#include "jwt-cpp/jwt.h"
#include <nlohmann/json.hpp>

using jason = nlohmann::json;
namespace fs = std::filesystem;

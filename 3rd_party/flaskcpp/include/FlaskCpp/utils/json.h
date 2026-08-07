#ifndef FlaskCpp_UTILS_JSON
#define FlaskCpp_UTILS_JSON

#include <iostream>
#include <string>
#include <vector>
#include <sstream>
#include <iomanip>
// #include <ostream>
#include <regex>
#include <unordered_map>

namespace flaskcpp
{
class JsonGenerator
{
public:
    JsonGenerator(int precision=8) {setPrecision(precision);}

    void setPrecision(int precision=8){this->precision = precision;}

    template<typename T>
    std::string add(std::string key, std::vector<T> values)
    {
        std::ostringstream oss;
        oss << "[";
        int i=0;
        for (T& value: values)
        {
            if (i) oss << ",";
            oss << this->add("", value);
            i++;
        }
        oss << "]";
        if (key.size()) 
        {
            json_data[key] = oss.str();
            isBool[key] = false;
            isJson[key] = false;
            isList[key] = true;
        }
        return oss.str();
    }

    std::string add(std::string key, bool value)
    {
        if (key.size()) json_data[key] = std::string(value?"true":"false");
        isBool[key] = true;
        isJson[key] = false;
        isList[key] = false;
        return std::string(value?"true":"false");
    }

    std::string add(std::string key, JsonGenerator& json)
    {
        // if (this == &json) 
        // {
        //     std::cerr << "can not add self!" << std::endl;
        //     return json.toString();
        // }
        if (key.size()) 
        {
            json_data[key] = json.toString();
            isBool[key] = false;
            isJson[key] = true;
            isList[key] = false;
        }
        return json.toString();
    }

    std::string add(std::string key, const std::string& value)
    {
        std::ostringstream oss;
        if (key.empty()) oss << "\"" << value << "\"";
        else oss << value;
        std::string v = oss.str();
        if (key.size()) 
        {
            json_data[key] = v;
            isBool[key] = false;
            isJson[key] = false;
            isList[key] = false;
        }
        return v;
    }

    template<typename T>
    std::string add(std::string key, T value)
    {
        std::ostringstream oss;
        oss << std::fixed << std::setprecision(this->precision);
        oss << value;
        std::string v = oss.str();
        if (key.size()) 
        {
            json_data[key] = v;
            isBool[key] = false;
            isJson[key] = false;
            isList[key] = false;
        }
        return v;
    }

    std::string toString()
    {
        std::ostringstream oss;
        oss << "{";
        int i=0;
        for(const auto& [k, v] : json_data)
        {
            if (i) oss << ",";
            oss << "\"" << k << "\":";
            if (!isNumber(v) && !isBool[k] && !isJson[k] && !isList[k]) oss << "\"" << v << "\"";
            else oss << v;
            i++;
        }
        oss << "}";
        return oss.str();
    }

    std::map<std::string,std::string> toMap()
    {
        std::map<std::string,std::string> map_;
        for(const auto& [k, v]: json_data)
        {
            map_[k] = v;
        }
        return map_;
    }

    void clear()
    {
        json_data.clear();
    }

private:
    std::unordered_map<std::string, std::string> json_data;
    std::unordered_map<std::string, bool> isBool, isJson, isList;
    int precision=8;


    bool isNumber(const std::string& str) {
        std::regex pattern(R"([-+]?\d+(\.\d+)?)");
        return std::regex_match(str, pattern);
    }
};


}





#endif
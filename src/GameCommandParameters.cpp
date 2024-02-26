#include "GameCommandParameters.h"

#include <cstring>

using namespace std;

GameCommandParameters::GameCommandParameters()
{
}

GameCommandParameters::~GameCommandParameters()
{
}

void GameCommandParameters::SetValues(int argc, char** argv)
{
    char* currentParameter = nullptr;
    for (int i = 0; i < argc; ++i)
    {
        char* parameter = argv[i];
        if (strlen(parameter) > 0)
        {
            if (parameter[0] == '-')
            {
                if (currentParameter != nullptr)
                {
                    m_parameters[string(&currentParameter[1])] = s_emptyString;
                    currentParameter = nullptr;
                }

                currentParameter = parameter;
            }
            else
            {
                if (currentParameter != nullptr)
                {
                    m_parameters[string(&currentParameter[1])] = string(parameter);
                    currentParameter = nullptr;
                }
                
                // if we find a value that wasn't part of a command we just ignore it
            }
        }
    }

    if (currentParameter != nullptr)
    {
        m_parameters[string(&currentParameter[1])] = string(s_emptyString);
        currentParameter = nullptr;
    }
}

bool GameCommandParameters::IsSet(const std::string& name) const
{
    ParameterMap::const_iterator it = m_parameters.find(name);
    if (it != m_parameters.end())
    {
        return true;
    }
    return false;
}

const std::string& GameCommandParameters::GetString(const std::string& name) const
{
    ParameterMap::const_iterator it = m_parameters.find(name);
    if (it != m_parameters.end())
    {
        return it->second;
    }

    return s_emptyString;
}

long GameCommandParameters::GetLong(const std::string& name) const
{
    ParameterMap::const_iterator it = m_parameters.find(name);
    if (it != m_parameters.end())
    {
        return atol(it->second.c_str());
    }
    return 0l;
}

double GameCommandParameters::GetDouble(const std::string& name) const
{
    ParameterMap::const_iterator it = m_parameters.find(name);
    if (it != m_parameters.end())
    {
        return atof(it->second.c_str());
    }
    return 0.0;
}

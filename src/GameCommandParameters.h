#pragma once

#include <unordered_map>
#include <string>

class GameCommandParameters
{
public:
	GameCommandParameters();
	~GameCommandParameters();

	void SetValues(int argc, char** argv);

	bool IsSet(const std::string& name) const;

	const std::string& GetString(const std::string& name) const;
	long GetLong(const std::string& name) const;
	double GetDouble(const std::string& name) const;

private:
	const std::string s_emptyString;
	typedef std::unordered_map<std::string, std::string> ParameterMap;
	ParameterMap m_parameters;
};

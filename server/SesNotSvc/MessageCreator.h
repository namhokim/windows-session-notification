#pragma once
#include <string>
#include <vector>

class MessageCreator
{
public:
	MessageCreator(unsigned short interestedPort);
	~MessageCreator();
	std::string makeMessage(unsigned long code);
private:
	unsigned short port;
	std::string dwordToString(unsigned long dw);
	std::string codeToString(unsigned long code);
	std::string addrsToString(const std::vector<std::string>& addrs);
};


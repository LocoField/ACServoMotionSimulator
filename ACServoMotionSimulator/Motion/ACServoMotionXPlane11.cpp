#include "ACServoMotionXPlane11.h"

#include <iostream>
#include <fstream>
#include <sstream>
#include <algorithm>

using namespace std;

ACServoMotionXPlane11::ACServoMotionXPlane11()
{
	
}

char* ACServoMotionXPlane11::getMotionName()
{
	return "X-Plane 11";
}

bool ACServoMotionXPlane11::start()
{
	if (dataFilePath_.empty() || numRecords_ < 3)
		return false;

	realTime_ = 0;
	lastDataPosition = std::_BADOFF;

	return true;
}

void ACServoMotionXPlane11::stop()
{
}

bool ACServoMotionXPlane11::process(void* arg)
{
	auto readLastRecordsFromData = [this](std::vector<std::string>& records)
	{
		ifstream fs;
		fs.open(dataFilePath_);

		if (lastDataPosition == std::_BADOFF)
			fs.seekg(-3, ios_base::end);
		else
			fs.seekg(lastDataPosition);

		while (1)
		{
			char ch;
			fs.get(ch);

			if (ch == '\n')
			{
				std::string data;
				getline(fs, data);

				records = parsingData(data);
				break;
			}

			fs.seekg(-2, ios_base::cur);
		}

		bool completeData = records.size() == numRecords_;

		if (completeData)
			lastDataPosition = std::_BADOFF;
		else
			lastDataPosition = fs.tellg();

		fs.close();
		return completeData;
	};

	std::vector<std::string> records;
	if (readLastRecordsFromData(records) == false)
	{
		return false;
	}

	float time = (float)atof(records[0].c_str());
	if (realTime_ == time)
	{
		return false;
	}

	angle_.x = (float)atof(records[rollIndex_].c_str());
	angle_.y = (float)atof(records[pitchIndex_].c_str());

	realTime_ = time;
	return true;
}

bool ACServoMotionXPlane11::initialize(const std::string& dataFilePath)
{
	ifstream fs(dataFilePath);
	if (fs.is_open() == false)
		return false;

	fs.seekg(2, ios_base::cur);

	std::string header;
	getline(fs, header);

	std::vector<std::string> column = parsingData(header);
	numRecords_ = (int)column.size();

	auto itPitch = std::find(column.cbegin(), column.cend(), "pitch,__deg");
	auto itRoll = std::find(column.cbegin(), column.cend(), "_roll,__deg");

	if (itPitch == column.cend() || itRoll == column.cend())
		return false;

	dataFilePath_ = dataFilePath;

	pitchIndex_ = (int)std::distance(column.cbegin(), itPitch);
	rollIndex_ = (int)std::distance(column.cbegin(), itRoll);

	return true;
}

std::vector<std::string> ACServoMotionXPlane11::parsingData(const std::string& data)
{
	const std::string delimiter = "|";
	std::string d = data;
	std::vector<std::string> records;

	d.erase(remove(d.begin(), d.end(), ' '), d.end());

	size_t pos = 0;
	while ((pos = d.find(delimiter)) != std::string::npos)
	{
		std::string token = d.substr(0, pos);
		records.push_back(token);

		d.erase(0, pos + delimiter.length());
	}

	return records;
}

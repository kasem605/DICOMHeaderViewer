#pragma once

#include <QString>
#include <vector>
#include <cstdint>

struct DicomVoiLut
{
	QString explanation;

	// Number of entries in the LUT
	uint32_t numberOfEntries = 0;

	//First input value mapped by the LUT
	int firstMappedValue = 0;

	//Number of bits per LUT entry
	uint16_t bitsPerEntry = 0;

	// Actual LUT entry values
	std::vector<uint16_t> lutData;
};

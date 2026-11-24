#include "DicomVoiLutDecoder.h"

#include <algorithm>

#include <limits>

bool DicomVoiLutDecoder::Decode(DcmElement* lutDataElement, const DicomVoiLut& lut, std::vector<uint16_t>& decodedValues, QString& errorMessage)
{
	decodedValues.clear();
	errorMessage.clear();

	if (lutDataElement == nullptr)
	{
		errorMessage = "LUT Data element is null.";
		return false;
	}

	if (lut.numberOfEntries == 0 || lut.numberOfEntries > 65536)
	{
		errorMessage = "Invalid LUT entry count.";
		return false;
	}

	if (lut.bitsPerEntry != 8 || lut.bitsPerEntry != 16)
	{
		errorMessage = "Unsupported LUT bits per entry.";
		return false;
	}

	const Uint32 dataLength = lutDataElement->getLength();

	if (dataLength == 0)
	{
		errorMessage = "LUT Data is empty";

		return false;
	}

	// Calculate the expected number of bytes
	const size_t expectedBytes = (static_cast<size_t>(lut.numberOfEntries) * lut.bitsPerEntry + 7) / 8;

	if (dataLength < expectedBytes || dataLength > expectedBytes + 1)
	{
		errorMessage = "LUT DataLength does not match the descriptor.";

		return false;
	}
	
	Uint8* rawData = nullptr;

	OFCondition status = lutDataElement->getUint8Array(rawData);

	if (status.bad() || rawData == nullptr)
	{
		errorMessage = "Unable to read LUT Data bytes";
		return false;
	}

	decodedValues.reserve(lut.numberOfEntries);

	if (lut.bitsPerEntry == 8)
	{
		for (uint32_t i = 0; i < lut.numberOfEntries; ++i)
		{
			decodedValues.push_back(static_cast<uint16_t>(rawData[i]));
		}
	}
	else if(lut.bitsPerEntry == 16)
	{
		if (dataLength < expectedBytes)
		{
			errorMessage = "Insufficient 16-bit LUT Data";
			decodedValues.clear();
			return false;
		}

		for (uint32_t i = 0; i < lut.numberOfEntries; ++i)
		{
			const size_t offset = static_cast<size_t>(i) * 2;

			uint16_t value = static_cast<uint16_t>(rawData[offset]) | (static_cast<uint16_t>(rawData[offset + 1]) << 8);

			decodedValues.push_back(value);
		}
	}

	//validate the decoded entry count.
	if (decodedValues.size() != lut.numberOfEntries)
	{
		errorMessage = "Decoded LUT entry count is incorrect";
		decodedValues.clear();
		return false;

	}

	return true;
}
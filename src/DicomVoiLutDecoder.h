#pragma once

#include "DicomVoiLut.h"

#include <dcmtk/dcmdata/dcelem.h>
#include <QString>

class DicomVoiLutDecoder
{
public:
	static bool Decode(DcmElement* lutDataElement, const DicomVoiLut& lut, std::vector<uint16_t>& decodedValues, QString& errorMessage);
};
#pragma once

#include "DicomHeaderEntry.h"

#include <QString>
#include <vector>
#include <QImage>
#include <cstdint>

class DcmItem;

class DicomHeaderReader
{
public:
	bool LoadFile(
		const QString& filePath,
		QString& errorMessage
	);

	bool  LoadImage(
		const QString& filePath,
		QImage& image,
		QString& errorMessage
	);

	bool LoadRawPixelData(
		const QString& filePathg,
		std::vector<uint16_t>& pixels,
		unsigned long& width,
		unsigned long& height,
		QString& errorMessage
	);

	const std::vector<DicomHeaderEntry>& Entries() const;


private:

	std::vector<DicomHeaderEntry> entries_;

	void ReadItem(
		DcmItem* item,
		int level,
		const QString& section
	);
};
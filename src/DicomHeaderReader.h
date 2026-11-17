#pragma once

#include "DicomHeaderEntry.h"

#include <QString>
#include <vector>

class DcmItem;

class DicomHeaderReader
{
public:
	bool LoadFile(
		const QString& filePath,
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
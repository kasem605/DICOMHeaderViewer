#pragma once

#include <QString>

struct DicomHeaderEntry
{
	QString section;
	QString name;
	QString tag;
	QString vr;
	QString value;

	int level = 0;
};

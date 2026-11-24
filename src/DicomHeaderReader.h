#pragma once

#include "DicomHeaderEntry.h"
#include "DicomVoiLut.h"

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


	bool GetWindowCenter(const QString& filePath, double& windowCenter, QString& errorMessage);

	bool GetWindowWidth(const QString& filePath, double& windowWidth, QString& errorMessage);

	bool GetPhotometricInterpretation(const QString& filePath, QString& photometricInterpretation, QString& errorMessage);

	bool GetWindowCenters(const QString& filePath, std::vector<double>& windowCenters, QString& errorMessage);

	bool GetWindowWidths(const QString& filePath, std::vector<double>& windowWidths, QString& errorMessage);

	bool GetWindowExplanations(const QString& filePath, std::vector<QString>& explanations, QString& errorMessage);

	bool HasVoiLutSequence(const QString& filePath, bool& hasVoiLut, QString& errorMessage);

	bool GetVoiLuts(const QString& filePath, std::vector<DicomVoiLut>& voiLuts, QString& errorMessage);


private:

	std::vector<DicomHeaderEntry> entries_;

	void ReadItem(
		DcmItem* item,
		int level,
		const QString& section
	);
};
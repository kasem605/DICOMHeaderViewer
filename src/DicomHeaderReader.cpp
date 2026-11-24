#include "DicomHeaderReader.h"

#include <dcmtk/dcmdata/dctk.h>

#include <dcmtk/dcmimgle/dcmimage.h>

#include <QFile>

#include <QByteArray>

#include <QDebug>

#include <cmath>

#include <QStringList>

#include <dcmtk/dcmdata/dcsequen.h>




QString FormatTag(const DcmTag& tag)
{
	return QString("(%1,%2)")
		.arg(tag.getGroup(), 4, 16, QChar('0'))
		.arg(tag.getElement(), 4, 16, QChar('0'))
	.toUpper();
}

QString GetElementValue(DcmElement* element)
{
	const DcmTag& tag = element->getTag();	

	//do not display raw pixel data
	if (tag.getGroup() == 0x7FE0 && tag.getElement() == 0x0010)
	{
		return QString("<Pixel Data %1 bytes>")
			.arg(element->getLength());
	}

	OFString value;

	OFCondition status = element->getOFStringArray(value);

	if (status.bad())
	{
		return QString("<Binary or Unavailable>");
	}

	QString result = QString::fromLocal8Bit(value.c_str(), static_cast<qsizetype>(value.length()));
		
	constexpr qsizetype maxLength = 4096;

	if (result.length() > maxLength)
	{
		result.truncate(maxLength);
		result += "...";
	}
		
	return result;	

}

bool DicomHeaderReader::LoadFile(const QString& filePath, QString& errorMessage)
{
	entries_.clear();
	errorMessage.clear();

	DcmFileFormat fileFormat;

	QByteArray encodedPath = QFile::encodeName(filePath);

	OFCondition status = fileFormat.loadFile(encodedPath.constData());

	if (status.bad())
	{
		errorMessage = QString::fromLocal8Bit(status.text());
		return false;
	}

	// Read Dicom metadata
	ReadItem(fileFormat.getMetaInfo(), 0, "File Meta Information");

	// Read main DICOM dataset
	ReadItem(fileFormat.getDataset(), 0, "Dataset");

	return true;
}

bool DicomHeaderReader::LoadImage(const QString& filePath, QImage& image, QString& errorMessage)
{
	image = QImage();
	errorMessage.clear();

	QByteArray encodedPath = QFile::encodeName(filePath);

	// Create DCMTK image decoder
	DicomImage dicomImage(encodedPath.constData());

	//check whether decoding succeeded
	if (dicomImage.getStatus() != EIS_Normal)
	{
		errorMessage = "DCMTK could not decode the DICOM image.";
		return false;
	}

	// get image dimensions
	unsigned long width = dicomImage.getWidth();
	unsigned long height = dicomImage.getHeight();

	qDebug() << "DCMTK image width:" << width;
	qDebug() << "DCMTK image height:" << height;
	qDebug() << "DCMTK frame count:" << dicomImage.getFrameCount();

	if (width == 0 || height == 0)
	{
		errorMessage = "Invalod DICOM Image dimensions";
		return false;
	}

	// Convert pixel data to 8-bit grayscale
	const void* pixelData = dicomImage.getOutputData(8);

	if (pixelData == nullptr)
	{
		errorMessage = "Unable to extract DICOM pixel data";
		return false;
	}

	//Create Qt image and copy the pixel data
	QImage decodedImage(
		static_cast<const uchar*> (pixelData),
		static_cast<int> (width),
		static_cast<int> (height),
		QImage::Format_Grayscale8
	);

	image = decodedImage.copy();

	return true;
}

bool DicomHeaderReader::LoadRawPixelData(
	const QString& filePath,
	std::vector<uint16_t>& pixels,
	unsigned long& width,
	unsigned long& height,
	QString& errorMessage)
{
	pixels.clear();
	width = 0;
	height = 0;
	errorMessage.clear();

	QByteArray encodedPath = QFile::encodeName(filePath);

	DcmFileFormat fileFormat;

	OFCondition status = fileFormat.loadFile(encodedPath.constData());

	if (status.bad())
	{
		errorMessage = QString::fromLocal8Bit(status.text());
		return false;
	}

	DcmDataset* dataset = fileFormat.getDataset();

	Uint16 rows = 0;
	Uint16 columns = 0;
	Uint16 bitsAllocated = 0;
	Uint16 bitsStored = 0;
	Uint16 pixelRepresentation = 0;

	if (dataset->findAndGetUint16(DCM_Rows, rows).bad() ||
		dataset->findAndGetUint16(DCM_Columns, columns).bad() ||
		dataset->findAndGetUint16(DCM_BitsAllocated, bitsAllocated).bad() ||
		dataset->findAndGetUint16(DCM_BitsStored, bitsStored).bad() ||
		dataset->findAndGetUint16(DCM_PixelRepresentation, pixelRepresentation).bad())
	{
		errorMessage = "Unable to read required image attributes.";
		return false;
	}

	if (bitsAllocated != 16 || pixelRepresentation != 0)
	{
		errorMessage = "This reader currently supports unsigned 16-bit allocated pixels only.";
		return false;
	}

	const Uint16* pixelData = nullptr;

	status = dataset->findAndGetUint16Array(DCM_PixelData, pixelData);

	if (status.bad() || pixelData == nullptr) 
	{
		errorMessage = "Unable to read uncompressed Pixel data.";
		return false;
	}

	const size_t pixelCount = static_cast<size_t>(rows) * columns;



	pixels.reserve(pixelCount);

	for (size_t i = 0; i < pixelCount; ++i)
	{
		pixels.push_back(static_cast<uint16_t>(pixelData[i]));
	}

	width = columns;
	height = rows;

	qDebug() << "Rows:" << rows;
	qDebug() << "Columns:" << columns;
	qDebug() << "Bits Allocated:" << bitsAllocated;
	qDebug() << "Bits Stored:" << bitsStored;
	qDebug() << "Pixel Representation:" << pixelRepresentation;

	return true;
}

bool DicomHeaderReader::GetWindowCenter(const QString& filePath, double& windowCenter, QString& errorMessage)
{
	windowCenter = 0.0;
	errorMessage.clear();

	QByteArray encodedPath = QFile::encodeName(filePath);

	DcmFileFormat fileFormat;

	OFCondition status = fileFormat.loadFile(encodedPath.constData());

	if (status.bad())
	{
		errorMessage = QString::fromLocal8Bit(status.text());
		return false;
	}

	OFString value;

	status = fileFormat.getDataset()->findAndGetOFString(
		DCM_WindowCenter,
		value
	);

	if (status.bad())
	{
		errorMessage = "Window center not found.";
		return false;
	}

	bool ok = false;

	windowCenter = QString::fromLatin1(value.c_str()).toDouble(&ok);

	if (!ok)
	{
		errorMessage = "Invalid Window Center value.";
		return false;
	}

	return true;
}

bool DicomHeaderReader::GetWindowWidth(const QString& filePath, double& windowWidth, QString& errorMessage)
{
	windowWidth = 0.0;
	errorMessage.clear();

	QByteArray encodedPath = QFile::encodeName(filePath);

	DcmFileFormat fileFormat;

	OFCondition status = fileFormat.loadFile(encodedPath.constData());

	if (status.bad())
	{
		errorMessage = QString::fromLocal8Bit(status.text());
		return false;
	}

	OFString value;

	status = fileFormat.getDataset()->findAndGetOFString(
		DCM_WindowWidth,
		value
	);

	if (status.bad())
	{
		errorMessage = "Window Width not found.";
		return false;
	}

	bool ok = false;

	windowWidth = QString::fromLatin1(value.c_str()).toDouble(&ok);

	if (!ok || windowWidth <= 0.0)
	{
		errorMessage = "Invalid Window Width value.";
		return false;
	}

	return true;
}

const std::vector<DicomHeaderEntry>& DicomHeaderReader::Entries() const
{
	return entries_;
}

void DicomHeaderReader::ReadItem(DcmItem* item, int level, const QString& section)
{
	if (item == nullptr)
	{
		qDebug() << "Null DICOM Item: " << section;
		return;
	}


	for (unsigned long i = 0; i < item->card(); i++)
	{
		DcmElement* element = item->getElement(i);

		if (element==nullptr)
			continue;

		qDebug() << "Reading Element: " << i;

		DcmTag tag = element->getTag();

		DicomHeaderEntry entry;

		entry.section = section;
		entry.level = level;
		entry.tag = FormatTag(tag);
		entry.name = QString::fromLatin1(tag.getTagName());

		entry.vr = QString::fromLatin1(DcmVR(element->ident()).getVRName());

		//determine if the element is a sequence

		DcmSequenceOfItems* sequence = dynamic_cast<DcmSequenceOfItems*>(element);

		if (sequence != nullptr)
		{
			entry.value = QString("<Sequence: %1 items>").arg(sequence->card());

			entries_.push_back(entry);

			// Read each nested sequence item
			for (unsigned long j = 0; j<sequence->card(); ++j)
			{
				ReadItem(sequence->getItem(j), level + 1, section);
			}
		}
		else
		{
			entry.value = GetElementValue(element);

			entries_.push_back(entry);

		}
	}

}

bool DicomHeaderReader::GetPhotometricInterpretation(const QString& filePath, QString& photometricInterpretation, QString& errorMessage)
{
	photometricInterpretation.clear();
	errorMessage.clear();

	QByteArray encodedPath = QFile::encodeName(filePath);

	DcmFileFormat fileFormat;

	OFCondition status = fileFormat.loadFile(encodedPath.constData());

	if (status.bad())
	{
		errorMessage = QString::fromLocal8Bit(status.text());

		return false;
	}

	OFString value;

	status = fileFormat.getDataset()->findAndGetOFString(DCM_PhotometricInterpretation, value);

	if (status.bad())
	{
		errorMessage = "Photometric Interpretation not found.";

		return false;
	}

	photometricInterpretation = QString::fromLatin1(value.c_str()).trimmed();

	return true;
}

bool DicomHeaderReader::GetWindowWidths(const QString& filePath, std::vector<double>& windowWidths, QString& errorMessage)
{
	windowWidths.clear();
	errorMessage.clear();

	QByteArray encodedPath = QFile::encodeName(filePath);

	DcmFileFormat fileFormat;

	OFCondition status = fileFormat.loadFile(encodedPath.constData());

	if (status.bad())
	{
		errorMessage = QString::fromLocal8Bit(status.text());

		return false;
	}

	OFString value;

	status = fileFormat.getDataset()->findAndGetOFStringArray(DCM_WindowWidth, value);

	if (status.bad())
	{
		errorMessage = "Window width not found.";

		return false;
	}

	// DICOM stores multiple values separated by backslashes

	QStringList values = QString::fromLatin1(value.c_str()).split('\\');

	for (const QString& item : values)
	{
		bool ok = false;

		double width = item.trimmed().toDouble(&ok);

		if (!ok || !std::isfinite(width) || width < 1.0)
		{
			windowWidths.clear();
			errorMessage = "Invalid DICOM Window Width value.";
			return false;
		}

		windowWidths.push_back(width);
	}
	return !windowWidths.empty();
}

bool DicomHeaderReader::GetWindowCenters(const QString& filePath, std::vector<double>& windowCenters, QString& errorMessage)
{
	windowCenters.clear();
	errorMessage.clear();

	QByteArray encodedPath = QFile::encodeName(filePath);

	DcmFileFormat fileFormat;

	OFCondition status = fileFormat.loadFile(encodedPath.constData());

	if (status.bad())
	{
		errorMessage = QString::fromLocal8Bit(status.text());

		return false;
	}

	OFString value;

	status = fileFormat.getDataset()->findAndGetOFStringArray(DCM_WindowCenter, value);

	if (status.bad())
	{
		errorMessage = "Window center not found.";

		return false;
	}

	// DICOM stores multiple values separated by backslashes

	QStringList values = QString::fromLatin1(value.c_str()).split('\\');

	for (const QString& item : values)
	{
		bool ok = false;

		double center = item.trimmed().toDouble(&ok);

		if (!ok || !std::isfinite(center) || center < 1.0)
		{
			windowCenters.clear();
			errorMessage = "Invalid DICOM Window Center value.";
			return false;
		}

		windowCenters.push_back(center);
	}
	return !windowCenters.empty();
}

bool DicomHeaderReader::GetWindowExplanations(const QString& filePath, std::vector<QString>& explanations, QString& errorMessage)
{
	explanations.clear();
	errorMessage.clear();

	QByteArray encodedPath = QFile::encodeName(filePath);

	DcmFileFormat fileFormat;

	OFCondition status = fileFormat.loadFile(encodedPath.constData());

	if (status.bad())
	{
		errorMessage = QString::fromLocal8Bit(status.text());

		return false;
	}

	OFString value;

	status = fileFormat.getDataset()->findAndGetOFStringArray(DCM_WindowCenterWidthExplanation, value);

	if (status.bad())
	{
		errorMessage = "Window center/width explanation not found.";

		return false;
	}

	// DICOM stores multiple values separated by backslashes

	QStringList values = QString::fromLatin1(value.c_str()).split('\\');

	for (const QString& item : values)
	{
		explanations.push_back(item.trimmed());
	}
	return !explanations.empty();
}

bool DicomHeaderReader::HasVoiLutSequence(const QString& filePath, bool& hasVoiLut, QString& errorMessage)
{
	hasVoiLut = false;
	errorMessage.clear();

	QByteArray encodedPath = QFile::encodeName(filePath);

	DcmFileFormat fileFormat;

	OFCondition status = fileFormat.loadFile(encodedPath.constData());

	if (status.bad())
	{
		errorMessage = QString::fromLocal8Bit(status.text());

		return false;
	}

	DcmSequenceOfItems* voiLutSequence = nullptr;

	status = fileFormat.getDataset()->findAndGetSequence(DCM_VOILUTSequence, voiLutSequence);

	if (status.bad())
	{
		// the sequence is optional, so it is not an error
		hasVoiLut = false;

		return true;
	}

	hasVoiLut = (voiLutSequence != nullptr);

	return true;
}

bool DicomHeaderReader::GetVoiLuts(const QString& filePath, std::vector<DicomVoiLut>& voiLuts, QString& errorMessage)
{
	voiLuts.clear();
	errorMessage.clear();

	QByteArray encodedPath = QFile::encodeName(filePath);

	DcmFileFormat fileFormat;

	OFCondition status = fileFormat.loadFile(encodedPath.constData());

	if(status.bad())
	{
		errorMessage = QString::fromLocal8Bit(status.text());

		return false;
	}

	DcmSequenceOfItems* sequence = nullptr;

	status = fileFormat.getDataset()->findAndGetSequence(DCM_VOILUTSequence, sequence);

	if (status.bad() || sequence == nullptr)
	{
		return true;
	}

	for (unsigned long i = 0; i < sequence->card(); ++i)
	{
		DcmItem* item = sequence->getItem(i);

		if (item == nullptr)
		{
			continue;
		}

		DicomVoiLut lut;

		//Read LUT descriptor
		Uint16 descriptor[3] = {};

		status = item->findAndGetUint16(DCM_LUTDescriptor, descriptor[0], 0);

		if (status.bad())
		{
			errorMessage = "Unable to read VOI LUT descriptor";
			voiLuts.clear();
			return false;
		}

		status = item->findAndGetUint16(DCM_LUTDescriptor, descriptor[1], 0);

		if (status.bad())
		{
			errorMessage = "Unable to read VOI LUT descriptor";
			voiLuts.clear();
			return false;
		}

		status = item->findAndGetUint16(DCM_LUTDescriptor, descriptor[2], 0);

		if (status.bad())
		{
			errorMessage = "Unable to read VOI LUT descriptor";
			voiLuts.clear();
			return false;
		}

		lut.numberOfEntries = descriptor[0] == 0 ? 65536 : descriptor[0];

        // validate the number of entries
		if (lut.numberOfEntries == 0 || lut.numberOfEntries > 65536)
		{
			errorMessage = "Unable to read LUT first mapped value";
			voiLuts.clear();
			return false;
		}


		//Read pixel representation from the parent dataset
		Uint16 pixelRepresentation = 0;

		status = fileFormat.getDataset()->findAndGetUint16(DCM_PixelRepresentation, pixelRepresentation);

		if (status.bad() || pixelRepresentation > 1)
		{
			errorMessage = "Invalid or missing Pixel Representation";
			voiLuts.clear();
			return false;
		}

		// Interpret the second LUT Descriptor value correctly.
		if (pixelRepresentation == 0)
		{
			Uint16 firstMappedValue = 0;

			status = item->findAndGetUint16(DCM_LUTDescriptor, firstMappedValue, 1);

			if (status.bad())
			{
				errorMessage = "Unable to read LUT first mapped value";
				voiLuts.clear();
				return false;
			}

			lut.firstMappedValue = firstMappedValue;
		}
		else
		{
			Sint16 firstMappedValue = 0;

			status = item->findAndGetSint16(DCM_LUTDescriptor, firstMappedValue);

			if (status.bad())
			{
				errorMessage = "Unable to read LUT signed first mapped value";
				voiLuts.clear();
				return false;
			}

			lut.firstMappedValue = firstMappedValue;

		}

		lut.bitsPerEntry = descriptor[2];

		//Read the optional LUT explanation
		OFString explanation;

		if (item->findAndGetOFString(DCM_LUTExplanation, explanation).good())
		{
			lut.explanation = QString::fromLatin1(explanation.c_str()).trimmed();
		}

		// Read LUT data
		DcmElement* lutDataElement = nullptr;

		status = item->findAndGetElement(DCM_LUTData, lutDataElement);

		if (status.bad() || lutDataElement == nullptr)
		{
			errorMessage = "Unable to read VOI LUT data";
			voiLuts.clear();
			return false;
		}


		// Determine the actual encoded length in bytes
		const Uint32 dataLength = lutDataElement->getLength();

		if (dataLength == 0)
		{
			errorMessage = "VOI LUT is empty";
			voiLuts.clear();
			return false;
		}

		Uint16* lutData16 = nullptr;

		status = lutDataElement->getUint16Array(lutData16);

		if (status.bad() || lutData16 == nullptr)
		{
			errorMessage = "Unable to extract VOI LUT data";
			voiLuts.clear();
			return false;
		}

		//calculate the number of available 16-bit words
		const size_t wordCount = dataLength / sizeof(Uint16);

		if (wordCount < lut.numberOfEntries)
		{
			errorMessage = "VOI LUT data contains insufficient entries";
			voiLuts.clear();
			return false;
		}
		// determine the actual number of 16 bit words available
		unsigned long actualWordCount = lutDataElement->getLength() / 2;

		if (actualWordCount < lut.numberOfEntries)
		{
			errorMessage = "VOI LUT data contains fewer entries than expected.";
			voiLuts.clear();
			return false;
		}

		// copy LUT values
		lut.lutData.reserve(lut.numberOfEntries);

		for (int32_t j = 0; j < lut.numberOfEntries; ++j)
		{
			uint16_t value = lutData16[j];

			// keep the number of significant bits specified by the descriptor
			if (lut.bitsPerEntry < 16)
			{
				const uint16_t mask = static_cast<uint16_t>((1u << lut.bitsPerEntry) - 1u);

				value &= mask;
			}

			lut.lutData.push_back(value);
		}
	}

	return true;
};



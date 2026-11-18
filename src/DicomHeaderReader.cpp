#include "DicomHeaderReader.h"

#include <dcmtk/dcmdata/dctk.h>

#include <dcmtk/dcmimgle/dcmimage.h>

#include <QFile>

#include <QByteArray>

#include <QDebug>



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

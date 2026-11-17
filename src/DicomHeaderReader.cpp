#include "DicomHeaderReader.h"

#include <dcmtk/dcmdata/dctk.h>

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

#include "DicomImageView.h"

#include <QPainter>
#include <QPaintEvent>
#include <QSizePolicy>

DicomImageView::DicomImageView(QWidget* parent) :QWidget(parent)
{
	setMinimumSize(300, 300);

	setSizePolicy(
		QSizePolicy::Expanding,
		QSizePolicy::Expanding
	);
}

void DicomImageView::SetImage(const QImage& image)
{
	image_ = image;

	update();
}

void DicomImageView::paintEvent(QPaintEvent* event)
{
	Q_UNUSED(event);

	QPainter painter(this);

	//paint the background
	painter.fillRect(rect(), Qt::black);

	// Nothing to display
	if (rawPixels_.empty() || imageWidth_ <= 0 || imageHeight_ <= 0)
	{
		return;
	}


	// calculate window boundaries
	double lower = windowLevel_ - windowWidth_ / 2.0;
	double upper = windowLevel_ + windowWidth_ / 2.0;

	// create grayscale image with adjusted intensity
	QImage adjustedImage(imageWidth_, imageHeight_, QImage::Format_Grayscale8);


	for (int y = 0; y < imageHeight_; ++y)
	{
		uchar* destination = adjustedImage.scanLine(y);

		for (int x = 0; x < imageWidth_; ++x)
		{
			size_t index = static_cast<size_t>(y) * imageWidth_ + x;

			double pixel = rawPixels_[index];

			double value = ((pixel - lower) / (upper - lower)) * 255.0;

			value = qBound(0.0, value, 255.0);

			destination[x] = static_cast<uchar>(value);
		}
	}

	//scale image while preserving aspect ration

	QImage scaledImage = adjustedImage.scaled(
		size(),
		Qt::KeepAspectRatio,
		Qt::SmoothTransformation
	);

	// center image
	int x = (width() - scaledImage.width()) / 2;
	int y = (height() - scaledImage.height()) / 2;

	painter.drawImage(x, y, scaledImage);
}

void DicomImageView::SetWindowWidth(double width)
{
	windowWidth_ = qMax(1.0, width);
	update();
}

void DicomImageView::SetWindowLevel(double level)
{
	windowLevel_ = level;
	update();
}

void DicomImageView::SetRawImage(const std::vector<uint16_t>& pixels, int width, int height)
{
	if (width <= 0 || height < 0)
		return;

	const size_t expectedSize = static_cast<size_t>(width) * height;

	if (pixels.size() != expectedSize)
		return;

	rawPixels_ = pixels;
	imageWidth_ = width;
	imageHeight_ = height;

	update();
}
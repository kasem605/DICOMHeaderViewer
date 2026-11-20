#include "DicomImageView.h"

#include <QPainter>
#include <QPaintEvent>
#include <QSizePolicy>
#include <cmath>

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
	double center = windowLevel_;
	double windowWidth = windowWidth_;

	if (!std::isfinite(center) || !std::isfinite(windowWidth) || windowWidth < 1.0)
	{
		return;
	}

	double lower = center - 0.5 - (windowWidth - 1.0) / 2.0;
	double upper = center - 0.5 + (windowWidth - 1.0) / 2.0;

	// create grayscale image with adjusted intensity
	QImage adjustedImage(imageWidth_, imageHeight_, QImage::Format_Grayscale8);


	for (int y = 0; y < imageHeight_; ++y)
	{
		uchar* destination = adjustedImage.scanLine(y);

		for (int x = 0; x < imageWidth_; ++x)
		{
			size_t index = static_cast<size_t>(y) * imageWidth_ + x;

			double pixel = rawPixels_[index];

			double value;

			if (pixel <= lower)
			{
				value = 0.0;
			}
			else if(pixel > upper)
			{
				value = 255.0;
			}
			else
			{
				value = ((pixel - (center - 0.5)) / (windowWidth - 1.0) + 0.5) * 255.0;
			}


			value = qBound(0.0, value, 255.0);

			if (monochrome1_)
			{
				value = 255.0 - value;
			}

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

void DicomImageView::SetDicomWindowLevel(double center, double width)
{
	if (!std::isfinite(center) || !std::isfinite(width) || width < 1.0)
	{
		return;
	}

	windowLevel_ = center;
	windowWidth_ = width;

	update();
}

void DicomImageView::SetRawImage(const std::vector<uint16_t>& pixels, int width, int height)
{
	if (width <= 0 || height <= 0)
		return;

	const size_t expectedSize = static_cast<size_t>(width) * height;

	if (pixels.size() != expectedSize)
		return;

	rawPixels_ = pixels;
	imageWidth_ = width;
	imageHeight_ = height;

	update();
}

void DicomImageView::AutoWindowLevel()
{
	if (rawPixels_.empty())
		return;

	// Make a copy so the original pixel data remains unchanged.
	std::vector<uint16_t> sortedPixels = rawPixels_;

	std::sort(sortedPixels.begin(), sortedPixels.end());

	//calculate the 1st and 99th percentile positions.
	size_t lowerIndex = static_cast<size_t>(sortedPixels.size() * 0.01);

	size_t upperIndex = static_cast<size_t>(sortedPixels.size() * 0.99);

	double lower = sortedPixels[lowerIndex];
	double upper = sortedPixels[upperIndex];

	// prevent an invalid window
	if (upper <= lower)
		return;

	windowWidth_ = upper - lower;
	windowLevel_ = (upper + lower) / 2.0;

	update();
}


double DicomImageView::GetWindowWidth() const
{
	return windowWidth_;
}

double DicomImageView::GetWindowLevel() const
{
	return windowLevel_;
}

void DicomImageView::ResetWindowLevel()
{
	AutoWindowLevel();
}

void DicomImageView::SetMonochrome1(bool monochrome1)
{
	monochrome1_ = monochrome1;
	update();
}

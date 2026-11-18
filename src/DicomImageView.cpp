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

	if (!painter.isActive())
	{
		qDebug() << "QPainter could not be activated";

		return;
	}

	//paint the background
	painter.fillRect(rect(), Qt::black);

	// Nothing to display
	if (image_.isNull())
		return;

	// scale image while aspect ratio is preserved
	QImage scaledImage = image_.scaled(
		size(),
		Qt::KeepAspectRatio,
		Qt::SmoothTransformation
	);

	//center the image inside the widget
	int x = (width() - scaledImage.width()) / 2;
	int y = (height() - scaledImage.height()) / 2;

	painter.drawImage(x, y, scaledImage);
}
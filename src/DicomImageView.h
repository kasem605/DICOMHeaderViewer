#pragma once

#include <QWidget>
#include <QImage>

class DicomImageView : public QWidget
{
	Q_OBJECT

public:
	explicit DicomImageView(QWidget* parent = nullptr);

	void SetImage(const QImage& image);

	void SetWindowWidth(double width);

	void SetWindowLevel(double level);

protected:
	void paintEvent(QPaintEvent* event) override;

private:
	QImage image_;
	double windowWidth_ = 256.0;
	double windowLevel_ = 128.0;
};
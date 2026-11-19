#pragma once

#include <QWidget>
#include <QImage>
#include <cstdint>
#include <vector>

class DicomImageView : public QWidget
{
	Q_OBJECT

public:
	explicit DicomImageView(QWidget* parent = nullptr);

	void SetImage(const QImage& image);

	void SetWindowWidth(double width);

	void SetWindowLevel(double level);

	void SetRawImage(const std::vector<uint16_t>& pixels, int width, int height)
		;
protected:
	void paintEvent(QPaintEvent* event) override;

private:
	QImage image_;
	double windowWidth_ = 3916.0;
	double windowLevel_ = 1958.0;

	std::vector<uint16_t> rawPixels_;

	int imageWidth_ = 0;
	int imageHeight_ = 0;

};
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

	double GetWindowWidth() const;

	void SetWindowLevel(double level);

	double GetWindowLevel() const;

	void AutoWindowLevel();

	void SetRawImage(const std::vector<uint16_t>& pixels, int width, int height);

	void ResetWindowLevel();

	void SetDicomWindowLevel(double windowLevel, double windowCenter);

	void SetMonochrome1(bool monochrome1);

	bool PhotometricInterpretation(const QString& filePath, QString& photometricInterpretation, QString& errorMessage);

protected:
	void paintEvent(QPaintEvent* event) override;

private:
	QImage image_;
	double windowWidth_ = 3916.0;
	double windowLevel_ = 1958.0;

	std::vector<uint16_t> rawPixels_;

	int imageWidth_ = 0;
	int imageHeight_ = 0;

	bool monochrome1_;

};
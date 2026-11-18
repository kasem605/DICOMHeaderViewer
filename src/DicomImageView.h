#pragma once

#include <QWidget>
#include <QImage>

class DicomImageView : public QWidget
{
	Q_OBJECT

public:
	explicit DicomImageView(QWidget* parent = nullptr);

	void SetImage(const QImage& image);

protected:
	void paintEvent(QPaintEvent* event) override;

private:
	QImage image_;
};
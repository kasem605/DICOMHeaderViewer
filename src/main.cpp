
#include <QApplication>
#include <QDebug>
#include <QCoreApplication>
#include <QTableWidget>
#include <qheaderview.h>
#include <QMessageBox>
#include <QStringList>

#include <QLineEdit>
#include <QWidget>
#include <QLabel>

#include <QFileDialog>

#include <QPixmap>
#include <QHBoxLayout>
#include <QScrollArea>

#include <QSlider>
#include <QBoxLayout>
#include <QComboBox>

#include <algorithm>
#include <vector>
#include <cstdint>

#include <QPushButton>
#include <cmath>

#include "DicomHeaderReader.h"
#include "DicomImageView.h"

int main(int argc, char* argv[])
{
    // Tell Qt where its platform plugins are located.
    QCoreApplication::addLibraryPath(
        "C:/Qt/6.12.0/msvc2022_64/plugins"
    );

    QApplication app(argc, argv);

    DicomHeaderReader reader;

    QString errorMessage;

    QString filePath = QFileDialog::getOpenFileName(nullptr, 
                                                    "select DICOM File",
                                                    "D:/RSNA knee abnormality detection 2026",
                                                    "DICOM Files (*.dcm);;All Files (*.*)");
     
    // check if the user selects a file
    if (filePath.isEmpty())
    {
        qDebug() << "NoDICOM file is selected.";
        return 0;
    }

    if (!reader.LoadFile(filePath, errorMessage))
    {
        QMessageBox::critical(nullptr, "DICOM Loading Error", errorMessage);

        return -1;
    }

    std::vector<uint16_t> rawPixels;
    unsigned long rawWidth = 0;
    unsigned long rawHeight = 0;
    QString rawError;

    if (!reader.LoadRawPixelData(
        filePath,
        rawPixels,
        rawWidth,
        rawHeight,
        rawError
    ))
    {
        QMessageBox::critical(nullptr, "Pixel Data Error", rawError);

        return -1;
    }

    auto [minPixel, maxPixel] = std::minmax_element(rawPixels.begin(), rawPixels.end());

    qDebug() << "Raw pixel width:" << rawWidth;
    qDebug() << "Raw pixel height:" << rawHeight;
    qDebug() << "Pixel count:" << rawPixels.size();
    qDebug() << "Minimum pixel value:" << *minPixel;
    qDebug() << "Maximum pixel value:" << *maxPixel;
    qDebug() << "First 10 pixel values:";

    for (size_t i = 0; i < 10; ++i)
    {
        qDebug() << rawPixels[i];
    }


    qDebug() << "DICOM file load successfully:";
    qDebug() << filePath;

    QImage dicomImage;

    QString imageError;

    if (!reader.LoadImage(filePath,dicomImage, imageError))
    {
        QMessageBox::critical(nullptr, "Image loading error", imageError);

        return -1;
    }


    qDebug() << "Image loaded successfully";
    qDebug() << "Width: " << dicomImage.width();
    qDebug() << "Height: " << dicomImage.height();
    qDebug() << "Format: " << dicomImage.format();


    double dicomWindowCenter = 0.0;
    double dicomWindowWidth = 0.0;

    QString windowError;

    if (reader.GetWindowCenter(
        filePath,
        dicomWindowCenter,
        windowError
    ))
    {
        qDebug() << "DICOM Window Center:" << dicomWindowCenter;
    }
    else
    {
        qDebug() << windowError;
    }

    if (reader.GetWindowWidth(
        filePath,
        dicomWindowWidth,
        windowError
    ))
    {
        qDebug() << "DICOM Window Width:" << dicomWindowWidth;
    }
    else
    {
        qDebug() << windowError;
    }

    DicomImageView* imageView = new DicomImageView();

    imageView->SetRawImage(rawPixels, static_cast<int> (rawWidth), static_cast<int> (rawHeight));

    QString photometricInterpretation;

    QString photometricError;

    if (!reader.GetPhotometricInterpretation(filePath, photometricInterpretation, photometricError))
    {
        QMessageBox::critical(nullptr, "Photometric Interpretation Error", photometricError);

        return -1;
    }

    qDebug() << "Photometric Interpretation:" << photometricInterpretation;

    if (photometricInterpretation == "MONOCHROME1")
    {
        imageView->SetMonochrome1(true);
    }
    else if (photometricInterpretation == "MONOCHROME2")
    {
        imageView->SetMonochrome1(false);
    }
    else
    {
        QMessageBox::critical(nullptr, "Unsupported Image Format", "Unsupported Photometric Interpretation: " + photometricInterpretation);

        return -1;
    }


    imageView->SetDicomWindowLevel(dicomWindowCenter, dicomWindowWidth);

    int initialWidth = static_cast<int>(imageView->GetWindowWidth());

    int initialLevel = static_cast<int>(imageView->GetWindowLevel());


    // windows slider

    QLabel* widthLabel = new QLabel("Window Width");
    QSlider* widthSlider = new QSlider(Qt::Horizontal);

    //window level slider
    QLabel* levelLabel = new QLabel("Window Level");
    QSlider* levelSlider = new QSlider(Qt::Horizontal);

    widthSlider->setRange(1, 4096);
    widthSlider->setValue(initialWidth);

    levelSlider->setRange(0, 4095);
    levelSlider->setValue(initialLevel);


    widthLabel->setText(QString("Window Width: %1").arg(imageView->GetWindowWidth(),0,'f',2));
    levelLabel->setText(QString("Window Level: %1").arg(imageView->GetWindowLevel(),0,'f',2));

    QPushButton* autoWindowButton = new QPushButton("Reset / Auto Window");
    QPushButton* resetDicomWindowButton = new QPushButton("Reset DICOM Window");

    // Create main window container
    QWidget window;
    window.setWindowTitle("Dicom Haeder Viewer");

    // Main Layout
    QVBoxLayout* layout = new QVBoxLayout(&window);

    // Search box
    QLabel* searchLabel = new QLabel("Search DICOM Metadata:");

    QLineEdit* searchBox = new QLineEdit();
    searchBox->setPlaceholderText("Search by Tag, Attribute Name, VR or value ...");

    //Create Horizontal layout
    QHBoxLayout* contentLayout = new QHBoxLayout();

    // Create the metadata table
    QTableWidget* table = new QTableWidget();

    table->setColumnCount(4);

    table->setHorizontalHeaderLabels({
        "Tag",
        "Name",
        "VR",
        "VAlue"
        });

    //set number of rows
    table->setRowCount(static_cast<int>(reader.Entries().size()));

    //populate the table
    int row = 0;

    for (const auto& entry : reader.Entries())
    {
        table->setItem(row, 0, new QTableWidgetItem(entry.tag));

        table->setItem(row, 1, new QTableWidgetItem(entry.name));

        table->setItem(row, 2, new QTableWidgetItem(entry.vr));

        table->setItem(row, 3, new QTableWidgetItem(entry.value));

        ++row;

    }

    // test for window centers and widths
    std::vector<double> windowCenters;
    std::vector<double> windowWidths;

    errorMessage.clear();

    bool centersLoaded = reader.GetWindowCenters(filePath, windowCenters, errorMessage);

    if (!centersLoaded)
    {
        qDebug() << "Windows Centers error:" << errorMessage;
    }
    else
    {
        qDebug() << "WindowCenters:";

        for (double center : windowCenters)
            qDebug() << center;
    }

    bool widthsLoaded = reader.GetWindowWidths(filePath, windowWidths, errorMessage);

    if (!widthsLoaded)
    {
        qDebug() << "Windows Widths error:" << errorMessage;
    }
    else
    {
        qDebug() << "WindowWidths:";

        for (double width : windowWidths)
            qDebug() << width;
    }

    qDebug() << "Number of centers:" << windowCenters.size();
    qDebug() << "Number of widths:" << windowWidths.size();

    if (windowCenters.size() != windowWidths.size())
    {
        QMessageBox::critical(nullptr, "DICOM Windowing", "The number of Window Center values do not math the numver of Winow Width values");

        return 1;
    }

    QComboBox* windowPresetCombo = new QComboBox();

    std::vector<QString> windowExplanations;


    for (size_t i = 0; i < windowCenters.size(); i++)
    {
        QString presetName;

        if (i < windowExplanations.size() && !windowExplanations[i].trimmed().isEmpty())
        {
            presetName = windowExplanations[i].trimmed();
        }
        else
        {
            presetName = QString("Preset %1").arg(i + 1);
        }

        presetName += QString(" (Center: %1, Width: %2)")
            .arg(windowCenters[i])
            .arg(windowWidths[i]);

        windowPresetCombo->addItem(presetName);
    }

    // test window explanations


    bool explanationsLoaded = reader.GetWindowExplanations(filePath, windowExplanations, errorMessage);

    if (explanationsLoaded)
    {
        qDebug() << "Windows Explanations:";

        for (const QString& explanation : windowExplanations)
        {
            qDebug() << explanation;
        }
    }
    else
    {
        qDebug() << "Window Explanations:" << errorMessage;
    }

    bool hasVoiLut = false;

    QString voiLutError;

    if (reader.HasVoiLutSequence(filePath, hasVoiLut, voiLutError))
    {
        qDebug() << "VOI LUT sequence exists:" << hasVoiLut;
    }
    else
    {
        qDebug() << "VOI LUT sequence has error:" << voiLutError;
    }


    // Test LUT
    std::vector<DicomVoiLut> voiLuts;

    if (reader.GetVoiLuts(filePath, voiLuts, voiLutError))
    {
        qDebug() << "Number of VOI LUTs:" << voiLuts.size();
    }
    else
    {
        qDebug() << "VOI LUT error:" << voiLutError;
    }

    //Configure table appearance
    table->setAlternatingRowColors(true);

    table->setSelectionBehavior(QAbstractItemView::SelectRows);

    table->setEditTriggers(QAbstractItemView::NoEditTriggers);

    table->horizontalHeader()->sectionResizeMode(QHeaderView::ResizeToContents);

    table->horizontalHeader()->setStretchLastSection(true);
        
    // add controls to layout
    layout->addWidget(searchLabel);
    layout->addWidget(searchBox);

    // Window/Level controls

    layout->addWidget(widthLabel);
    layout->addWidget(widthSlider);

    layout->addWidget(levelLabel);
    layout->addWidget(levelSlider);

    layout->addWidget(autoWindowButton);
    layout->addWidget(resetDicomWindowButton);
    layout->addWidget(windowPresetCombo);


    contentLayout->addWidget(table, 3);
    contentLayout->addWidget(imageView, 2);

    layout->addLayout(contentLayout);

    window.resize(1200, 750);

    QObject::connect(searchBox, &QLineEdit::textChanged, table, [table](const QString& searchText)
        {
            for (int row = 0; row < table->rowCount(); ++row)
            {
                bool match = false;

                for (int column = 0; column < table->columnCount(); ++column)
                {
                    QTableWidgetItem* item = table->item(row, column);

                    if (item && item->text().contains(searchText, Qt::CaseInsensitive))
                    {
                        match = true;
                        break;
                    }
                }
                table->setRowHidden(row, !match);

            }
        });

    auto UpdateWindowPresetText = [&]()
        {
            int index = windowPresetCombo->currentIndex();

            if (index < 0)
                return;

            windowPresetCombo->setItemText(
                index,
                QString("Preset %1 (Center %2, Width %3)")
                .arg(index + 1)
                .arg(imageView->GetWindowLevel(), 0, 'f', 2)
                .arg(imageView->GetWindowWidth(), 0, 'f', 2)

            );
        };

    QObject::connect(widthSlider, &QSlider::valueChanged, imageView, [&](int value)
        {
            imageView->SetWindowWidth(value);

            widthLabel->setText(QString("Window Width: %1").arg(value));
            UpdateWindowPresetText();

        });

    QObject::connect(levelSlider, &QSlider::valueChanged, imageView, [&](int value)
        {
            imageView->SetWindowLevel(value);

            levelLabel->setText(QString("WIndow Level: %1").arg(value));

            UpdateWindowPresetText();


        });

    QObject::connect(autoWindowButton, &QPushButton::clicked, [&]()
        {
            imageView->ResetWindowLevel();

            double automaticWidth = imageView->GetWindowWidth();

            double automaticLevel = imageView->GetWindowLevel();

            widthSlider->setValue(static_cast<int>(std::round(automaticWidth)));

            levelSlider->setValue(static_cast<int>(std::round(automaticLevel)));

            widthLabel->setText(QString("Window Width: %1").arg(automaticWidth,0,'f',2));

            levelLabel->setText(QString("Window Level: %1").arg(automaticLevel,0,'f',2));

        });

    QObject::connect(resetDicomWindowButton, &QPushButton::clicked, [&]()
        {
            imageView->SetDicomWindowLevel(
                dicomWindowCenter,
                dicomWindowWidth);

            widthSlider->setValue(static_cast<int>(std::round(dicomWindowWidth)));

            levelSlider->setValue(static_cast<int>(std::round(dicomWindowCenter)));

            widthLabel->setText(QString("Window Width : %1").arg(dicomWindowWidth, 0, 'f', 2));

            levelLabel->setText(QString("Window Level: %1").arg(dicomWindowCenter, 0, 'f', 2));

        });

    QObject::connect(windowPresetCombo, &QComboBox::currentIndexChanged, [=](int index)
        {
            if (index < 0 || index >= static_cast<int>(windowCenters.size()))
                return;

            imageView->SetDicomWindowLevel(windowCenters[index], windowWidths[index]);

            widthSlider->setValue(static_cast<int>(std::round(windowWidths[index])));

            levelSlider->setValue(static_cast<int>(std::round(windowCenters[index])));

            widthLabel->setText(QString::number(windowWidths[index], 'f', 2));

            levelLabel->setText(QString::number(windowCenters[index], 'f', 2));

        });




    window.show();

    return app.exec();

}

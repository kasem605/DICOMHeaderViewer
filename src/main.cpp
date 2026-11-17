
#include <QApplication>
#include <QDebug>
#include <QCoreApplication>
#include <QTableWidget>
#include <qheaderview.h>
#include <QMessageBox>
#include <QStringList>
#include "DicomHeaderReader.h"

int main(int argc, char* argv[])
{
    // Tell Qt where its platform plugins are located.
    QCoreApplication::addLibraryPath(
        "C:/Qt/6.12.0/msvc2022_64/plugins"
    );

    QApplication app(argc, argv);

    DicomHeaderReader reader;

    QString errorMessage;

    QString filePath = "D:/RSNA knee abnormality detection 2026/train_series/1.2.826.0.1.3680043.8.498.10004873229099053869093324292195817260/1.2.826.0.1.3680043.8.498.12343110195036213483454091715412333772/1.2.826.0.1.3680043.8.498.10374285052733466977592225981851620435.dcm";

    if (!reader.LoadFile(filePath, errorMessage))
    {
        qDebug() << "Dicom loading failed:";
        qDebug() << errorMessage;

        return -1;
    }

    // Create the metadata table
    QTableWidget table;

    table.setWindowTitle("DICOM Header Viewer");

    table.setColumnCount(4);

    table.setHorizontalHeaderLabels({
        "Tag",
        "Name",
        "VR",
        "VAlue"
        });

    //set number of rows
    table.setRowCount(static_cast<int>(reader.Entries().size()));

    //populate the table
    int row = 0;

    for (const auto& entry : reader.Entries())
    {
        table.setItem(row, 0, new QTableWidgetItem(entry.tag));

        table.setItem(row, 1, new QTableWidgetItem(entry.name));

        table.setItem(row, 2, new QTableWidgetItem(entry.vr));

        table.setItem(row, 3, new QTableWidgetItem(entry.value));

        ++row;

    }


    //Configure table appearance
    table.setAlternatingRowColors(true);

    table.setSelectionBehavior(QAbstractItemView::SelectRows);

    table.setEditTriggers(QAbstractItemView::NoEditTriggers);

    table.horizontalHeader()->sectionResizeMode(QHeaderView::ResizeToContents);

    table.horizontalHeader()->setStretchLastSection(true);
        
    table.resize(1200, 750);

    table.show();

    return app.exec();
}

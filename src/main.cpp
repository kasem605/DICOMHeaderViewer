
#include <QApplication>
#include <QDebug>
#include <QCoreApplication>
#include <QTableWidget>
#include <qheaderview.h>
#include <QMessageBox>
#include <QStringList>

#include <QLineEdit>
#include <QBoxLayout>
#include <QWidget>
#include <QLabel>

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

    // Create main window container
    QWidget window;
    window.setWindowTitle("Dicom Haeder Viewer");

    // Main Layout
    QVBoxLayout* layout = new QVBoxLayout(&window);

    // Search box
    QLabel* searchLabel = new QLabel("Search DICOM Metadata:");

    QLineEdit* searchBox = new QLineEdit();
    searchBox->setPlaceholderText("Search by Tag, Attribute Name, VR or value ...");


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


    //Configure table appearance
    table->setAlternatingRowColors(true);

    table->setSelectionBehavior(QAbstractItemView::SelectRows);

    table->setEditTriggers(QAbstractItemView::NoEditTriggers);

    table->horizontalHeader()->sectionResizeMode(QHeaderView::ResizeToContents);

    table->horizontalHeader()->setStretchLastSection(true);
        
    table->resize(1200, 750);

    table->horizontalHeader()->setStretchLastSection(true);

    // add controls to layout
    layout->addWidget(searchLabel);
    layout->addWidget(searchBox);
    layout->addWidget(table);

    window.resize(1200, 750);

    QObject::connect(searchBox, &QLineEdit::textChanged, table, [table](const QString& searchText)
        {
            for (int row = 0; row < table->rowCount(); ++row)
            {
                bool match = false;

                for (int column = 0; column < table->columnCount(); ++column)
                {
                    QTableWidgetItem* item = table->item(row, column);

                    if (item && item->text().contains(searchText, Qt::CaseSensitive))
                    {
                        match = true;
                        break;
                    }
                }
                table->setRowHidden(row, !match);

            }
        });

    window.show();

    return app.exec();

}

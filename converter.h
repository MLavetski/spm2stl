#ifndef CONVERTER_H
#define CONVERTER_H

#include <QMainWindow>

namespace Ui {
class converter;
}

class converter : public QMainWindow
{
    Q_OBJECT

public:
    explicit converter(QWidget *parent = 0);
    converter(const converter&) = default;
        converter& operator=(const converter&) = default;
    ~converter();

    QString inputfilename;
    QVector<QVector<char> >dataRaw;//[16][524288];//Place for raw data of fields from spm1 files. 2 bytes for each point. Max 256*256 points.
    QVector<QVector<quint16> >dataShort;//[16][262144];//Same data but transformed into 16 bit integers.
    QVector<QVector<float> >dataMuliplied;//[16][262144];//Data after applying z-multiplier.
    qint16 Nx[16],Ny[16];
    char fieldName[16][32], scaleXYname[16][6], scaleZname[16][6];//info from notifications about fields
    float scaleX[16], scaleY[16], scaleZ[16];
    //16 is borderline amount of fields for that file format. The service info won't support more than 32.

private slots:
    void on_BrowseButton_clicked();

    void on_openBut_clicked();

    void on_convBut_clicked();

private:
    Ui::converter *ui;



    void openspm();
};

#endif // CONVERTER_H

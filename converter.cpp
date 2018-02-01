#include "converter.h"
#include "ui_converter.h"
#include <QFileDialog>
#include <QMessageBox>
#include <triangles.h>

converter::converter(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::converter)
{
    ui->setupUi(this);
}

converter::~converter()
{
    delete ui;
}

void converter::on_BrowseButton_clicked()
{
    ui->openBut->setEnabled(false);
    ui->convBut->setEnabled(false);
    ui->fieldselect->clear();
    ui->fieldselect->setEnabled(false);
    inputfilename = QFileDialog::getOpenFileName(this, tr("Открыть изображение")
                                                 ,inputfilename, tr("spm file (*.spm)"));
    ui->lineEditIn->setText(inputfilename);
    if(!(inputfilename.isEmpty()))
        ui->openBut->setEnabled(true);
}

void converter::openspm()
{
    if(inputfilename=="")//Default inputfilename
        {
            QMessageBox MTname;
            MTname.setText("Не выбран .spm файл. Пожалуйста, выберите.");
            MTname.exec();
            inputfilename = QFileDialog::getOpenFileName(this, tr("Открыть .spm файл"),"/", tr("Spm files (*.spm)"));
        }
    QFile spm(inputfilename);//Opening input file
        if(!spm.open(QIODevice::ReadOnly))
            return;
        QDataStream data(&spm);
        qint8 spmType; data>>spmType;//Getting filetype.
        char HEAD_SPM1[223]; //Getting HEAD_SPM1 from file.
        data.readRawData(HEAD_SPM1,223);
        quint16 MaxNx, MaxNy, field_amount=0;
        quint16 what_this;
        memcpy(&MaxNx,HEAD_SPM1+49,2);
        memcpy(&MaxNy,HEAD_SPM1+51,2);
        memcpy(&what_this,HEAD_SPM1+59,2);
        while(what_this)//Subfield what_this contains info about stored fields witch we can use to find amount of those.
        {
            field_amount++;
            what_this &= (what_this-1); //current AND itself-1 to remove the least bit. repeat untill its 0.
        }
        dataRaw.resize(field_amount);
        dataShort.resize(field_amount);
        dataMuliplied.resize(field_amount);

        char notifications[16][336];//Getting all notifications about stored fields of data.
        for(int i=0;i<field_amount;i++)
            data.readRawData(notifications[i], 336);
        for(int i=0; i<field_amount;i++)//Getting that information.
        {
            strncpy(fieldName[i], notifications[i], 32);
            strncpy(scaleXYname[i], notifications[i]+68, 6);
            strncpy(scaleZname[i], notifications[i]+74, 6);
            memcpy(&Nx[i], notifications[i]+34, 2);
            memcpy(&Ny[i], notifications[i]+36, 2);
            memcpy(&scaleX[i], notifications[i]+40, 4);
            memcpy(&scaleY[i], notifications[i]+44, 4);
            memcpy(&scaleZ[i], notifications[i]+48, 4);

            dataRaw[i].resize(Nx[i]*Ny[i]*2);
            dataShort[i].resize(Nx[i]*Ny[i]);
            dataMuliplied[i].resize(Nx[i]*Ny[i]);
        }

        //Getting all raw data
        for(int i=0; i<field_amount;i++)
        {
            for(int j=0;j<(Nx[i]*Ny[i]*2);j++)
            {
                data.readRawData(&dataRaw[i][j], 1);
            }
        }
        for(int i=0; i<field_amount;i++)//Trasform raw data into 16bit integers.
        {
            for(int j=0; j<(Nx[i]*Ny[i]);j++)
            {
                memcpy(&dataShort[i][j], &dataRaw[i][j*2], 2);
            }
        }
        for(int i=0; i<field_amount;i++)//Multiping data by z-multiplier
        {
            for(int j=0; j<(Nx[i]*Ny[i]);j++)
            {
                dataMuliplied[i][j]=dataShort[i][j]*scaleZ[i];
            }
        }
        for(int i=0;i<field_amount;i++)
            ui->fieldselect->insertItem(i, fieldName[i]);
        ui->fieldselect->setEnabled(true);
        ui->convBut->setEnabled(true);
}

void converter::on_openBut_clicked()
{
    openspm();
}

void converter::on_convBut_clicked()
{
    //We have an array of data witch can be repressented as matrix Ny by Nx
    //that function takes points in said matrix in specified order(points MUST be CCW) to represent
    //verticles of triangles, stores them in vector of triangle class objects, calls calcNormal function
    //for each triange, and writes them into binary .stl file
    quint16 index = ui->fieldselect->currentIndex();
    quint16 sizeX = Nx[index], sizeY = Ny[index];
    char header[80];
    for(int i=0;i<80;i++)
    {
        header[i]=0;
    }
    quint32 amountTriangles = (sizeX-1)*(sizeY-1)*2;
    QVector<triangle> triangles(amountTriangles);
    int i=0;//counter for current triangle
    for(int y=0;y<sizeY-1;y++)  //write coordinates into corresponding triangle
    {                           //there are to types of triangles
        for(int x=0;x<sizeX;x++)//flat side up and down
        {
            if(x!=0)//here we writing flat side down triangle
            {
                triangles[i].point3[0] = x*scaleX[index];
                triangles[i].point3[1] = y*scaleY[index];
                triangles[i].point3[2] = dataMuliplied[index][y*Nx[index]+x];
                triangles[i].point2[0] = (x-1)*scaleX[index];
                triangles[i].point2[1] = (y+1)*scaleY[index];
                triangles[i].point2[2] = dataMuliplied[index][(y+1)*Nx[index]+(x-1)];
                triangles[i].point1[0] = x*scaleX[index];
                triangles[i].point1[1] = (y+1)*scaleY[index];
                triangles[i].point1[2] = dataMuliplied[index][(y+1)*Nx[index]+x];
                i++;
            }
            if(x!=sizeX-1)//here we writing flat side up triangle
            {
                triangles[i].point3[0] = x*scaleX[index];
                triangles[i].point3[1] = y*scaleY[index];
                triangles[i].point3[2] = dataMuliplied[index][y*Nx[index]+x];
                triangles[i].point2[0] = x*scaleX[index];
                triangles[i].point2[1] = (y+1)*scaleY[index];
                triangles[i].point2[2] = dataMuliplied[index][(y+1)*Nx[index]+x];
                triangles[i].point1[0] = (x+1)*scaleX[index];
                triangles[i].point1[1] = y*scaleY[index];
                triangles[i].point1[2] = dataMuliplied[index][y*Nx[index]+(x+1)];
                i++;
            }
        }
    }
    for(quint32 j=0;j<amountTriangles;j++)
    {
        triangles[j].calcNorm();
    }
    triangle test[20];
    for(int i=0;i<20;i++)
        test[i] = triangles[i];
    //ok, we got triangles ready, now write them into .stl file
    //first, generate filemane for output file.
    int extPointPos = inputfilename.lastIndexOf(".");
    QString outputfilename = inputfilename.left(extPointPos);
    //then, we open the file with generatated name. If it doesn't exist, create it.
    QFile file(outputfilename + "_" + fieldName[ui->fieldselect->currentIndex()] + ".stl");
    file.open(QIODevice::WriteOnly);
    QDataStream out(&file);   // we will serialize the data into the file
    out.setByteOrder(QDataStream::LittleEndian);
    //write 80-byte header into file. In our case, header is filled with empty symbols.
    out.writeRawData(header, 80);
    //write int with amount of triangles in file
    out.writeRawData((char*)&amountTriangles,4);
    //write each triangle
    for(quint32 i=0;i<amountTriangles;i++)
    {
        //first 3 dimensions of normal vector
        out.writeRawData((char*)&triangles[i].normal[0],4);
        out.writeRawData((char*)&triangles[i].normal[1],4);
        out.writeRawData((char*)&triangles[i].normal[2],4);
        //3 dim. of first point
        out.writeRawData((char*)&triangles[i].point1[0],4);
        out.writeRawData((char*)&triangles[i].point1[1],4);
        out.writeRawData((char*)&triangles[i].point1[2],4);
        //second point
        out.writeRawData((char*)&triangles[i].point2[0],4);
        out.writeRawData((char*)&triangles[i].point2[1],4);
        out.writeRawData((char*)&triangles[i].point2[2],4);
        //third point
        out.writeRawData((char*)&triangles[i].point3[0],4);
        out.writeRawData((char*)&triangles[i].point3[1],4);
        out.writeRawData((char*)&triangles[i].point3[2],4);
        //and the attribyte file count, witch, by default, is zero.
        out.writeRawData((char*)&triangles[i].attrByteCount,2);
    }
    //then we close the output file and send notification of successfull conversion
    file.close();
    QMessageBox yep;
    yep.setText("Selected field was saved to " + outputfilename + "_" + fieldName[ui->fieldselect->currentIndex()] + ".stl");
    yep.exec();
}

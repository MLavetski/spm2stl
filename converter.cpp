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
    ui->convBut->setEnabled(false);
    ui->fieldselect->clear();
    ui->fieldselect->setEnabled(false);
    inputfilename = QFileDialog::getOpenFileName(this, tr("Open file")
                                                 ,inputfilename, tr("spm file (*.spm)"));
    if(inputfilename.isEmpty())
    {
        QMessageBox::critical(this, "Filename was not specified", "Filename for input file was not specified.");
        return;
    }
    ui->lineEditIn->setText(inputfilename);
    bool isOK = openspm();
    if(!isOK)
    {
        return;
    }
    delZeros();
}

bool converter::openspm()
{
    QFile spm(inputfilename);//Opening input file
        if(!spm.open(QIODevice::ReadOnly))
            return false;
        QDataStream data(&spm);
        qint8 spmType; data>>spmType;//Getting filetype.
        if(spmType != 1)
        {
            QMessageBox::critical(this, "Wrong file format", "Opened file has unknown filetype. Program can work with spm2001 file format only. Please, use SurfaceXplorer to convert your file to required format.");
            return false;
        }
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
        wasFixed.resize(field_amount);

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
        ui->fixSurfBut->setEnabled(true);
        return true;
}

void converter::on_convBut_clicked()
{
    //We have an array of data witch can be repressented as matrix Ny by Nx
    //that function takes points in said matrix in specified order(points MUST be CCW) to represent
    //verticles of triangles, stores them in vector of triangle class objects, calls calcNormal function
    //for each triange, and writes them into binary .stl file
    //first, generate filemane for output file.
    int extPointPos = inputfilename.lastIndexOf(".");
    QString outputfilename = inputfilename.left(extPointPos);
    //then, we open the file with generatated name. If it doesn't exist, create it.
    outputfilename = QFileDialog::getSaveFileName(this, tr("Save as")
                                                 ,outputfilename + + "_" + fieldName[ui->fieldselect->currentIndex()], tr("Stereolithography file (*.stl)"));
    if(outputfilename.isEmpty())
    {
        QMessageBox::critical(this, "Filename was not specified", "Filename for output file was not specified. Stopping function.");
        return;
    }
    QFile file(outputfilename);
    file.open(QIODevice::WriteOnly);
    quint16 index = ui->fieldselect->currentIndex();
    quint16 sizeX = Nx[index];
    quint16 sizeY = Ny[index];
    char header[80];
    for(int i=0;i<80;i++)
    {
        header[i]=0;
    }
    quint32 amountTriangles = (sizeX-1)*(sizeY-1)*2;
    QVector<triangle> triangles(amountTriangles);
    int i=0;//counter for current triangle
    double scaleMult = 1000;//value by witch we will divide the x and y coordinates of our points, since most software uses our values as mm instead of mkm.
    float scaleXfixed = scaleX[index]/scaleMult, scaleYfixed = scaleY[index]/scaleMult;
    for(int y=0;y<sizeY-1;y++)  //write coordinates into corresponding triangle
    {                           //there are to types of triangles
        for(int x=0;x<sizeX;x++)//flat side up and down
        {
            if(x!=0)//here we writing flat side down triangle
            {
                triangles[i].point3[0] = x*scaleXfixed;
                triangles[i].point3[1] = y*scaleYfixed;
                triangles[i].point3[2] = dataMuliplied[index][y*Nx[index]+x]/scaleMult;
                triangles[i].point2[0] = (x-1)*scaleXfixed;
                triangles[i].point2[1] = (y+1)*scaleYfixed;
                triangles[i].point2[2] = dataMuliplied[index][(y+1)*Nx[index]+(x-1)]/scaleMult;
                triangles[i].point1[0] = x*scaleXfixed;
                triangles[i].point1[1] = (y+1)*scaleYfixed;
                triangles[i].point1[2] = dataMuliplied[index][(y+1)*Nx[index]+x]/scaleMult;
                i++;
            }
            if(x!=sizeX-1)//here we writing flat side up triangle
            {
                triangles[i].point3[0] = x*scaleXfixed;
                triangles[i].point3[1] = y*scaleYfixed;
                triangles[i].point3[2] = dataMuliplied[index][y*Nx[index]+x]/scaleMult;
                triangles[i].point2[0] = x*scaleXfixed;
                triangles[i].point2[1] = (y+1)*scaleYfixed;
                triangles[i].point2[2] = dataMuliplied[index][(y+1)*Nx[index]+x]/scaleMult;
                triangles[i].point1[0] = (x+1)*scaleXfixed;
                triangles[i].point1[1] = y*scaleYfixed;
                triangles[i].point1[2] = dataMuliplied[index][y*Nx[index]+(x+1)]/scaleMult;
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
}

void converter::delZeros()
{
    int deleted = 0;
    QVector<QVector<bool> >isMT(dataMuliplied.size());
    for(int i=0;i<dataMuliplied.size();i++)
    {
        isMT[i].resize(Ny[i]);
        isMT[i].fill(true);
    }
    for(int i=0;i<isMT.length();i++)
    {
        for(int j=0;j<Ny[i];j++)
        {
            for(int k=0;k<Nx[i];k++)
            {
                if(dataMuliplied[i][(j*Nx[i])+k]!=0)
                {
                    isMT[i][j] = false;
                    break;
                }
            }
            if(isMT[i][j])
            {
                dataMuliplied[i].remove(j*Nx[i],Nx[i]);
                j--;
                Ny[i]--;
                deleted++;
            }
        }
    }
    if(deleted>0)
    {
        QMessageBox yep;
        yep.setText("Deleted " + QString::number(deleted) + " empty strings");
        yep.exec();
    }
}

void converter::on_fixSurfBut_clicked()
{
    quint8 ind = ui->fieldselect->currentIndex();
    quint16 X = Nx[ind], Y = Ny[ind];
    for(int i=0;i<X*Y;i+=X)//for each row
    {
        calcCurve(i);
        for(int j=0;j<X;j++)
        {
            dataMuliplied[ind][i+j]-=(a*j*j)+(b*j)+c;
        }
    }
    wasFixed[ind] = true;
    ui->fixSurfBut->setEnabled(false);
}

void converter::calcCurve(int start)
{
    quint8 ind = ui->fieldselect->currentIndex();
    quint16 X = Nx[ind];
    QVector<short> xArray(X);
    QVector<float> yArray(X);
    for(int i=0;i<X;i++)
    {
        xArray[i]=i;
        yArray[i]=dataMuliplied[ind][start+i];
    }
    double ATAbase[5], ATAmatrix[3][3], ATAinversed[3][3], ATYmatrix[3], CBAmatrix[3], det = 0;
    for(int i=0;i<5;i++)
    {
        ATAbase[i]=0;
        for(int j=0;j<X;j++)
        {
            ATAbase[i] += pow(xArray[j],i);
        }
    }
    for(int i=0;i<3;i++)
    {
        CBAmatrix[i]=0;
        for(int j=0;j<3;j++)
        {
            ATAmatrix[i][j] = ATAbase[i+j];
        }
    }
    //calculate determinant
    for(int i = 0; i < 3; i++)
        det = det + (ATAmatrix[0][i] * (ATAmatrix[1][(i+1)%3] * ATAmatrix[2][(i+2)%3] - ATAmatrix[1][(i+2)%3] * ATAmatrix[2][(i+1)%3]));
    //calculate inverse matrix
    for(int i = 0; i < 3; i++)
    {
        for(int j = 0; j < 3; j++)
            ATAinversed[i][j]=((ATAmatrix[(j+1)%3][(i+1)%3] * ATAmatrix[(j+2)%3][(i+2)%3]) - (ATAmatrix[(j+1)%3][(i+2)%3] * ATAmatrix[(j+2)%3][(i+1)%3]))/det;
    }
    for(int i=0;i<3;i++)
    {
        ATYmatrix[i]=0;
        for(int j=0;j<X;j++)
        {
            ATYmatrix[i]+=(pow(xArray[j],i)*yArray[j]);
        }
    }
    for(int j = 0; j < 3; ++j)
         for(int k = 0; k < 3; ++k)
         {
            CBAmatrix[j] += ATYmatrix[k] * ATAinversed[k][j];
         }
    c = CBAmatrix[0]; b = CBAmatrix[1]; a = CBAmatrix[2];
}

void converter::on_fieldselect_currentIndexChanged(int index)
{
    if(ui->fieldselect->count()>0)
        ui->fixSurfBut->setEnabled(!wasFixed[index]);
}

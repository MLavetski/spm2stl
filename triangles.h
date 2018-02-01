#ifndef TRIANGLES_H
#define TRIANGLES_H

class triangle
{
public:
        float point1[3], point2[3], point3[3], normal[3];//1st is x; 2nd is y; 3rd is z.
        quint16 attrByteCount = 0;
        void calcNorm()
        {
            normal[0] = (point1[1]*(point2[2]-point3[2])+point2[1]*(point3[2]-point1[2])+point3[1]*(point1[2]-point2[2]));
            normal[1] = (point1[2]*(point2[0]-point3[0])+point2[2]*(point3[0]-point1[0])+point3[2]*(point1[0]-point2[0]));
            normal[2] = (point1[0]*(point2[1]-point3[1])+point2[0]*(point3[1]-point1[1])+point3[0]*(point1[1]-point2[1]));
        }
};

#endif // TRIANGLES_H

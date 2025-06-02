#ifndef HANDMADE_MATH_H
#define HANDMADE_MATH_H

struct v2
{
    real32 X, Y;
    real32 &operator[](int Index) {return((&X)[Index]);}

    inline v2 &operator*=(real32 A);
    inline v2 &operator+=(v2 A);
};

inline v2
V2(real32 A, real32 B)
{
    v2 Result;

    Result.X = A;
    Result.Y = B;

    return Result;
}

inline v2
operator*(real32 A, v2 B)
{
    v2 Result;

    Result.X = A*B.X;
    Result.Y = A*B.Y;

    return Result;
}

inline v2
operator*(v2 A, real32 B)
{
    v2 Result;

    Result.X = B*A.X;
    Result.Y = B*A.Y;

    return Result;
}

inline v2
operator-(v2 A)
{
    v2 Result;

    Result.X = -A.X;

    return Result;
}

inline v2
operator+(v2 A, v2 B)
{
    v2 Result;

    Result.X = A.X + B.X;
    Result.Y = A.Y + B.Y;

    return Result;
}

inline v2 
operator-(v2 A, v2 B)
{
    v2 Result;

    Result.X = A.X - B.X;
    Result.Y = A.Y - B.Y;

    return Result;
}

inline v2 &v2::
operator*=(real32 A)
{
    *this = A * (*this);
    
    return (*this);
}

inline v2 &v2::
operator+=(v2 A)
{
    *this = A + (*this);

    return (*this);
}

inline real32
LengthSq(v2 A)
{
    real32 Result = A.X*A.X + A.Y*A.Y;

    return Result;
}

inline real32
Square(real32 A)
{
    real32 Result = A*A;

    return Result;
}

inline real32
Inner(v2 A, v2 B)
{
    real32 Result = A.X*B.X + A.Y*B.Y;

    return Result;
}

#endif

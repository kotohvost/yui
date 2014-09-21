/*
	$Id: point.h,v 3.2.2.1 2007/07/24 09:58:07 shelton Exp $
*/
#ifndef _POINT_RECT_
#define _POINT_RECT_

inline int min( int a, int b )
{
    return (a>b) ? b : a;
}

inline int max( int a, int b )
{
    return (a<b) ? b : a;
}

class Point
{
public:
    Point( int Y, int X ) { x=X; y=Y; }
    Point() { x=y=0; }
    Point& operator+=( const Point& adder );
    Point& operator-=( const Point& subber );
    Point& operator =( const Point& r);
    friend Point operator - ( const Point& one, const Point& two)
       { return Point( one.y - two.y, one.x - two.x ); }
    friend Point operator + ( const Point& one, const Point& two)
       { return Point( one.y + two.y, one.x + two.x ); }
    friend inline int operator == ( const Point& one, const Point& two)
       { return one.x == two.x && one.y == two.y; }
    friend inline int operator != ( const Point& one, const Point& two)
       { return one.x != two.x || one.y != two.y; }
    int x,y;
};

inline Point& Point::operator += ( const Point& adder )
{
    x += adder.x;
    y += adder.y;
    return *this;
}

inline Point& Point::operator -= ( const Point& subber )
{
    x -= subber.x;
    y -= subber.y;
    return *this;
}

inline Point& Point::operator = (const Point& r)
{
    x=r.x;
    y=r.y;
    return *this;
}

class PointL
{
public:
    PointL( long Y, long X ) { x=X; y=Y; }
    PointL() { x=y=0; }
    PointL& operator+=( const PointL& adder );
    PointL& operator-=( const PointL& subber );
    PointL& operator =( const PointL& r);
    friend PointL operator - ( const PointL& one, const PointL& two )
       { return PointL( one.y - two.y, one.x - two.x ); }
    friend PointL operator + ( const PointL& one, const PointL& two )
       { return PointL( one.y + two.y, one.x + two.x ); }
    friend inline int operator == ( const PointL& one, const PointL& two )
       { return one.x == two.x && one.y == two.y; }
    friend inline int operator != ( const PointL& one, const PointL& two )
       { return one.x != two.x || one.y != two.y; }
    long x, y;
};

inline PointL& PointL::operator += ( const PointL& adder )
{
    x += adder.x;
    y += adder.y;
    return *this;
}

inline PointL& PointL::operator -= ( const PointL& subber )
{
    x -= subber.x;
    y -= subber.y;
    return *this;
}

inline PointL& PointL::operator = (const PointL& r)
{
    x=r.x;
    y=r.y;
    return *this;
}


class Rect
{
public:
    Rect( int ay, int ax, int by, int bx );
    Rect( Point p1, Point p2 );
    Rect();
    void grow( int aDX, int aDY );
    void move( int aDX, int aDY );
    void intersect( const Rect& r );
    friend int operator == ( const Rect &r1, const Rect &r2 )
	{ return r1.a == r2.a && r1.b == r2.b; }
    int contain( const Point &p )
	{ return a.y<=p.y && b.y>=p.y && a.x<=p.x && b.x>=p.x; }
    Rect & operator = (const Rect& r);
    Point a, b;
};

inline Rect& Rect::operator=(const Rect&r)
{
   a.x=r.a.x; a.y=r.a.y; b.x=r.b.x; b.y=r.b.y;
   return *this;
}

inline Rect::Rect( int ay, int ax, int by, int bx )
{
   a.x = ax; a.y = ay; b.x = bx; b.y = by;
}

inline Rect::Rect( Point p1, Point p2 )
{
   a = p1; b = p2;
}

inline Rect::Rect()
{
   a=b=Point(0,0);
}

inline void Rect::grow( int aDX, int aDY )
{
   a.x -= aDX; a.y -= aDY; b.x += aDX; b.y += aDY;
}

inline void Rect::move( int aDX, int aDY )
{
   a.x += aDX; a.y += aDY; b.x += aDX; b.y += aDY;
}

inline void Rect::intersect( const Rect& r )
{
   a.x = max( a.x, r.a.x );
   a.y = max( a.y, r.a.y );
   b.x = min( b.x, r.b.x );
   b.y = min( b.y, r.b.y );
}

#endif

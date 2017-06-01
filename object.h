#include "primitives.h"
#include <iostream>

enum ObjectType {tri,quad,sphere};

class Primitive {
	public:
	// initialization
	Primitive(void);													// null type - specification must be added later - do NOT use this.
	Primitive(ObjectType t, vector p1, vector p2, vector p3, color c);			// tri type
	Primitive(ObjectType t, vector p1, vector p2, vector p3, vector p4, color c);	// quad type
	Primitive(ObjectType t, vector origin, double radius, color c);				// sphere type
	
	// class functions
	vector Intersect(ray r);
	vector Normal(vector intersection);
	vector Reflect(ray r);
	color  ColorAt(vector pos);
	void Print();
	
	private:
	// primitive properties
	ObjectType type;
	color Color;
	
	vector Point1;		// for tri/quad
	vector Point2;
	vector Point3;
	vector Point4;
	
	vector Origin;		// sphere
	double Radius;		
};

Primitive::Primitive(void) {
	;
}

Primitive::Primitive(ObjectType t, vector p1, vector p2, vector p3, color c) {
	if (t == tri) {
		type = t;
		Point1 = p1;
		Point2 = p2;
		Point3 = p3;
		Color = c;
	}
	else 
		std::cout << "Invalid Primitive declaration.";
}
	
Primitive::Primitive(ObjectType t, vector p1, vector p2, vector p3, vector p4, color c) {
	if (t == quad) {
		type = t;
		Point1 = p1;
		Point2 = p2;
		Point3 = p3;
		Point4 = p4;
		Color = c;
	}
	else 
		std::cout << "Invalid Primitive declaration.";
}

Primitive::Primitive(ObjectType t, vector origin, double radius, color c) {
	if (t == sphere) {
		type = t;
		Origin = origin;
		Radius = radius;
		Color = c;
	}
	else 
		std::cout << "Invalid Primitive declaration.";
}

vector Primitive::Intersect(ray r) {
	switch (type) {
		case tri: {
			return intersect(r,polygon(triangle(Point1,Point2,Point3),color(0,0,0)));
		}
		case quad: {
			vector result = intersect(r,polygon(triangle(Point1,Point2,Point3),color(0,0,0)));
			if (magnitude(result) == 0) result = intersect(r,polygon(triangle(Point4,Point2,Point3),color(0,0,0)));
			
			return result;
		}
		case sphere: {
			r.origin.x -= Origin.x;
			r.origin.y -= Origin.y;
			r.origin.z -= Origin.z;
			
			double a,b,c,determinant,m;
			
			a = (r.trace.x)*(r.trace.x) + (r.trace.y)*(r.trace.y) + (r.trace.z)*(r.trace.z);
			b = 2*((r.trace.x)*(r.origin.x) + (r.trace.y)*(r.origin.y) + (r.trace.z)*(r.origin.z));
			c = (r.origin.x)*(r.origin.x) + (r.origin.y)*(r.origin.y) + (r.origin.z)*(r.origin.z) - Radius*Radius;
			
			determinant = b*b - 4*a*c;
			
			if (determinant > 0) {
				m = (-b - sqrt(determinant))/(2*a);
				if (m < 0) m += 2*sqrt(determinant)/(2*a);
			}
			else if (determinant == 0 && b < 0) m = 0.5*(-b/a);
			else m = -1;
			
			if (m > 0) return add_vector(r.origin,scalar_multiply(r.trace,m));
			else return vector(0,0,0);
		}
		default: std::cout << "Undefined type in Primitive::Intersect\n";
	}
	
	return vector();
}

vector Primitive::Normal(vector intersection) {
	switch (type) {
		case tri: return cross_product(subtract_vector(Point2,Point1),subtract_vector(Point3,Point1));
		case quad: return cross_product(subtract_vector(Point2,Point1),subtract_vector(Point3,Point1));
		case sphere: return normalize(subtract_vector(intersection,Origin));
		default: std::cout << "Undefined type in Primitive::Normal\n";
	}
}
		
vector Primitive::Reflect(ray r) {
	vector normal = Normal(Intersect(r)),
		  reflection = add_vector(r.trace,scalar_multiply(normal,-2*(dot_product(r.trace,normal))/(magnitude(r.trace)*magnitude(normal))));
	return reflection;
}

color Primitive::ColorAt(vector v) {	// FIXME: this will have to be changed when using tex maps
	return Color; 
}

void Primitive::Print() {
	switch (type) {
		case tri: {
			std::cout << "Triangle" << std::endl
				<< "  Point1: (" << Point1.x << "," << Point1.y << "," << Point1.z << ")" << std::endl
				<< "  Point2: (" << Point2.x << "," << Point2.y << "," << Point2.z << ")" << std::endl
				<< "  Point3: (" << Point3.x << "," << Point3.y << "," << Point3.z << ")" << std::endl
				<< "  Color:  " << (int)Color.r << " " << (int)Color.g << " " << (int)Color.b << " " << (int)Color.a << " " << (int)Color.ref << std::endl;
			break;
		}
		case quad: {
			std::cout << "Quadrangle" << std::endl
				<< "  Point1: (" << Point1.x << "," << Point1.y << "," << Point1.z << ")" << std::endl
				<< "  Point2: (" << Point2.x << "," << Point2.y << "," << Point2.z << ")" << std::endl
				<< "  Point3: (" << Point3.x << "," << Point3.y << "," << Point3.z << ")" << std::endl
				<< "  Point4: (" << Point4.x << "," << Point4.y << "," << Point4.z << ")" << std::endl
				<< "  Color:  " << (int)Color.r << " " << (int)Color.g << " " << (int)Color.b << " " << (int)Color.a << " " << (int)Color.ref << std::endl;
			break;
		}
		case sphere: {
			std::cout << "Sphere" << std::endl
				<< "  Origin: (" << Origin.x << "," << Origin.y << "," << Origin.z << ")" << std::endl
				<< "  Radius: " << Radius << std::endl
				<< "  Color:  " << (int)Color.r << " " << (int)Color.g << " " << (int)Color.b << " " << (int)Color.a << " " << (int)Color.ref << std::endl;
			break;
		}
		default: {
			std::cout << "Type Unknown (FIX!)" << std::endl;
		}
	}
}
			
struct object_node {
	Primitive object;
	object_node * next;
	object_node(void) : object(),next(NULL) {}
	object_node(Primitive o,object_node *n) : object(o),next(n) { }
};

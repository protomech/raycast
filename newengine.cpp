#include "object.h"
#include <iostream.h>

int main() {
	ray camera(vector(-10,-10,-10),vector(1,1,1));
	ObjectType s = sphere;
	Primitive sphere1(s,vector(0,0,0),3);
	vector temp_intersect = sphere1.Intersect(camera);
	
	cout << temp_intersect.x << " " 
	     << temp_intersect.y << " " 
	     << temp_intersect.z << endl; 
	return 0;
}
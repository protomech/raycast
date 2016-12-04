#include <math.h>
#include "targa.h"

// geometric primitives

struct vector {
	float x, y, z;
     vector (void) : x(0),y(0),z(0) { }
	vector (float x_, float y_, float z_) :
     	x(x_),y(y_),z(z_) { }
};

struct ray {
	vector origin, trace;
	ray (void) : origin(),trace() { }
     ray (vector o, vector t) :
     	origin(o),trace(t) { }
};

struct triangle {
	vector p1, p2, p3;
	triangle(void) : p1(), p2(), p3() { }
     triangle(vector a, vector b, vector c) :
     	p1(a),p2(b),p3(c) { }
};

struct plane {
	vector origin, trace1, trace2;
	plane(void) : origin(), trace1(), trace2() { }
     plane(vector o, vector t1, vector t2) :
     	origin(o), trace1(t1), trace2(t2) { }
};


// basic vector/geometry operations

vector scalar_multiply(vector v, float i) {
	return vector(i * v.x, i * v.y, i * v.z);
}

float dot_product(vector a, vector b) {
	return ((a.x * b.x) + (a.y * b.y) + (a.z * b.z));
}

vector cross_product(vector a, vector b) {
	return vector ((a.y * b.z - a.z * b.y),
    			   	(a.z * b.x - a.x * b.z),
                 	(a.x * b.y - a.y * b.x));
}

vector add_vector(vector a, vector b) {
	return vector(a.x + b.x, a.y + b.y, a.z + b.z);
}

vector subtract_vector(vector a, vector b) {
	return vector(a.x - b.x, a.y - b.y, a.z - b.z);
}

plane tri_to_plane(triangle t) {
     return plane(t.p1, subtract_vector(t.p2,t.p1), subtract_vector(t.p3,t.p1));
}

float magnitude(vector v) {
	return sqrt((v.x * v.x) + (v.y * v.y) + (v.z * v.z));
}

vector normalize(vector v) {
	return scalar_multiply(v,1.0/magnitude(v));
}

vector reflect_vector(vector normal, vector original) {
		vector reflection = add_vector(original,scalar_multiply(normal,-2*(dot_product(original,normal))/(magnitude(original)*magnitude(normal))));
		return reflection;
}


// engine primitives

struct color {
	unsigned char r,g,b,a,ref;  // red, green, blue, alpha, reflectivity channels
     color(void) : r(0),g(0),b(0),a(0),ref(0) { }
     color(unsigned char red, unsigned char green, unsigned char blue) : // rgb triple
     	r(red),g(green),b(blue),a(0),ref(0) { }
     color(unsigned char red, unsigned char green, unsigned char blue, unsigned char alpha, unsigned char reflectivity) : //rgbar pentuple
     	r(red),g(green),b(blue),a(alpha),ref(reflectivity) { }
};

color add_color(color a, color b) {
	int 	sum_r = a.r + b.r,
		sum_g = a.g + b.g,
		sum_b = a.b + b.b;
		
	return color((sum_r > 255 ? 255 : sum_r),(sum_g > 255 ? 255 : sum_g),(sum_b > 255 ? 255 : sum_b));
}

struct polygon {
     triangle t;
	color col;
	vector origin, leg1, leg2;
	vector normal;
	texture tex;
	polygon(void) : 
		t(),col(),origin(),leg1(),leg2(),normal(),tex() { }
     polygon(triangle tri, color c) :
     	t(tri),col(c),origin(tri.p1),
     	leg1(subtract_vector(tri.p2,tri.p1)),leg2(subtract_vector(tri.p3,tri.p1)),
     	normal(cross_product(leg1,leg2)),tex() { }
};

struct node {
	polygon p;
     node * next;
     node(void) : p(),next(NULL) { }
     node(polygon pol, node *nxt) :
     	p(pol),next(nxt) { }
};

struct light {
	vector pos;
	vector inclination;
	color hue;
	float fov;
	light (void) : 								// non-initialized light
		pos(),inclination(),hue(),fov(0) { }
	light (vector p, color h) :						// non-directional light
		pos(p),inclination(),hue(h),fov(M_PI/2) { }
	light (vector p, vector i, color h, float f) :		// directional "spot"light
		pos(p),inclination(i),hue(h),fov(f) { }
};

struct light_node {
	light l;
	light_node * next;
	light_node(void) : l(),next(NULL) { }
	light_node(light lgt, light_node * nxt) :
		l(lgt),next(nxt) { }
};

int remove_node(node *source, node *target) {
//	cout << "delete_node..\n";
	if (source == NULL) {
		cout << "Ooops..." << endl;
		return 0; // shouldn't happen
	}

	node *temp, *counter = source;
	while (counter->next != NULL) {
	
		if (counter->next == target) {
			temp = counter->next;
			delete counter->next;
			counter->next = temp->next;
			delete temp;
			return 0;
		}
		counter = counter->next;
	}
}

int add_node(node *source, node *target) {
	node *temp;
//	cout << "add_node..\n";
	if (source == NULL) {
		cout << "Ooops..." << endl;
		return 0;  // shouldn't happen
	}
	
	if (source->next == NULL) {
		source->next = target;
		return 0;
	}
	else {
		temp = source->next->next;
		source->next = target;
		source->next->next = temp;
	}
}

// spherical form values/functions

struct spherical_vector {
	float theta, phi;
	float radius;
	spherical_vector(void) : theta(0),phi(0),radius(0) { }
	spherical_vector(float th, float ph, float ra) : theta(th),phi(ph),radius(ra) { }
};

vector spherical_to_cartesian(spherical_vector s) {
	vector c;
	
	c.x = s.radius*cos(s.phi)*cos(s.theta);
	c.y = s.radius*sin(s.phi);
	c.z = s.radius*sin(s.theta)*cos(s.phi);
	
	return c;
}

spherical_vector cartesian_to_spherical(vector c) {
	spherical_vector s;
	
	if (c.x == 0)
		if (c.z > 0)	s.theta = M_PI_2;
		else			s.theta = -M_PI_2;
	else if (c.z > 0)	s.theta = acos(c.x/magnitude(vector(c.x,0,c.z)));
	else 			s.theta = -acos(c.x/magnitude(vector(c.x,0,c.z)));
	
	if (c.y == 0)		s.phi = 0;
	else if (c.y > 0)	s.phi = acos(magnitude(vector(c.x,0,c.z))/magnitude(c));
	else				s.phi = -acos(magnitude(vector(c.x,0,c.z))/magnitude(c));
	
	s.radius = magnitude(c);
	
	return s;
}
	


// engine functions

vector intersect(ray r, polygon pol) {
	plane p(pol.origin,pol.leg1,pol.leg2);

     vector v = subtract_vector(r.origin,p.origin);
     

/*   this section uses Cramer's Rule to locate a point that represents
	the intersection of a line and a plane.  Cramer's Rule states that
                  	a1(x) + b1(y) + c1(z) = d1
     the system : 	a2(x) + b2(y) + c2(z) = d2
                    a3(x) + b3(y) + c3(z) = d3

     x = D1/D, y = D2/D, z = D3/D

               |a1 b1 c1|       |d1 b1 c1|       |a1 d1 c1|       |a1 b1 d1|
     where D = |a2 b2 c2|, D1 = |d2 b2 c2|, D2 = |a2 d2 c2|, D3 = |a2 b2 d2|
               |a3 b3 c3|       |d3 b3 c3|       |a3 d3 c3|       |a3 b3 d3|
*/

	float D =    -(p.trace1.x * p.trace2.y * r.trace.z  +
     			p.trace2.x * r.trace.y  * p.trace1.z +
                    r.trace.x  * p.trace1.y * p.trace2.z -
                    r.trace.x  * p.trace2.y * p.trace1.z -
                    p.trace2.x * p.trace1.y * r.trace.z  -
                    p.trace1.x * r.trace.y  * p.trace2.z );

	if (D == 0) return vector(0,0,0); // no intersection

	float D1 =   -(v.x		 * p.trace2.y * r.trace.z  +
     			p.trace2.x * r.trace.y  * v.z        +
                    r.trace.x  * v.y        * p.trace2.z -
                    r.trace.x  * p.trace2.y * v.z        -
                    p.trace2.x * v.y        * r.trace.z  -
                    v.x  	 * r.trace.y  * p.trace2.z );

	float D2 =   -(p.trace1.x * v.y        * r.trace.z  +
     			v.x        * r.trace.y  * p.trace1.z +
                    r.trace.x  * p.trace1.y * v.z        -
                    r.trace.x  * v.y        * p.trace1.z -
                    v.x        * p.trace1.y * r.trace.z  -
                    p.trace1.x * r.trace.y  * v.z        );

	if ((D2/D + D1/D > 1.0) || (D2/D < 0) || (D1/D < 0))
		return vector(0,0,0);		// intersection is outside triangle


     float D3 =   -(p.trace1.x * p.trace2.y * v.z        +
     			p.trace2.x * v.y        * p.trace1.z +
                    v.x        * p.trace1.y * p.trace2.z -
                    v.x        * p.trace2.y * p.trace1.z -
                    p.trace2.x * p.trace1.y * v.z        -
                    p.trace1.x * v.y        * p.trace2.z );
                    
     vector return_vector = add_vector(r.origin,scalar_multiply(r.trace,-D3/D));
     vector connecting_vector = subtract_vector(return_vector,r.origin);
     
     if (connecting_vector.x/r.trace.x < 0 || connecting_vector.y/r.trace.y < 0 || connecting_vector.z/r.trace.z < 0)
     	return vector(0,0,0);
                    
 	return return_vector;
}

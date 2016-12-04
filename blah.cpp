#include <iostream.h>
#include <stdio.h>
#include "primitives.h"

int main() {
	
	node *temp = head, *storage;                                                            
	node *poly_temp = head;                                                                 
	light_node *light_temp = light_head;                                                    
	tex_list *tex_temp = tex_head;                                                          
	                                                                                        
	float theta, phi;                                                                       
	char line[MAXLINE] = { };                                                               
	triangle tri;                                                                           
	light lgt;                                                                              
	vector p1, p2, p3;                                                                      
	color col;                                                                              
	bool light_called = false, poly_called = false;                                         
	int input1,input2,input3;                                                               
	int line_counter = 0, num_lights = 0, num_obj = 0, num_poly = 0, num_tex = 0;           

	
	FILE *input = fopen("temp.ren","r");
	bool pos = false, color = false, angle = false, fov = false;
	do {
		if (!feof(input) && fgets(line,MAXLINE,input)) {
			line_counter++;
			if ((ptr = strstr(line,"Pos")) != NULL) {
				cout << "Pos located." << endl;
				float x, y, z;
				ptr += strlen("Pos");
				if (sscanf(ptr,"%f %f %f",&x,&y,&z) == 3) {
					light_temp->l.pos = vector(x,y,z);
					pos = true;
				}
				else {
					printf("Syntax error on line %d: %s",line_counter,line);
					return 1;
				}
				cout << "pos: " << pos << endl;
			}
			if ((ptr = strstr(line,"Hue")) != NULL) {
				cout << "Hue located." << endl;
				unsigned char r, g, b;
				ptr += strlen("Hue");
				cout << pos << endl;
				if (sscanf(ptr,"%d %d %d",&r,&g,&b) == 3) {
				cout << pos << endl;
					light_temp->l.hue.r = r;
					light_temp->l.hue.g = g;
					light_temp->l.hue.b = b;
					color = true;
				}
				else {
					printf("Syntax error on line %d: %s",line_counter,line);
					return 1;
				}
				cout << "hue: " << color << " Pos: " << pos << endl;
			}
		}
		else {
			printf("EOF on line %d.\n",line_counter);
			return 1;
		}
	} while (!(ptr = strstr(line,"}")));
	
	return 0;
}
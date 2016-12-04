#include "object.h"
#include <iostream.h>
#include <stdio.h>

#define MAXLINE 1000

color ambient_light;

int parse_input(FILE *input, ray *cam, object_node * object_list, light_node * light_list);

int main() {
	ray cam;
	object_node * object_list = NULL, object_temp;
	light_node * light_list = NULL, light_temp;
	
	
	
	FILE *input = fopen("new.ren","r");
	if (parse_input(input,&cam,object_list,light_list)) {
		cout << "Error parsing input file.  Exiting..." << endl;
		return 1;
	}
	
	cout << cam.origin.x << " " << cam.trace.x << endl;
}

int parse_input(FILE *input, ray *cam, object_node * object_list, light_node * light_list) {
	char line[MAXLINE] = {};
	char *ptr = NULL;
	vector point[4];
		
	object_node * object_temp = object_list;
	light_node  * light_temp  = light_list;
	
	int num_lights = 0, num_objects = 0, num_points = 0, line_num = 0;
	
	while (!feof(input)) {
		fgets(line,MAXLINE,input); line_num++;
		if ((ptr = strstr(line,"//")) != NULL) *ptr = 0;
			
		if ((ptr = strstr(line,"Cam")) != NULL) {
			bool cam_pos = false, cam_view = false;
			float theta, phi;
			while (!feof(input)) {
				fgets(line,MAXLINE,input); line_num++;
				if ((ptr = strstr(line,"//")) != NULL) *ptr = 0;
		
				if ((ptr = strstr(line,"CameraPos")) != NULL) {
					ptr += strlen("CameraPos");
					if (sscanf(ptr,"%f %f %f",&(cam->origin.x),&(cam->origin.y),&(cam->origin.z)) != 3) {
						cout << "Error in CameraPos declaration.\n";
						return 1;
					}
					cam_pos = true;
				}
				else if ((ptr = strstr(line,"CameraView")) != NULL) {
					ptr += strlen("CameraView");
					if (sscanf(ptr,"%f %f",&theta,&phi) != 2) {
						cout << "Error in CameraView declaration.\n";
						return 1;
					}
					cam->trace = spherical_to_cartesian(spherical_vector(theta,phi,1));
					cam_view = true;
				}
				if (cam_view && cam_pos) break;
			}
		}
		if ((ptr = strstr(line,"Light")) != NULL) {
			fgets(line,MAXLINE,input); line_num++;
			if ((ptr = strstr(line,"Ambient")) != NULL) {
				ptr += strlen("Ambient");
				if (sscanf(ptr,"%d %d %d",&(ambient_light.r),&(ambient_light.g),&(ambient_light.b)) != 3) {
					cout << "Error in Light::Ambient declaration.\n";
					return 1;
				}
			}
			else {
				cout << "error in line " << line_num << endl;
				return 1;
			}
			while(!feof(input)) {
				fgets(line,MAXLINE,input); line_num++;
				if ((ptr = strstr(line,"EndLight")) != NULL)
					break;
				if ((ptr = strstr(line,"Point")) != NULL) {
					fgets(line,MAXLINE,input); line_num++;
					if ((ptr = strstr(line,"Pos")) != NULL) {
						ptr += strlen("Pos");
						if (sscanf(ptr,"%f %f %f",&(light_temp->l.pos.x),&(light_temp->l.pos.y),&(light_temp->l.pos.z)) != 3) {
							cout << "Error in Light::Point::Pos declaration, line " << line_num << endl;
							return 1;
						}
					}
					else {
						cout << "error in line " << line_num << endl;
						return 1;
					}
					fgets(line,MAXLINE,input); line_num++;
					if ((ptr = strstr(line,"Hue")) != NULL) {
						ptr += strlen("Hue");
						if (sscanf(ptr,"%d %d %d",&(light_temp->l.hue.r),&(light_temp->l.hue.g),&(light_temp->l.hue.b)) != 3) {
							cout << "Error in Light::Point::Hue declaration.\n";
							return 1;
						}
					}
					else {
						cout << "error in line " << line_num << endl;
						return 1;
					}
					light_temp->next = new light_node;
					light_temp = light_temp->next;
				}
				else {
					cout << "error in line " << line_num << endl;
					return 1;
				}
			}
		}
		if ((ptr = strstr(line,"ObjectList")) != NULL) {
			color obj_color;
			while(!feof(input)) {
				fgets(line,MAXLINE,input); line_num++;
				if ((ptr = strstr(line,"EndObjectList")) != NULL)
					break;
				if ((ptr = strstr(line,"Tri")) != NULL) {
					for (num_points = 0; num_points < 3; num_points++) {
						fgets(line,MAXLINE,input); line_num++;
						if ((ptr = strstr(line,"Vertex")) != NULL) {
							ptr += strlen("Vertex");
							if (sscanf(ptr,"%f %f %f",&(point[num_points].x),&(point[num_points].y),&(point[num_points].z)) != 3) {
								cout << "Error in Tri::Vertex declaration, line " << line_num << endl;
								return 1;
							}
						}
						else {
							cout << "error in line " << line_num << endl;
							return 1;
						}
					}
					fgets(line,MAXLINE,input); line_num++;
					if ((ptr = strstr(line,"Color")) != NULL) {
						ptr += strlen("Color");
						if (sscanf(ptr,"%d %d %d %d %d",&(obj_color.r),&(obj_color.g),&(obj_color.b),&(obj_color.a),&(obj_color.ref)) != 5) {
							cout << "Error in Tri::Color declaration, line " << line_num << endl;
							return 1;
						}
					}
					else {
						cout << "error in line " << line_num << endl;
						return 1;
					}
					ObjectType t = tri;
					Primitive bob(t,point[0],point[1],point[2],obj_color);
					object_temp->object = bob;
					object_temp->next = new object_node;
					object_temp = object_temp->next;
				}
				else if ((ptr = strstr(line,"Quad")) != NULL) {
					for (num_points = 0; num_points < 4; num_points++) {
						fgets(line,MAXLINE,input); line_num++;
						if ((ptr = strstr(line,"Vertex")) != NULL) {
							ptr += strlen("Vertex");
							if (sscanf(ptr,"%f %f %f",&(point[num_points].x),&(point[num_points].y),&(point[num_points].z)) != 3) {
								cout << "Error in Tri::Vertex declaration, line " << line_num << endl;
								return 1;
							}
						}
						else {
							cout << "error in line " << line_num << endl;
							return 1;
						}
					}
					fgets(line,MAXLINE,input); line_num++;
					if ((ptr = strstr(line,"Color")) != NULL) {
						ptr += strlen("Color");
						if (sscanf(ptr,"%d %d %d %d %d",&(obj_color.r),&(obj_color.g),&(obj_color.b),&(obj_color.a),&(obj_color.ref)) != 5) {
							cout << "Error in Tri::Color declaration, line " << line_num << endl;
							return 1;
						}
					}
					else {
						cout << "error in line " << line_num << endl;
						return 1;
					}
					ObjectType q = quad;
					object_temp->object = Primitive(q,point[0],point[1],point[2],point[4],obj_color);
					object_temp->next = new object_node;
					object_temp = object_temp->next;
				}
				else if ((ptr = strstr(line,"Sphere")) != NULL) {
					vector sphere_origin;
					float sphere_radius;
					fgets(line,MAXLINE,input); line_num++;
					if ((ptr = strstr(line,"Origin")) != NULL) {
						ptr += strlen("Origin");
						if (sscanf(ptr,"%f %f %f",&(sphere_origin.x),&(sphere_origin.y),&(sphere_origin.z)) != 3) {
							cout << "Error in Sphere::Origin declaration, line " << line_num << endl;
							return 1;
						}
					}
					else {
						cout << "error in line " << line_num << endl;
						return 1;
					}
					fgets(line,MAXLINE,input); line_num++;
					if ((ptr = strstr(line,"Radius")) != NULL) {
						ptr += strlen("Radius");
						if (sscanf(ptr,"%f",&sphere_radius) != 1) {
							cout << "Error in Sphere::Radius declaration, line " << line_num << endl;
							return 1;
						}
					}
					else {
						cout << "error in line " << line_num << endl;
						return 1;
					}
					fgets(line,MAXLINE,input); line_num++;
					if ((ptr = strstr(line,"Color")) != NULL) {
						ptr += strlen("Color");
						if (sscanf(ptr,"%d %d %d %d %d",&(obj_color.r),&(obj_color.g),&(obj_color.b),&(obj_color.a),&(obj_color.ref)) != 5) {
							cout << "Error in Sphere::Color declaration, line " << line_num << endl;
							return 1;
						}
					}
					else {
						cout << "error in line " << line_num << endl;
						return 1;
					}
					ObjectType s = sphere;
					object_temp->object = Primitive(s,sphere_origin,sphere_radius,obj_color);
					object_temp->next = new object_node;
					object_temp = object_temp->next;
				}
				else {
					cout << "error in line " << line_num << endl;
					return 1;
				}
			}
		}
	}
	cout << "Parsed " << line_num << " line(s) successfully." << endl;
	
	return 0;
}

		
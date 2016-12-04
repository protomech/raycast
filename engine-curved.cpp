// Primitive Rendering Engine
// v.10 alpha
//
// Environment settings: set tab to 5 spaces
//
// todo:
// true reflective lights
// textures
// texture mipmapping? (necessary?)
// convert to .tga format (?)
// fix precision bugs
// fix skewed perspective
// 
// optimization:
// "smarter" polygon sorting
// faster inner loop
//
// What it does: parses an input file for a list of polygons/environment settings, then writes out a resulting bitmap.
// Features: Allows six-degrees-of-freedom (6dof), alpha and reflectivity channels; accurately renders a scene.

#include <time.h>		// time
#include <iostream.h>	// various ... needs to be removed.
#include <math.h>		// math operations
#include <string.h>		// strcat, strcmp
#include <stdio.h>		// file i/o
#include <stdlib.h>		// atoi
#include "object.h"		// object and engine primitives


#define H_RES 640
#define V_RES 480

#define H_FOV 1.5708
#define V_FOV 1.1000

#define MAXLINE 1000
#define OBJECT_CLIP 0.001		// clip distance for objects (reflection & translucency)
#define NEAR_CLIP   0.001		// clip distance for the viewpoint

int h_res = H_RES;
int v_res = V_RES;
int anti_aliasing_samples = 1;
int flags_log = 0;
int flags_time = 0;
int pad = (4-(3*h_res % 4)) % 4;
float near_clip = 0.001;
int fog_type = 0;
float fog_clip = 10;
color fog(255,255,255,0,0);
color ambient_light(255,255,255,0,0);

char *output_filename = "temp.bmp";
char *input_filename = NULL;

double min(double a, double b) { if (a<b) return a; return b; }
double dabs(double a) { if (a < 0) return -a; return a; }
double dmod(double a, double b) {
	b = dabs(b);
	if (a > 0)
		while (a > dabs(b))
			a -= b;
	else {
		while (a < 0) 
			a += b;
		while (a > dabs(b))
			a -= b;
	}
	
	return a;
}

void print_vector(vector v);
void print_objectlist(object_node *n);
void print_lightlist(light_node *l);
int parse_args(int argc, char *argv[]);
int write_bitmap_header(FILE *outfile);
int parse_input(FILE *input, ray *r, object_node * object_list, light_node * light_list);

color trace(ray init_trace, object_node *object_list, light_node *light_list, color bgcolor, vector mask);

void main(int argc, char *argv[]) {
	
	if (parse_args(argc,argv) == 1) return 0;
	
	cout << "h_res: " << h_res << " v_res: " << v_res << endl;
	
	time_t begin = time(0), end;
	
	FILE *outfile = fopen(output_filename,"wb");
	write_bitmap_header(outfile);

	float i = 0, j = 0, k = 0;
	int real_i, real_j;
     float x, y, z;
     float temp_x_scale, temp_y_scale;
     int aa_i, aa_j;
     float aa_shift = 1.0/(anti_aliasing_samples+1);
     float i_partial, j_partial;
     int accumulate_red, accumulate_green, accumulate_blue;
     color bgcolor(0x10,0x10,0x10,0x00,0x00), return_color;
     
	ray cam_pos(vector(3,1,0), spherical_to_cartesian(spherical_vector(1.9,-.2,1)));
	float epsilon;
	spherical_vector temp_sv;
	vector ul, ur, ll, lr; 			// these are the corners of the viewing "frustrum"
	
	int result;
	
	object_node * object_list = new object_node;
	light_node * light_list = new light_node;
	
	if (input_filename == NULL) {
		cout << "Please enter an input filename" << endl;
		return 0;
//		head = new node(polygon(triangle(vector(0,1,4), vector(-1,-1,4), vector(1,-1,4)),color(255,255,255,0,0)),
//			  new node(polygon(triangle(vector(0,.5,8), vector(-.5,-.5,3.5), vector(.5,-.5,2)),color(0,255,255,0,0)),
//			  NULL));
	}
	else {
		FILE *input = fopen(input_filename,"r");
		if (parse_input(input,&cam_pos,object_list,light_list) != 0) {
			cout << "error in input file " << input_filename << ". Aborting..\n";
			return 0;
		}
	}
	
	temp_sv = cartesian_to_spherical(cam_pos.trace);
	
	ul = spherical_to_cartesian(spherical_vector(temp_sv.theta+H_FOV/2.0,temp_sv.phi+V_FOV/2.0,temp_sv.radius));
	ur = spherical_to_cartesian(spherical_vector(temp_sv.theta-H_FOV/2.0,temp_sv.phi+V_FOV/2.0,temp_sv.radius));
	ll = spherical_to_cartesian(spherical_vector(temp_sv.theta+H_FOV/2.0,temp_sv.phi-V_FOV/2.0,temp_sv.radius));
	lr = spherical_to_cartesian(spherical_vector(temp_sv.theta-H_FOV/2.0,temp_sv.phi-V_FOV/2.0,temp_sv.radius));

	ray ray_trace = cam_pos; // initial value
	
    	vector intersection_point, upper_vector, lower_vector;

	float min_magnitude, temp_magnitude;
     node *temp, *min_poly;

     temp_x_scale = sin(H_FOV)/(h_res-1);
	temp_y_scale = sin(V_FOV)/(v_res-1);
	
	for (j = 0; j < v_res; j++) {
     	for (i = h_res-1; i >= 0; i--) {
     	
//     		real_i = int(h_res - i);
//     		real_j = int(v_res - j);
  	
     		accumulate_red = accumulate_green = accumulate_blue = 0;
     		for (aa_i = 0; aa_i < anti_aliasing_samples; aa_i++) {
				i_partial = (i - 0.5 + aa_shift*(aa_i+1))/(h_res-1);

     			upper_vector = add_vector(scalar_multiply(ul,i_partial),scalar_multiply(ur,1-i_partial));
     			lower_vector = add_vector(scalar_multiply(ll,i_partial),scalar_multiply(lr,1-i_partial));
     			
     			for (aa_j = 0; aa_j < anti_aliasing_samples; aa_j++) {
					j_partial = (j - 0.5 + aa_shift*(aa_j+1))/(v_res-1);
					
					ray_trace.trace = add_vector(scalar_multiply(upper_vector,j_partial),scalar_multiply(lower_vector,1-j_partial));
					
					return_color = trace(ray_trace,object_list,light_list,bgcolor,ray_trace.origin);

     				accumulate_red   += return_color.r;
     				accumulate_green += return_color.g;
     				accumulate_blue  += return_color.b;
			    	}
		    	}
		    	accumulate_red   /= (anti_aliasing_samples * anti_aliasing_samples);
		    	accumulate_green /= (anti_aliasing_samples * anti_aliasing_samples);
		    	accumulate_blue  /= (anti_aliasing_samples * anti_aliasing_samples);
		    	
		    	fprintf(outfile,"%c%c%c",(unsigned char) accumulate_blue, (unsigned char) accumulate_green, (unsigned char) accumulate_red);
        	}
          for (k = 0; k < pad; k++)
			fprintf(outfile,"%c",(char) 0x00);
   	}
     end = time(0);
     fclose(outfile);

     if (flags_time) 
     	cout << "Render time: " << int(end-begin)/3600 << "h " << int((end-begin)/60)%60 << "m " << (end-begin)%60 << "s" << endl;
}


void print_vector(vector v) {
	cout << "(" << v.x << "," << v.y << "," << v.z << ")" << endl;
}

void print_objectlist(object_node *n) {
	int counter = 1;
	while (n != NULL) {
		n->object.Print();
		n = n->next;
	}
}

void print_lightlist(light_node *l) {
	int counter = 1;
	while (l != NULL) {
		cout << "Light " << counter << ": " << endl;
		cout << "  pos   : "; print_vector(l->l.pos);
		cout << "  color : " << (int)l->l.hue.r << " " << (int)l->l.hue.g << " " << (int)l->l.hue.b << endl;
		
		l = l->next;
		counter++;
	}
}
		
int parse_args(int argc, char *argv[]) {
	if (argc == 1) {
		cout << "Usage : render.exe [options]\n"
			<< "Options: \n"
			<< "  -aa [aa_samples]        Sets the number of anti-aliasing samples\n\n"
			<< "                          Note: fog currently broken.\n"
			<< "  -f [type] [max_dist]    Enables fog of a certain type and max distance.\n"
			<< "                          type 1: linear   type 2: squared\n"
			<< "  -fc [red] [grn] [blu]   Input the fog color.\n\n"
			<< "  -help                   Displays this message\n"
			<< "  -i [inputfile]          Set the input file (.ren)\n"
			<< "  -log                    Logs raw image data to a file\n"
			<< "  -o [outputfile]         Set the output bitmap file\n"
			<< "  -r [h_res] [v_res]      Change the default resolution\n"
			<< "  -t                      Displays time spent rendering\n\n"
			<< "For further information or support, email <protomech@flashmail.com>\n";
			
		return 1;
	}
	
	while (--argc > 0) {
		if (strcmp((++argv)[0],"-aa") == 0 && argc > 1) {
			if ((anti_aliasing_samples = atoi(argv[1])) < 1)
				anti_aliasing_samples = 1;
			argc--;
			argv++;
		}
		else if (strcmp(argv[0],"-f") == 0 && argc > 2) {
			fog_type = atoi(argv[1]);
			fog_clip = atof(argv[2]);
			if (fog_type != 1 && fog_type != 2) {
				cout << "Invalid fog type.\n";
				return 1;
			}
			argc -= 2;
			argv += 2;
		}
		else if (strcmp(argv[0],"-fc") == 0 && argc > 3) {
			fog.r = atoi(argv[1]);
			fog.g = atoi(argv[2]);
			fog.b = atoi(argv[3]);
			argc -= 3;
			argv += 3;
		}
		else if (strcmp(argv[0],"-i") == 0 && argc > 1) {
			input_filename = argv[1];
			argc--; 
			argv++;
		}
		else if (strcmp(argv[0],"-log") == 0) {
			flags_log = 1;
		}
		else if (strcmp(argv[0],"-o") == 0 && argc > 1) {
			output_filename = argv[1];
			argc--; 
			argv++;
		}
		else if (strcmp(argv[0],"-r") == 0 && argc > 2) {
			h_res = atoi(argv[1]);
			v_res = atoi(argv[2]);
			argc -= 2; 
			argv += 2;
		}
		else if (strcmp(argv[0],"-t") == 0) {
			flags_time = 1;
		}
		else {
			cout << "Usage : render.exe [options]\n"
				<< "Options: \n"
				<< "  -aa [aa_samples]        Sets the number of anti-aliasing samples\n"
				<< "  -f [type] [max_dist]    Enables fog of a certain type and max distance.\n"
				<< "                          type 1: linear   type 2: squared\n"
				<< "  -fc [red] [blu] [grn]   Input the fog color.\n"
				<< "  -help                   Displays this message\n"
				<< "  -i [inputfile]          Set the input file (.ren)\n"
				<< "  -log                    Logs raw image data to a file\n"
				<< "  -o [outputfile]         Set the output bitmap file\n"
				<< "  -r [h_res] [v_res]      Change the default resolution\n"
				<< "  -t                      Displays time spent rendering\n\n"
				<< "For further information or support, email <protomech@flashmail.com>\n";
				
			return 1;
		}
	}
	return 0;
}

int parse_input(FILE *input, ray *cam, object_node * object_list, light_node * light_list) {
// TODO : clean up, replace with something like get_input(FILE *input, char key[], void * output)
//        make this a lot cleaner...
	
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
				if (num_lights > 0) {
					light_temp->next = new light_node;
					light_temp = light_temp->next;
				}
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
					num_lights++;
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
				if (num_objects > 0) {
					object_temp->next = new object_node;
					object_temp = object_temp->next;
				}
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
					object_temp->object = Primitive(t,point[0],point[1],point[2],obj_color);
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
				}
				else {
					cout << "error in line " << line_num << endl;
					return 1;
				}
				num_objects++;
			}
		}
	}
	cout << "Parsed " << line_num << " line(s) successfully." << endl;
	
	return 0;
}

int write_bitmap_header(FILE *outfile) {

     long filesize = 0x36+v_res*(3*h_res+pad);
     long temp_dec = filesize;

     cout << "filesize: " << filesize << endl;
     int pos;

     fprintf(outfile,"%s","BM");
     for (pos = 2; pos < 6; pos++) {
     	fprintf(outfile,"%c",(char) (temp_dec % 256));
          temp_dec /= 256;
     }
     for (pos = 6; pos < 10; pos++) 
     	fprintf(outfile,"%c", (char) 0x00);
     fprintf(outfile,"%c", (char) 0x36);
     for (pos = 11; pos < 14; pos++) 
     	fprintf(outfile,"%c",(char) 0x00);

// BITMAPINFOHEADER

	fprintf(outfile,"%c", (char) 0x28);
     for (pos = 15; pos < 18; pos++) fprintf(outfile,"%c", (char) 0x00);
     temp_dec = h_res;
     for (pos = 18; pos < 22; pos++) {
     	fprintf(outfile,"%c", (char) (temp_dec % 256));
          temp_dec /= 256;
     }
     temp_dec = v_res;
     for (pos = 22; pos < 26; pos++) {
     	fprintf(outfile,"%c", (char) (temp_dec % 256));
          temp_dec /= 256;
     }
	fprintf(outfile,"%c", (char) 0x01);
     fprintf(outfile,"%c", (char) 0x00);
     fprintf(outfile,"%c", (char) 0x18);
     fprintf(outfile,"%c", (char) 0x00);
     for (pos = 30; pos < 34; pos++) 
     	fprintf(outfile,"%c", (char) 0x00);
     temp_dec = filesize - 0x36;
     for (pos = 34; pos < 38; pos++) {
     	fprintf(outfile,"%c", (char) (temp_dec % 256));
          temp_dec /= 256;
     }
     temp_dec = 3780;
     for (pos = 38; pos < 42; pos++) {
     	fprintf(outfile,"%c", (char) (temp_dec % 256));
          temp_dec /= 256;
     }
     temp_dec = 3780;
     for (pos = 42; pos < 46; pos++) {
     	fprintf(outfile,"%c", (char) (temp_dec % 256));
          temp_dec /= 256;
     }
     for (pos = 46; pos < 54; pos++) 
     	fprintf(outfile,"%c", (char) 0x00);
     
     return 0;
}

color trace(ray ray_trace, object_node *object_list, light_node *light_list, color bgcolor, vector mask) {
	float accumulate_red = 0, accumulate_green = 0, accumulate_blue = 0;
	float temp_magnitude, min_magnitude = 0;
	vector intersection_point, min_intersection_point;
	object_node *place_holder;

	color temp_color;
	object_node *temp = object_list, *min_object, *prev;
	int counter = 0;

	while (temp != NULL) {
		intersection_point = temp->object.Intersect(ray_trace);
		if (magnitude(subtract_vector(intersection_point,mask)) > OBJECT_CLIP) {
			if ((((temp_magnitude = magnitude(subtract_vector(intersection_point,ray_trace.origin))) < min_magnitude) || !min_magnitude) && magnitude(intersection_point) && temp_magnitude > near_clip) {
				min_object = temp;
				min_intersection_point = intersection_point;
				min_magnitude = temp_magnitude;
			}
		}
		temp = temp->next;
	}
	
	if (min_magnitude == 0 && fog_type == 0)
		return bgcolor;
	if (min_magnitude == 0)
		return fog;
		
	color intersection_color = min_object->object.ColorAt(min_intersection_point);
	
	accumulate_red   = intersection_color.r;
	accumulate_green = intersection_color.g;
	accumulate_blue  = intersection_color.b;
	
	temp = object_list;
	
	if (min_magnitude < fog_clip && fog_type == 1) {	
		accumulate_red   *= (fog_clip-min_magnitude)/fog_clip;
		accumulate_green *= (fog_clip-min_magnitude)/fog_clip;
		accumulate_blue  *= (fog_clip-min_magnitude)/fog_clip;
		
		accumulate_red   += (min_magnitude/fog_clip)*fog.r;
		accumulate_green += (min_magnitude/fog_clip)*fog.g;
		accumulate_blue  += (min_magnitude/fog_clip)*fog.b;
	}
	else if (min_magnitude < fog_clip && fog_type == 2) {
		accumulate_red   *= ((fog_clip-min_magnitude)/fog_clip)*((fog_clip-min_magnitude)/fog_clip);
		accumulate_green *= ((fog_clip-min_magnitude)/fog_clip)*((fog_clip-min_magnitude)/fog_clip);
		accumulate_blue  *= ((fog_clip-min_magnitude)/fog_clip)*((fog_clip-min_magnitude)/fog_clip);
		
		accumulate_red   += (1 - ((fog_clip-min_magnitude)/fog_clip)*((fog_clip-min_magnitude)/fog_clip))*fog.r;
		accumulate_green += (1 - ((fog_clip-min_magnitude)/fog_clip)*((fog_clip-min_magnitude)/fog_clip))*fog.g;
		accumulate_blue  += (1 - ((fog_clip-min_magnitude)/fog_clip)*((fog_clip-min_magnitude)/fog_clip))*fog.b;
	}
	else if (fog_type != 0 && min_magnitude > fog_clip)
		return fog;
		
	unsigned char min_obj_alpha, min_obj_ref;
	
	if ((min_obj_alpha = (min_object->object.ColorAt(min_intersection_point)).a) != 0) { // if polygon is transparent
		accumulate_red   *= (255.0 - min_obj_alpha)/255;
		accumulate_green *= (255.0 - min_obj_alpha)/255;
		accumulate_blue  *= (255.0 - min_obj_alpha)/255;
		
		temp_color = trace(ray(min_intersection_point,ray_trace.trace), object_list, light_list, bgcolor, min_intersection_point);
		
		accumulate_red   += temp_color.r * min_obj_alpha/255.0;
		accumulate_green += temp_color.g * min_obj_alpha/255.0;
		accumulate_blue  += temp_color.b * min_obj_alpha/255.0;
	} 
	
	if ((min_obj_ref = (min_object->object.ColorAt(min_intersection_point)).ref) != 0) { // if polygon has reflectivity
		accumulate_red   *= (255.0 - min_obj_ref)/255;
		accumulate_green *= (255.0 - min_obj_ref)/255;
		accumulate_blue  *= (255.0 - min_obj_ref)/255;
		
		vector reflection = min_object->object.Reflect(ray_trace);
		
		temp_color = trace(ray(min_intersection_point, reflection), object_list, light_list, bgcolor, min_intersection_point);
		
		accumulate_red   += temp_color.r * min_obj_ref/255.0;
		accumulate_green += temp_color.g * min_obj_ref/255.0;
		accumulate_blue  += temp_color.b * min_obj_ref/255.0;
	}
	
	light_node * light_temp = light_list;
	vector original_trace_vector = subtract_vector(ray_trace.origin,min_intersection_point);
	vector light_trace_vector, light_intersection;
	color light_intersection_color;
	
	float illumination_accumulate_red   = ambient_light.r,
		 illumination_accumulate_green = ambient_light.g,
		 illumination_accumulate_blue  = ambient_light.b;
	float illumination_red, illumination_green, illumination_blue, scale;
	
	while (light_temp != NULL) {
		light_trace_vector = subtract_vector(light_temp->l.pos,min_intersection_point);
		illumination_red   = light_temp->l.hue.r;
		illumination_green = light_temp->l.hue.g;
		illumination_blue  = light_temp->l.hue.b;
		
		temp = object_list;
		while (temp != NULL) {
			if (temp != min_object) {
				if (magnitude(light_intersection = temp->object.Intersect(ray(min_intersection_point,light_trace_vector)))) {
					light_intersection_color = temp->object.ColorAt(light_intersection);
					illumination_red   *= light_intersection_color.a/255.0;
					illumination_green *= light_intersection_color.a/255.0;
					illumination_blue  *= light_intersection_color.a/255.0;
			
					if (light_intersection_color.a == 0) break;
				}
			}
			temp = temp->next;
		}
		
		vector normal = min_object->object.Normal(min_intersection_point);

		normal = normalize(normal);
		light_trace_vector = normalize(light_trace_vector);

		if (dot_product(original_trace_vector,normal) > 0) {
			if ((scale = dot_product(normal,light_trace_vector)) < 0)
				scale = 0;
		}
		else {		
			if ((scale = dot_product(scalar_multiply(normal,-1),light_trace_vector)) < 0)
				scale = 0;
		}
	
		illumination_accumulate_red   += illumination_red  *scale;
		illumination_accumulate_green += illumination_green*scale;
		illumination_accumulate_blue  += illumination_blue *scale;
		light_temp = light_temp->next;
	}
	
	accumulate_red   *= min(illumination_accumulate_red,  255.0)/255.0;
	accumulate_green *= min(illumination_accumulate_green,255.0)/255.0;
	accumulate_blue  *= min(illumination_accumulate_blue, 255.0)/255.0;
		
	return color((unsigned char) accumulate_red,(unsigned char) accumulate_green,(unsigned char) accumulate_blue,0,0);
}


// Primitive Rendering Engine
// v.alpha
// (c) 2000 Michael Beatty
//
// Environment settings: set tab to 5 spaces
//
// todo:
// reflective
// textures
// texture mipmapping? (necessary?)
// convert to .tga format (?)
// fix precision bugs
// fix assorted memory leaks (nodelist)
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
#include "primitives.h"	// special primitives file

#define H_RES 640
#define V_RES 480

#define H_FOV 1.57
#define V_FOV 1.28

#define MAXLINE 1000

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
void print_nodelist(node *n);
void print_lightlist(light_node *l);
int parse_args(int argc, char *argv[]);
int write_bitmap_header(FILE *outfile);
int parse_input(FILE *input, ray *r, node *head, light_node *light_head, tex_list * tex_head);

color trace(ray init_trace, node *head, light_node *light_head, color bgcolor, node *mask);

void main(int argc, char *argv[]) {
	
	if (parse_args(argc,argv) == 1) return 0;
	
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
	
	node * head = new node;
	light_node * light_head = new light_node;
	tex_list * tex_head = new tex_list;
	
	if (input_filename == NULL)
		head = new node(polygon(triangle(vector(0,1,4), vector(-1,-1,4), vector(1,-1,4)),color(255,255,255,0,0)),
			  new node(polygon(triangle(vector(0,.5,8), vector(-.5,-.5,3.5), vector(.5,-.5,2)),color(0,255,255,0,0)),
			  NULL));
	else {
		FILE *input = fopen(input_filename,"r");
		if (parse_input(input,&cam_pos,head,light_head,tex_head) != 0) {
			cout << "error in input file " << input_filename << ". Aborting..";
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
					
					return_color = trace(ray_trace,head,light_head,bgcolor,NULL);

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

void print_nodelist(node *n) {
	int counter = 1;
	while (n != NULL) {
		cout << "Polygon " << counter << ": " << endl;
		cout << "  vertex 1: "; print_vector(n->p.t.p1);
		cout << "  vertex 2: "; print_vector(n->p.t.p2);
		cout << "  vertex 3: "; print_vector(n->p.t.p3);
		cout << "  color   : (" << (int)n->p.col.r << "," << (int)n->p.col.g << "," << (int)n->p.col.b << ")\n\n";
		
		n = n->next;
		counter++;
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
				<< "  -t                      Displays time spent rendering\n"
				<< endl
				<< "For further information or support, email <protomech@flashmail.com>";
				
			return 1;
		}
	}
	return 0;
}

void print_ptr(char *s) {
	while (*s)
		printf("%c",*s++);
}

int parse_input(FILE *input, ray *cam, node *head, light_node *light_head, tex_list *tex_head) {
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
	
	char *ptr;
	
/*	fgets(line,MAXLINE,input);
	if (sscanf(line,"%f %f %f %f %f %f",&r->origin.x,&r->origin.y,&r->origin.z,&theta,&phi) != 5) {
		printf("header error.\n");
		return 1;
	}
	
	r->trace = spherical_to_cartesian(spherical_vector(theta,phi,1));
	char *ptr, *buf;
	int format_hits; */
		
while (!feof(input)) {
	if (fgets(line,MAXLINE,input)) {
		line_counter++;
		if ((ptr = strstr(line,"//")) != NULL) // remove commented code
			*ptr = 0;
		if (strstr(line,"Camera {")) {
			bool pos = false, view = false;
			do {
				if (!feof(input) && fgets(line,MAXLINE,input)) {
					line_counter++;
					if ((ptr = strstr(line,"CameraPos")) != NULL) {
						float x, y, z;
						ptr += strlen("CameraPos");
						if (sscanf(ptr,"%f %f %f",&x,&y,&z) == 3) {
							cam->origin.x = x;
							cam->origin.y = y;
							cam->origin.z = z;
							pos = true;
						}
						else {
							printf("Syntax error on line %d: %s",line_counter,line);
							printf("%s",ptr);
							return 1;
						}
					}
					if ((ptr = strstr(line,"CameraView")) != NULL) {
						float theta, phi;
						ptr += strlen("CameraView");
						if (sscanf(ptr,"%f %f",&theta,&phi) == 2) {
							cam->trace = spherical_to_cartesian(spherical_vector(theta,phi,1));
							view = true;
						}
						else {
							printf("Syntax error on line %d: %s",line_counter,line);
							return 1;
						}
					}
				}
				else {
					printf("EOF on line %d.\n",line_counter);
					return 1;
				}
			} while (!strstr(line,"}"));
			
			if (!pos || !view) {
				printf("Invalid Camera specification, line %d\n",line_counter);
				return 1;
			}
		}
		if (strstr(line,"Light {")) {
			do {
				if (!feof(input) && fgets(line,MAXLINE,input)) {
					line_counter++;
					if ((ptr = strstr(line,"Ambient")) != NULL) {
						unsigned char r, g, b;
						ptr += strlen("Ambient");
						if (sscanf(ptr," %d %d %d",&r,&g,&b) == 3) {
							ambient_light = color(r,g,b);
						}
						else {
							printf("Syntax error on line %d: %s",line_counter,line);
							return 1;
						}
					}
					if (strstr(line,"Point {")) {
						if (num_lights > 0) {
							light_temp->next = new light_node;
							light_temp = light_temp->next;
						}
						bool pos = false, color = false, angle = false, fov = false;
						do {
							if (!feof(input) && fgets(line,MAXLINE,input)) {
								line_counter++;
								if ((ptr = strstr(line,"Pos")) != NULL) {
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
								}
								if ((ptr = strstr(line,"Hue")) != NULL) {
									unsigned char r, g, b;
									ptr += strlen("Hue");
									if (sscanf(ptr,"%d %d %d",&r,&g,&b) == 3) {
										print_ptr(ptr); printf("\n");
										light_temp->l.hue.r = r;
										light_temp->l.hue.g = g;
										light_temp->l.hue.b = b;
										color = true;
									}
									else {
										printf("Syntax error on line %d: %s",line_counter,line);
										return 1;
									}
								}
							}
							else {
								printf("EOF on line %d.\n",line_counter);
								return 1;
							}
						} while (!(ptr = strstr(line,"}")));
						
						*ptr = 0;
			
						if (!((pos && color && !angle && !fov) || (pos && color && angle && fov))) {
							printf("Invalid Point Light specification, line %d %d %d %d %d\n",line_counter,pos,color,angle,fov);
							return 1;
						}
						num_lights++;
						
						cout << "Point Light: " << (int) light_temp->l.hue.r << " " << (int) light_temp->l.hue.g << " " << (int)light_temp->l.hue.b << endl;
					}
				}
				else {
					printf("EOF on line %d.\n",line_counter);
					return 1;
				}
			} while (!strstr(line,"}"));
		}
		if (strstr(line,"ObjectList {")) {
			do {
				if (!feof(input) && fgets(line,MAXLINE,input)) {
					line_counter++;
					if (strstr(line,"Poly {")) {
						if (num_poly > 0) {
							poly_temp->next = new node;
							poly_temp = poly_temp->next;
						}
						int num_vertex = 0;
						vector vtx[3];
						color poly_color;
						tex_list * poly_tex;
						bool color = false, tex = false;
						do {
							if (!feof(input) && fgets(line,MAXLINE,input)) {
								line_counter++;
								if ((ptr = strstr(line,"Vertex")) != NULL) {
									float x, y, z;
									ptr += strlen("Vertex");
									if (sscanf(ptr,"%f %f %f",&x,&y,&z) == 3) {
										vtx[num_vertex] = vector(x,y,z);
										num_vertex++;
									}
									else {
										printf("Syntax error on line %d: %s",line_counter,line);
										return 1;
									}
								}
								if ((ptr = strstr(line,"Color")) != NULL) {
									unsigned char r, g, b, a, ref;
									ptr += strlen("Color");
									if (sscanf(ptr,"%d %d %d %d %d",&r,&g,&b,&a,&ref) == 5) {
										poly_color.r = r;
										poly_color.g = g;
										poly_color.b = b;
										poly_color.a = a;
										poly_color.ref = ref;
										color = true;
									}
									else {
										printf("Syntax error on line %d: %s",line_counter,line);
										return 1;
									}
								}
/*								if ((ptr = strstr(line,"Texture")) != NULL) {
									char * first, * second, tex_fname[100];
									ptr += strlen("Texture");
									
									if (sscanf(ptr,"%s",tex_fname)) {
										if (tex_fname == NULL) 
											cout << "NULL!\n";
										else
											cout << tex_fname << endl;
										if ((poly_tex = tex_list_member(tex_fname,tex_temp)) == NULL) {
											tex_temp->tex = read_targa(tex_fname);
											poly_tex->tex = tex_temp->tex;
											tex_temp = tex_temp->next;
										}
									}
									else {
										printf("Syntax error on line %d: %s",line_counter,line);
										return 1;
									}
								} */
							}
							else {
								printf("EOF on line %d.\n",line_counter);
								return 1;
							}
						} while (!(ptr = strstr(line,"}")));
						
						*ptr = 0;
			
						if (num_vertex < 3 || (!color && !tex)) {
							printf("Invalid Poly specification, line %d\n",line_counter);
							return 1;
						}

						poly_temp->p = polygon(triangle(vtx[0],vtx[1],vtx[2]),poly_color);
						poly_temp->p.tex = poly_tex->tex;
						num_poly++;
					}
				}
				else {
					printf("EOF on line %d.\n",line_counter);
					return 1;
				}
			} while (!strstr(line,"}"));
		}
	}
}

	printf("Parsed %d lines successfully.\n",line_counter);
	
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

color trace(ray ray_trace, node *head, light_node *light_head, color bgcolor, node *mask) {
	float accumulate_red = 0, accumulate_green = 0, accumulate_blue = 0;
	float temp_magnitude, min_magnitude = 0;
	vector intersection_point, min_intersection_point;
	node *place_holder;

	color temp_color;
	node *temp = head, *min_poly, *prev;
	int counter = 0;

	while (temp != NULL) {
		if (temp != mask) {
			intersection_point = intersect(ray_trace,temp->p);
			if ((((temp_magnitude = magnitude(subtract_vector(intersection_point,ray_trace.origin))) < min_magnitude) || !min_magnitude) && magnitude(intersection_point) && temp_magnitude > near_clip) {
				min_poly = temp;
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
	
/*	if (min_poly->p.tex.data != NULL) {
		cout << "tex: " << min_poly->p.tex.width << " " << min_poly->p.tex.height << endl; // << " " << (int)dmod(10*(min_intersection_point.x - min_poly->p.origin.x),min_poly->p.tex.width) << " " << (int)dmod(10*(min_intersection_point.z - min_poly->p.origin.z),min_poly->p.tex.height) << endl;
	} /*
		rgb_3 blah_col = access_tex(min_poly->p.tex,(int)dmod(10*(min_intersection_point.x - min_poly->p.origin.x),min_poly->p.tex.width),(int)dmod(10*(min_intersection_point.z - min_poly->p.origin.z),min_poly->p.tex.height));
		accumulate_red   = blah_col.r;
		accumulate_green = blah_col.g;
		accumulate_blue  = blah_col.b;
	}
	else { */
		accumulate_red   = min_poly->p.col.r;
		accumulate_green = min_poly->p.col.g;
		accumulate_blue  = min_poly->p.col.b;
//	}
	
	temp = head;
	
	if (min_poly->p.col.a != 0) { // if polygon is transparent
		accumulate_red   *= (255.0 - min_poly->p.col.a)/255;
		accumulate_green *= (255.0 - min_poly->p.col.a)/255;
		accumulate_blue  *= (255.0 - min_poly->p.col.a)/255;
		
		temp_color = trace(ray(min_intersection_point,ray_trace.trace), head, light_head, bgcolor, min_poly);
		
		accumulate_red   += temp_color.r * min_poly->p.col.a/255.0;
		accumulate_green += temp_color.g * min_poly->p.col.a/255.0;
		accumulate_blue  += temp_color.b * min_poly->p.col.a/255.0;
	} 
	
	if (min_poly->p.col.ref != 0) { // if polygon has reflectivity
		accumulate_red   *= (255.0 - min_poly->p.col.ref)/255;
		accumulate_green *= (255.0 - min_poly->p.col.ref)/255;
		accumulate_blue  *= (255.0 - min_poly->p.col.ref)/255;
		
		vector reflection = reflect_vector(min_poly->p.normal,ray_trace.trace);
		
		temp_color = trace(ray(min_intersection_point, reflection), head, light_head, bgcolor, min_poly);
		
		accumulate_red   += temp_color.r * min_poly->p.col.ref/255.0;
		accumulate_green += temp_color.g * min_poly->p.col.ref/255.0;
		accumulate_blue  += temp_color.b * min_poly->p.col.ref/255.0;
	}
	
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
		
	light_node * light_temp = light_head;
	vector original_trace_vector = subtract_vector(ray_trace.origin,min_intersection_point);
	vector light_trace_vector;
	float illumination_accumulate_red   = ambient_light.r,
		 illumination_accumulate_green = ambient_light.g,
		 illumination_accumulate_blue  = ambient_light.b;
	float illumination_red, illumination_green, illumination_blue, scale;
	
	while (light_temp != NULL) {
		light_trace_vector = subtract_vector(light_temp->l.pos,min_intersection_point);
		illumination_red   = light_temp->l.hue.r;
		illumination_green = light_temp->l.hue.g;
		illumination_blue  = light_temp->l.hue.b;
		
		temp = head;
		while (temp != NULL) {
			if (temp != min_poly) {
				if (magnitude(intersect(ray(min_intersection_point,light_trace_vector),temp->p))) {
					illumination_red   *= temp->p.col.a/255.0;
					illumination_green *= temp->p.col.a/255.0;
					illumination_blue  *= temp->p.col.a/255.0;
			
					if (temp->p.col.a == 0) break;
				}
			}
			temp = temp->next;
		}

		if (dot_product(original_trace_vector,min_poly->p.normal) > 0) {
				if ((scale = dot_product(min_poly->p.normal,light_trace_vector)) < 0)
				scale = 0;
			else 
				scale /= float(magnitude(min_poly->p.normal)*magnitude(light_trace_vector));
		}
		else {		
			if ((scale = dot_product(scalar_multiply(min_poly->p.normal,-1),light_trace_vector)) < 0)
					scale = 0;
			else 
				scale /= float(magnitude(min_poly->p.normal)*magnitude(light_trace_vector));
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

color light_trace(vector pos, light l, node *head) {
}

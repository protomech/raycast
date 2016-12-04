#include <stdio.h>
#include <iostream.h>
#include <stdlib.h>
#include <string.h>

struct rgb_3 {			// red green blue triple
	unsigned char r,g,b;
	rgb_3(void) : r(0),g(0),b(0) { }
	rgb_3(unsigned char r_,unsigned char g_,unsigned char b_) :
		r(r_),g(g_),b(b_) { }
};

struct texture {
	unsigned int width, height;
	rgb_3 *data;
	texture (void) :
		width(0),height(0),data(NULL) { }
	texture (unsigned int w, unsigned int h, rgb_3 *raw) :
		width(w),height(h),data(raw) { }
};

rgb_3 access_tex(texture t, unsigned int x, unsigned int y) {
	rgb_3 *ptr;
	if (x < t.width && y < t.height) {
		ptr = (t.data + y*t.width + x);
		return rgb_3(ptr->r,ptr->g,ptr->b);
	}
	return rgb_3();
}		

struct tex_list {
	char * filename;
	texture tex;
	tex_list * next;
	tex_list(void) : filename(NULL), tex(), next(NULL) { }
	tex_list(char *fname, texture t, tex_list *nxt) :
		filename(fname),tex(t),next(nxt) { }
};

tex_list * tex_list_member(char *fname, tex_list *head) {
	tex_list * t = head;
	
	while (t != NULL) {
		if (!stricmp(fname,t->filename))
			return t;
		t = t->next;
	}
	
	return NULL;
}		

unsigned int low_hi(FILE *in) {
	unsigned int accum = 0, temp = 0;
	fscanf(in,"%c",&temp);
	accum = temp;
	fscanf(in,"%c",&temp);
	accum += 256*temp;
	
	return accum;
}

texture read_targa(char * filename) {
	// TGA Data Type 2 Input 
	// reads in a Targa (.tga) file, unmapped RGB images
	//
	// for more information, 

	FILE *in;
	
	if ((in = fopen(filename,"rb")) == NULL) {
		cout << "file error: " << filename << endl;
		return texture();
	}
	
// header information					  	Offset	Length
	unsigned char num_chars_in_id_field;	//	0		1
	unsigned char color_map_type;			// 	1		1
	unsigned char image_type_code;		// 	2		1
	
	// color map specification; curr. ignored	3		5
	unsigned int color_map_origin;		//	3		2
	unsigned int color_map_length;		//	5		2
	unsigned char color_map_entry_size;	//	7		1
	
	// image specification					8		10
	unsigned int x_origin;				//	8		2
	unsigned int y_origin;				// 	10		2
	unsigned int image_width;			//	12		2
	unsigned int image_height;			//	14		2
	unsigned char image_pixel_size;		//	16		1
	unsigned char image_descriptor_byte;	//	17		1
	
/*	bit pattern 76543210
			  ^^			data storage interleaving flag - ignored
			    ^		screen origin bit - 0 lower left, 1 upper left
			     ^		reserved - must be 0.
			      ^^^^	attribute bits.  for Targa 24, should be 0.
*/			

	// image identification field - length given by info in byte 0
	// currently ignored.
	unsigned char * image_id_field; 		//	18		variable
	
	// color map data - if byte 0 is 0, then this length is 0.
	// currently ignored.
	unsigned char * color_map_data;		// 	variable	variable
	
	// image data - this is the Good Stuff (tm).
	rgb_3 * image_data; // = (rgb_3 *) malloc (width*height*3);
	
	unsigned char buf;					// data to be ignored..
	unsigned int i, j, ptr_pos;
	rgb_3 * rgb_ptr;

     unsigned char a, b;

	fscanf(in,"%c",&num_chars_in_id_field);
	fscanf(in,"%c",&color_map_type);
	fscanf(in,"%c",&image_type_code);
	
     fscanf(in,"%c%c",&a,&b); color_map_origin = a + (unsigned int)b*256;
     fscanf(in,"%c%c",&a,&b); color_map_length = a + (unsigned int)b*256;
	fscanf(in,"%c",&color_map_entry_size);

     fscanf(in,"%c%c",&a,&b); x_origin     = a + (unsigned int)b*256;
     fscanf(in,"%c%c",&a,&b); y_origin     = a + (unsigned int)b*256;
     fscanf(in,"%c%c",&a,&b); image_width  = a + (unsigned int)b*256;
     fscanf(in,"%c%c",&a,&b); image_height = a + (unsigned int)b*256;
	fscanf(in,"%c",&image_pixel_size);
	fscanf(in,"%c",&image_descriptor_byte);
	
	for (i = 0; i < num_chars_in_id_field; i++)
		fscanf(in,"%c",&buf);
	
	for (i = 0; i < color_map_length*color_map_entry_size; i++)
		fscanf(in,"%c",&buf);
		
	image_data = (rgb_3 *) malloc(image_width*image_height*3);
	
	
	if (image_descriptor_byte & 0x20) {		// data begins in upper left corner
		rgb_ptr = image_data;
		for (i = 0; i < image_width*image_height; i++, rgb_ptr++) {
			fscanf(in,"%c",&(rgb_ptr->r));
			fscanf(in,"%c",&(rgb_ptr->g));
			fscanf(in,"%c",&(rgb_ptr->b));
		}
	}
	else {								// data begins in lower left corner
		rgb_ptr = image_data + (image_height-1)*image_width;
		
		for (i = image_height-1; i > 0; i--, rgb_ptr -= image_width) {
			for (j = 0; j < image_width; j++, rgb_ptr++) {
				fscanf(in,"%c",&(rgb_ptr->r));
				fscanf(in,"%c",&(rgb_ptr->g));
				fscanf(in,"%c",&(rgb_ptr->b));
				rgb_ptr++;
			}
		}
	}

	return texture(image_width,image_height,image_data);
}	

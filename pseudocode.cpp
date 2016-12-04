for each pixel
	for each subpixel 
		locate ray
		for each (object.intersect(ray).mag > 0)
			if (intersect_point.mag < min_mag)
				min_obj = object
				min_pt = intersection_point
		
		accum += min_obj.color(min_pt)
	
		// transparencies
		if (min_obj.color(min_pt).alpha > 0) 
			accum *= min_obj.color(min_pt).alpha
			accum += trace(ray(min_pt,r.trace))*(255-alpha)/255.0
		
		// reflectivity
		if (min_obj.color(min_pt).ref > 0)
			accum *= min_obj.color(min_pt).ref
			accum += trace(ray(min_pt,reflect(min_obj,min_pt,r.trace)))*(255-ref)/255.0
			
		return accum
	average all accum

write pixel
		
		
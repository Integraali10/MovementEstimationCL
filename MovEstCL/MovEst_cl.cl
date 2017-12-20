
__kernel void mov_estimation()
{ 

}
//__read_only  image2d_t src_image
//__read_only  image2d_t ref_image 
//
__kernel void mov_reanimation(__read_only  image2d_t src_image, __read_only  image2d_t ref_image, __global short2 * motion_vector_buffer, __write_only image2d_t out_image)
{ 

  const int xout = get_global_id(0);
  const int yout = get_global_id(1);
  const sampler_t sampler = CLK_NORMALIZED_COORDS_FALSE | CLK_ADDRESS_CLAMP | CLK_FILTER_NEAREST;
  float4 pixel;
  pixel = read_imagef(ref_image, sampler, (int2)(xout, yout));
  write_imagef(out_image, (int2)(xout, yout), pixel);

}

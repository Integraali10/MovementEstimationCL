
__kernel void mov_estimation()
{ 

}
//__read_only  image2d_t src_image
//__read_only  image2d_t ref_image 
//

#define MotBlo 16
__kernel void mov_reanimation(__read_only  image2d_t src_image, __read_only  image2d_t ref_image, __global short2 * motion_vector_buffer, __write_only image2d_t out_image, int widthinMB)
{ 
  uint xol = get_local_id(0);
  uint yol = get_local_id(1);
  uint xgr = get_group_id(0);
  uint ygr = get_group_id(1);

  short2 er = motion_vector_buffer[ygr*widthinMB + xgr];
  const sampler_t sampler = CLK_NORMALIZED_COORDS_FALSE | CLK_ADDRESS_CLAMP | CLK_FILTER_NEAREST;
  float4 pixel;

  int xout = xgr*MotBlo + xol;
  int yout = ygr*MotBlo + yol;

  pixel = read_imagef(src_image, sampler, (int2)(xout - er.x, yout - er.y));
  write_imagef(out_image, (int2)(xout, yout), pixel);
  ///могли ли тут подраться к чортовой матери?
  ///нет, похоже, что не сильно дерется
}

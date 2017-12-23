#define _CRT_SECURE_NO_WARNINGS
#include <CL\cl.h>
#include <CL\cl_ext.h>
#include <stdio.h>
#include <fstream>
#include <setjmp.h>
#include <vector>
#include <iomanip>

extern "C" {
#include "jpeglib.h"
}

typedef unsigned char u8;
typedef short i16;
typedef unsigned short u16;

static const double aanscalefactor[8] =
{
  1.0, 1.3870398453221475, 1.3065629648763766, 1.1758756024193588,
  1.0, 0.7856949583871022, 0.5411961001461971, 0.2758993792829431,
};

void jpeg_init_fdct_table(const u16 *quantptr, float quant_table[64])
{
  for (int row = 0, i = 0; row < 8; row++)
    for (int col = 0; col < 8; i++, col++)
      quant_table[i] = 0.125 / (aanscalefactor[row] * aanscalefactor[col] * quantptr[i]); // 1/64
}

void jpeg_init_idct_table(const u16 *quantptr, float quant_table[64])
{
  for (int row = 0, i = 0; row < 8; row++)
    for (int col = 0; col < 8; i++, col++)
      quant_table[i] = quantptr[i] * aanscalefactor[row] * aanscalefactor[col] * 0.125;
}

struct my_error_mgr {
  struct jpeg_error_mgr pub;

  jmp_buf setjmp_buffer;
};

typedef struct my_error_mgr * my_error_ptr;

METHODDEF(void)
my_error_exit(j_common_ptr cinfo)
{
  my_error_ptr myerr = (my_error_ptr)cinfo->err;
  (*cinfo->err->output_message) (cinfo);
  longjmp(myerr->setjmp_buffer, 1);
}


void PGMwriting(JSAMPARRAY pich, int w, int h, char *filename, char num)
{
  FILE * infile;
  int n = (int)strlen(filename);
  char *newname = new char[n + 1];
  sprintf(newname, "%.*s%c.pgm", n - 4, filename, num);
  infile = fopen(newname, "wb");
  fprintf(infile, "P5\n%i %i\n255\n", w, h);
  for (int i = 0; i < (int)(h); i++)
  {
    fwrite(pich[i], 1, w, infile);
  }
  fclose(infile);
}


int read_JPEG_file(char * filename, u8 *pich, size_t *w, size_t *wreal, size_t *h, size_t *hreal, float fquant[64], float iquant[64])
{
  struct jpeg_decompress_struct cinfo;
  struct my_error_mgr jerr;
  FILE * infile;
  JSAMPARRAY buffer;
  int row_stride;

  if ((infile = fopen(filename, "rb")) == NULL) {
    fprintf(stderr, "can't open %s\n", filename);
    return 0;
  }
  cinfo.err = jpeg_std_error(&jerr.pub);
  jerr.pub.error_exit = my_error_exit;
  if (setjmp(jerr.setjmp_buffer)) {
    jpeg_destroy_decompress(&cinfo);
    fclose(infile);
    return 0;
  }

  jpeg_create_decompress(&cinfo);
  jpeg_stdio_src(&cinfo, infile);
  (void)jpeg_read_header(&cinfo, TRUE);
  *(wreal) = cinfo.image_width;
  *(hreal) = cinfo.image_height;
  cinfo.image_width = cinfo.image_width + 7 & ~7;
  cinfo.image_height = cinfo.image_height + 7 & ~7;
  cinfo.out_color_space = JCS_GRAYSCALE;
  (void)jpeg_start_decompress(&cinfo);
  row_stride = cinfo.output_width * cinfo.output_components;

  buffer = (*cinfo.mem->alloc_sarray)
    ((j_common_ptr)&cinfo, JPOOL_IMAGE, row_stride, 1);

  if (pich != NULL) {
    u16 quant[64];
    JQUANT_TBL** quants = cinfo.quant_tbl_ptrs;
    for (int i = 0; i < 64; i++) {
      quant[i] = quants[0]->quantval[i];
    }
    jpeg_init_fdct_table(quant, fquant);
    jpeg_init_idct_table(quant, iquant);

    printf("%i x %i \n", cinfo.output_height, cinfo.output_width);
  }
  *(w) = cinfo.output_width;
  *(h) = cinfo.output_height;
  JSAMPARRAY a = new JSAMPROW[cinfo.output_height];
  if (pich == NULL)
    return 1;
  int i = 0;
  while (cinfo.output_scanline < cinfo.output_height) {
    (void)jpeg_read_scanlines(&cinfo, buffer, 1);
    a[i] = new JSAMPLE[row_stride];
    for (int j = 0; j < row_stride; j++)
    {
      a[i][j] = buffer[0][j];
    }
    i++;
  }
  for (unsigned int y = 0; y < cinfo.output_height; y++)
  {
    for (int z = 0; z < row_stride; z++)
    {
      pich[y*row_stride + z] = a[y][z];
    }
  }

  PGMwriting(a, cinfo.output_width, cinfo.output_height, filename, '0');
  (void)jpeg_finish_decompress(&cinfo);
  jpeg_destroy_decompress(&cinfo);

  return 1;
}

#define INTELWEWANTDEBUG false //chose CPU
#define INTELWEWANTSOMEANALYSIS true //just intel fun

void main() {
  // reading cl file
  FILE * ptrFile = fopen("MovEst_cl.cl", "rb");
  // put pointer to the end of file
  fseek(ptrFile, 0, SEEK_END);
  // get the size of text file
  long lSize = ftell(ptrFile);
  // put pointer to the begin of file
  fseek(ptrFile, 0, SEEK_SET);
  // allocating memory
  char * buff = (char*)malloc(sizeof(char) * lSize);
  // read our file to array
  size_t result = fread(buff, 1, lSize, ptrFile);
  fclose(ptrFile);

  ////piches
  char *filename0 = "exp/chat0.jpg";
  char *filename1 = "exp/chat1.jpg";
  u8 *pSrcBuf = NULL;
  u8 *pRefBuf = NULL;
  float fquant[64], iquant[64];
  size_t w, h, wreal, hreal;
  read_JPEG_file(filename0, pSrcBuf, &w, &wreal, &h, &hreal, fquant, iquant); //to-do - add color component or write new function for that
  pSrcBuf = new u8[w*h];
  read_JPEG_file(filename1, pRefBuf, &w, &wreal, &h, &hreal, fquant, iquant);
  pRefBuf = new u8[w*h];
  read_JPEG_file(filename0, pSrcBuf, &w, &wreal, &h, &hreal, fquant, iquant);
  read_JPEG_file(filename1, pRefBuf, &w, &wreal, &h, &hreal, fquant, iquant);

  u8 *pOutBuf = new u8[w*h];
  //nu heb ik een pich, w, h, fquant, iquant

  //get platform id
  cl_uint refCount;
  cl_int err;

  //get device's ids
  cl_uint platformIdCount = 0;//
  clGetPlatformIDs(NULL, NULL, &platformIdCount);
  cl_platform_id* platforms = new cl_platform_id[platformIdCount];
  cl_platform_id  platform = 0;
  clGetPlatformIDs(platformIdCount, platforms, &platformIdCount);
  printf("===%d OpenCL platform(s) found: ===\n", platformIdCount);
  for (unsigned int i = 0; i < platformIdCount; ++i)
  {
    static char buffer[10240];
    clGetPlatformInfo(platforms[i], CL_PLATFORM_VENDOR, 10240, buffer, NULL);
    printf("PLATFORM_VENDOR = %s\n", buffer);
    if ((INTELWEWANTSOMEANALYSIS || INTELWEWANTDEBUG) && (buffer[0] == 'I'))
    {
      platform = platforms[i];
    }
    else platform = platforms[0];
  }

  cl_device_id devices[3];
  cl_device_id device;
  cl_uint devicesn = 0;
  int errordevice = 1;
  if (INTELWEWANTDEBUG) {
    errordevice = clGetDeviceIDs(platform, CL_DEVICE_TYPE_CPU, 3, &device, &devicesn);
    printf("DEBUG MODE ON\n");
  }
  else if (errordevice != CL_SUCCESS) {
    clGetDeviceIDs(platform, CL_DEVICE_TYPE_ALL, 3, devices, &devicesn);
    device = devices[0];
    printf("DESTINY MODE ON\n");
  }
  printf("===========<ACTUAL DEVICE>=========\n");
  static size_t bufferst[10240];
  cl_uint buf_uint;
  cl_ulong buf_ulong;
  static char buffer[1000];
  clGetDeviceInfo(device, CL_DEVICE_NAME, sizeof(buffer), buffer, NULL);
  printf(" DEVICE_NAME = %s\n", buffer);
  clGetDeviceInfo(device, CL_DEVICE_VENDOR, sizeof(buffer), buffer, NULL);
  printf(" DEVICE_VENDOR = %s\n", buffer);
  clGetDeviceInfo(device, CL_DEVICE_VERSION, sizeof(buffer), buffer, NULL);
  printf(" DEVICE_VERSION = %s\n", buffer);
  clGetDeviceInfo(device, CL_DRIVER_VERSION, sizeof(buffer), buffer, NULL);
  printf(" DRIVER_VERSION = %s\n", buffer);
  clGetDeviceInfo(device, CL_DEVICE_MAX_COMPUTE_UNITS, sizeof(buf_uint), &buf_uint, NULL);
  printf(" DEVICE_MCU = %u\n", (unsigned int)buf_uint);
  clGetDeviceInfo(device, CL_DEVICE_GLOBAL_MEM_SIZE, sizeof(buf_ulong), &buf_ulong, NULL);
  printf(" DEVICE_GMS = %llu\n", (unsigned long long)buf_ulong);
  clGetDeviceInfo(device, CL_DEVICE_LOCAL_MEM_SIZE, sizeof(buf_ulong), &buf_ulong, NULL);
  printf(" DEVICE_LocalMemSize = %llu\n", (unsigned long long)buf_ulong);
  clGetDeviceInfo(device, CL_DEVICE_MAX_WORK_GROUP_SIZE, sizeof(buf_ulong), &buf_ulong, NULL);
  printf(" DEVICE_work_group_size = %llu\n", (unsigned long long)buf_ulong);
  clGetDeviceInfo(device, CL_DEVICE_MAX_WORK_ITEM_SIZES, sizeof(bufferst), bufferst, NULL);
  printf(" DEVICE_work_item_sizes = %zu, %zu, %zu \n", bufferst[0], bufferst[1], bufferst[2]);
  printf("==========<\\ACTUAL DEVICE>=========\n");


  size_t sizeContext, len_prog;

  //creating context
  cl_context context;
  context = clCreateContext(NULL, 1, &device, NULL, NULL, NULL);
  if (!context) {
    printf("%s\n", "ContextError");
  }
  clGetContextInfo(context, CL_CONTEXT_REFERENCE_COUNT, sizeof(refCount), &refCount, &sizeContext);
  printf("CONTEXT_REFERENCE_COUNT = %u \n", refCount);

  //creating command queue
  cl_command_queue command = clCreateCommandQueue(context, device, CL_QUEUE_PROFILING_ENABLE, &err);
  // Get the func pointers to the accelerator routines 
  static clCreateAcceleratorINTEL_fn clCreateAcceleratorINTEL = (clCreateAcceleratorINTEL_fn)
    clGetExtensionFunctionAddressForPlatform(platform, "clCreateAcceleratorINTEL");
  static clReleaseAcceleratorINTEL_fn clReleaseAcceleratorINTEL = (clReleaseAcceleratorINTEL_fn)
    clGetExtensionFunctionAddressForPlatform(platform, "clReleaseAcceleratorINTEL");

  // Create the program and the built-in kernel for the motion estimation
  cl_program program = clCreateProgramWithBuiltInKernels(context, 1, &device, "block_motion_estimate_intel", NULL);
  const char *strings = buff;

  cl_program programsour = clCreateProgramWithSource(context, 1, &strings, &result, &err);
  printf("ProgramCreationError = %i\n", err);

  if (INTELWEWANTDEBUG) {
    //before using this, change path into your absolute path to .cl file
    clBuildProgram(programsour, 1, &device, "-g -s C:\\Users\\Savva\\Source\\Repos\\MovementEstimationCL\\MovEstCL\\MovEst_cl.cl", NULL, NULL);
  }
  else
  {
    clBuildProgram(programsour, 1, &device, "", NULL, NULL);
  }

  static char buffer2[20480];
  clGetProgramBuildInfo(programsour, device, CL_PROGRAM_BUILD_LOG, sizeof(buffer2), buffer2, &len_prog);
  printf("ProgramBuildInfo = %s \n", buffer2);


  cl_kernel kernel = clCreateKernel(program, "block_motion_estimate_intel", NULL);
  cl_kernel kern_mr = clCreateKernel(programsour, "mov_reanimation", &err);
  cl_kernel kern_me = clCreateKernel(programsour, "mov_estimation", &err);

  // Create the accelerator for the motion estimation 
  cl_motion_estimation_desc_intel desc = { // VME API configuration knobs
                                           // Num of motion vectors per source pixel block, here a single vector per block
    CL_ME_MB_TYPE_16x16_INTEL,
    CL_ME_SUBPIXEL_MODE_INTEGER_INTEL, // Motion vector precision
                                       // Adjust mode for the residuals, we don't compute them in this tutorial anyway: 
                                       CL_ME_SAD_ADJUST_MODE_NONE_INTEL,
                                       CL_ME_SEARCH_PATH_RADIUS_16_12_INTEL // Search window radius
  };
  cl_accelerator_intel accelerator =
    clCreateAcceleratorINTEL(context,
      CL_ACCELERATOR_TYPE_MOTION_ESTIMATION_INTEL,
      sizeof(cl_motion_estimation_desc_intel), &desc, 0);

  
  // Input images
  cl_image_format format = { CL_R, CL_UNORM_INT8 }; // luminance plane
  cl_mem srcImage = clCreateImage2D(context, CL_MEM_ALLOC_HOST_PTR | CL_MEM_COPY_HOST_PTR , &format,
    w, h, 0, pSrcBuf, &err);
  printf("SrcImage Error = %i\n", err);
  cl_mem refImage = clCreateImage2D(context, CL_MEM_ALLOC_HOST_PTR | CL_MEM_COPY_HOST_PTR , &format,
    w, h, 0, pRefBuf, &err);
  printf("RefImage Error = %i\n", err);

  // Compute number of output motion vectors 
  const int mbSize = 16; // size of the (input) pixel motion block
  size_t widthInMB = (w + mbSize - 1) / mbSize;
  size_t heightInMB = (h + mbSize - 1) / mbSize;
  printf("Qunatity of MB  = %zd X %zd \n", heightInMB, widthInMB);
  // Output buffer for MB motion vectors
  cl_mem outMVBuffer = clCreateBuffer(context, CL_MEM_READ_WRITE, widthInMB * heightInMB * sizeof(cl_short2), 0, NULL);
  cl_mem outImage = clCreateImage2D(context, CL_MEM_WRITE_ONLY, &format, w, h, 0, 0, &err);
  printf("OutImage Error = %i\n", err);

  short *pMVOut = new short[widthInMB*heightInMB*2];
  // Setup params for the built-in kernel 
  err = clSetKernelArg(kernel, 0, sizeof(cl_accelerator_intel), &accelerator);
  printf("Anne INTEL Kern 0 = %i\n", err);
  err = clSetKernelArg(kernel, 1, sizeof(cl_mem), &srcImage);
  printf("Anne INTEL Kern 1 = %i\n", err);
  err = clSetKernelArg(kernel, 2, sizeof(cl_mem), &refImage);
  printf("Anne INTEL Kern 2 = %i\n", err);
  err = clSetKernelArg(kernel, 3, sizeof(cl_mem), NULL); // disable predictor motion vectors
  printf("Anne INTEL Kern 3 = %i\n", err);
  err = clSetKernelArg(kernel, 4, sizeof(cl_mem), &outMVBuffer);
  printf("Anne INTEL Kern 4 = %i\n", err);
  err = clSetKernelArg(kernel, 5, sizeof(cl_mem), NULL); // disable extra motion block info output
  printf("Anne INTEL Kern 5 = %i\n", err);
                                                   // Run the kernel
                                                   // Notice that it *requires* to let runtime determine the local size, and requires 2D ndrange
  const size_t originROI[2] = { 0, 0 };
  const size_t sizeROI[2] = { w, h };
  err = clEnqueueNDRangeKernel(command, kernel, 2, originROI, sizeROI, NULL, 0, 0, 0);
  printf("Kernel Intel err = %i\n", err);
  // Read resulting motion vectors
  //err = clEnqueueReadBuffer(command, outMVBuffer, CL_TRUE, 0, widthInMB * heightInMB * sizeof(cl_short2), pMVOut, 0, 0, 0);
  //printf("Read buff err = %i\n", err);
  //
  //for (int y = 0; y < heightInMB; y++)
  //{
  //  for (int z = 0; z < widthInMB; z+=2)
  //  {
  //    printf("_%i,%i_  ", pMVOut[y*widthInMB + z], pMVOut[y*widthInMB + z + 1]);
  //  }
  //}

  err = clSetKernelArg(kern_mr, 0, sizeof(cl_mem), &srcImage);
  printf("Anne Reanimation Kern 0 = %i\n", err);
  err = clSetKernelArg(kern_mr, 1, sizeof(cl_mem), &refImage);
  printf("Anne Reanimation Kern 1 = %i\n", err);
  err = clSetKernelArg(kern_mr, 2, sizeof(cl_mem), &outMVBuffer);
  printf("Anne Reanimation Kern 2 = %i\n", err);
  err = clSetKernelArg(kern_mr, 3, sizeof(cl_mem), &outImage);
  printf("Anne Reanimation Kern 3 = %i\n", err);
  err = clSetKernelArg(kern_mr, 4, sizeof(cl_int), &widthInMB);
  printf("Anne Reanimation Kern 4 = %i\n", err);
  
  const size_t sizeROI_mr[2] = { widthInMB * mbSize, heightInMB * mbSize };
  const size_t localROI_mr[2] = { mbSize , mbSize };
  //cl_event event[2];

  err = clEnqueueNDRangeKernel(command, kern_mr, 2, originROI, sizeROI_mr, localROI_mr, 0, NULL, NULL);
  printf("Kernel Reanimation err = %i\n", err);
  size_t origin[] = { 0,0,0 }; // Defines the offset in pixels in the image from where to write.
  size_t region[] = { w, h, 1 }; // Size of object to be transferred
  err = clEnqueueReadImage(command, outImage, CL_TRUE, origin, region, 0, 0, pOutBuf, 0, NULL, NULL);
  printf("Read Reanimated Image err = %i\n", err);

  JSAMPARRAY a = new JSAMPROW[h];
  for (int y = 0; y < h; y++)
  {
    a[y] = new JSAMPLE[w];
    for (int z = 0; z < w; z++)
    {
      a[y][z] = (JSAMPLE)pOutBuf[y*w + z];
    }
  }

  PGMwriting(a, w, h, filename0, 'a');


  //покурить освобождение акселератора
  clReleaseAcceleratorINTEL(accelerator);
  clReleaseMemObject(srcImage);
  clReleaseMemObject(refImage);
  clReleaseMemObject(outMVBuffer);
  clReleaseProgram(program);
  clReleaseCommandQueue(command);
  clReleaseContext(context);
  //clReleaseEvent(event);
  clReleaseKernel(kernel);
 
  free(buff);
  /////вот чтобы этого не было, а так же ворнинга о переполнении стека, лучше все перевести в malloc, а не гребанные new
  //for (int y = 0; y < h; y++)
  //{
  //  delete a[y];
  //}
  //delete[] a;
  //delete[] pich;
  //delete[] resu;
  return ;
}

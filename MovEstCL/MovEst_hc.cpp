#define _CRT_SECURE_NO_WARNINGS
#include <CL\cl.h>
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

#define getjsample(value)  ((int) (value))
#define centerjsample	128

#define dctsize		    8
#define dctsize2	    64

static const double aanscalefactor[dctsize] =
{
  1.0, 1.3870398453221475, 1.3065629648763766, 1.1758756024193588,
  1.0, 0.7856949583871022, 0.5411961001461971, 0.2758993792829431,
};

void jpeg_init_fdct_table(const u16 *quantptr, float quant_table[dctsize2])
{
  for (int row = 0, i = 0; row < dctsize; row++)
    for (int col = 0; col < dctsize; i++, col++)
      quant_table[i] = 0.125 / (aanscalefactor[row] * aanscalefactor[col] * quantptr[i]); // 1/64
}

void jpeg_init_idct_table(const u16 *quantptr, float quant_table[dctsize2])
{
  for (int row = 0, i = 0; row < dctsize; row++)
    for (int col = 0; col < dctsize; i++, col++)
      quant_table[i] = quantptr[i] * aanscalefactor[row] * aanscalefactor[col] * 0.125;
}

u8 range_limit(int x)
{
  return x < 0 ? 0 : x > 0xFF ? 0xFF : x;
}

void jpeg_idct_float(const float *inptr, u8 *output_buf, int output_stride)
{
  float tmp0, tmp1, tmp2, tmp3, tmp4, tmp5, tmp6, tmp7;
  float tmp10, tmp11, tmp12, tmp13;
  float z5, z10, z11, z12, z13;
  float * wsptr;
  int ctr;
  float workspace[dctsize2];

  wsptr = workspace;
  for (ctr = dctsize; ctr > 0; ctr--) {
    if (inptr[dctsize * 1] == 0 && inptr[dctsize * 2] == 0 &&
      inptr[dctsize * 3] == 0 && inptr[dctsize * 4] == 0 &&
      inptr[dctsize * 5] == 0 && inptr[dctsize * 6] == 0 &&
      inptr[dctsize * 7] == 0)
    {
      float dcval = inptr[dctsize * 0];

      wsptr[dctsize * 0] = dcval;
      wsptr[dctsize * 1] = dcval;
      wsptr[dctsize * 2] = dcval;
      wsptr[dctsize * 3] = dcval;
      wsptr[dctsize * 4] = dcval;
      wsptr[dctsize * 5] = dcval;
      wsptr[dctsize * 6] = dcval;
      wsptr[dctsize * 7] = dcval;

      inptr++;
      wsptr++;
      continue;
    }

    tmp0 = inptr[dctsize * 0];
    tmp1 = inptr[dctsize * 2];
    tmp2 = inptr[dctsize * 4];
    tmp3 = inptr[dctsize * 6];

    tmp10 = tmp0 + tmp2;
    tmp11 = tmp0 - tmp2;

    tmp13 = tmp1 + tmp3;
    tmp12 = (tmp1 - tmp3) * 1.414213562f - tmp13;

    tmp0 = tmp10 + tmp13;
    tmp3 = tmp10 - tmp13;
    tmp1 = tmp11 + tmp12;
    tmp2 = tmp11 - tmp12;

    tmp4 = inptr[dctsize * 1];
    tmp5 = inptr[dctsize * 3];
    tmp6 = inptr[dctsize * 5];
    tmp7 = inptr[dctsize * 7];

    z13 = tmp6 + tmp5;
    z10 = tmp6 - tmp5;
    z11 = tmp4 + tmp7;
    z12 = tmp4 - tmp7;

    tmp7 = z11 + z13;
    tmp11 = (z11 - z13) * 1.414213562f;

    z5 = (z10 + z12) * 1.847759065f;
    tmp10 = z5 - z12 * 1.082392200f;
    tmp12 = z5 - z10 * 2.613125930f;

    tmp6 = tmp12 - tmp7;
    tmp5 = tmp11 - tmp6;
    tmp4 = tmp10 - tmp5;

    wsptr[dctsize * 0] = tmp0 + tmp7;
    wsptr[dctsize * 7] = tmp0 - tmp7;
    wsptr[dctsize * 1] = tmp1 + tmp6;
    wsptr[dctsize * 6] = tmp1 - tmp6;
    wsptr[dctsize * 2] = tmp2 + tmp5;
    wsptr[dctsize * 5] = tmp2 - tmp5;
    wsptr[dctsize * 3] = tmp3 + tmp4;
    wsptr[dctsize * 4] = tmp3 - tmp4;

    inptr++;
    wsptr++;
  }

  /* pass 2: process rows. */

  wsptr = workspace;
  for (ctr = 0; ctr < dctsize; ctr++) {
    /* prepare range-limit and float->int conversion */
    z5 = wsptr[0] + (centerjsample + 0.5f);
    tmp10 = z5 + wsptr[4];
    tmp11 = z5 - wsptr[4];

    tmp13 = wsptr[2] + wsptr[6];
    tmp12 = (wsptr[2] - wsptr[6]) * 1.414213562f - tmp13;

    tmp0 = tmp10 + tmp13;
    tmp3 = tmp10 - tmp13;
    tmp1 = tmp11 + tmp12;
    tmp2 = tmp11 - tmp12;

    z13 = wsptr[5] + wsptr[3];
    z10 = wsptr[5] - wsptr[3];
    z11 = wsptr[1] + wsptr[7];
    z12 = wsptr[1] - wsptr[7];

    tmp7 = z11 + z13;
    tmp11 = (z11 - z13) * 1.414213562f;

    z5 = (z10 + z12) * 1.847759065f;
    tmp10 = z5 - z12 * 1.082392200f;
    tmp12 = z5 - z10 * 2.613125930f;

    tmp6 = tmp12 - tmp7;
    tmp5 = tmp11 - tmp6;
    tmp4 = tmp10 - tmp5;

    /* final output stage: float->int conversion and range-limit */
    output_buf[0] = range_limit((int)(tmp0 + tmp7));
    output_buf[7] = range_limit((int)(tmp0 - tmp7));
    output_buf[1] = range_limit((int)(tmp1 + tmp6));
    output_buf[6] = range_limit((int)(tmp1 - tmp6));
    output_buf[2] = range_limit((int)(tmp2 + tmp5));
    output_buf[5] = range_limit((int)(tmp2 - tmp5));
    output_buf[3] = range_limit((int)(tmp3 + tmp4));
    output_buf[4] = range_limit((int)(tmp3 - tmp4));

    wsptr += dctsize;
    output_buf += output_stride;
  }
}


void jpeg_fdct_float(float *outptr, const u8 *input_buf, int input_stride)
{
  float tmp0, tmp1, tmp2, tmp3, tmp4, tmp5, tmp6, tmp7;
  float tmp10, tmp11, tmp12, tmp13;
  float z1, z2, z3, z4, z5, z11, z13;
  float *dataptr;
  int ctr;

  /* pass 1: process rows. */

  dataptr = outptr;
  for (ctr = 0; ctr < dctsize; ctr++) {
    tmp0 = (float)(getjsample(input_buf[0]) + getjsample(input_buf[7]));
    tmp7 = (float)(getjsample(input_buf[0]) - getjsample(input_buf[7]));
    tmp1 = (float)(getjsample(input_buf[1]) + getjsample(input_buf[6]));
    tmp6 = (float)(getjsample(input_buf[1]) - getjsample(input_buf[6]));
    tmp2 = (float)(getjsample(input_buf[2]) + getjsample(input_buf[5]));
    tmp5 = (float)(getjsample(input_buf[2]) - getjsample(input_buf[5]));
    tmp3 = (float)(getjsample(input_buf[3]) + getjsample(input_buf[4]));
    tmp4 = (float)(getjsample(input_buf[3]) - getjsample(input_buf[4]));

    tmp10 = tmp0 + tmp3;
    tmp13 = tmp0 - tmp3;
    tmp11 = tmp1 + tmp2;
    tmp12 = tmp1 - tmp2;

    /* apply unsigned->signed conversion. */
    dataptr[0] = tmp10 + tmp11 - 8 * centerjsample;
    dataptr[4] = tmp10 - tmp11;

    z1 = (tmp12 + tmp13) * 0.707106781f;
    dataptr[2] = tmp13 + z1;
    dataptr[6] = tmp13 - z1;

    tmp10 = tmp4 + tmp5;
    tmp11 = tmp5 + tmp6;
    tmp12 = tmp6 + tmp7;

    z5 = (tmp10 - tmp12) * 0.382683433f;
    z2 = 0.541196100f * tmp10 + z5;
    z4 = 1.306562965f * tmp12 + z5;
    z3 = tmp11 * 0.707106781f;

    z11 = tmp7 + z3;
    z13 = tmp7 - z3;

    dataptr[5] = z13 + z2;
    dataptr[3] = z13 - z2;
    dataptr[1] = z11 + z4;
    dataptr[7] = z11 - z4;

    dataptr += dctsize;
    input_buf += input_stride;
  }

  /* pass 2: process columns. */

  dataptr = outptr;
  for (ctr = dctsize - 1; ctr >= 0; ctr--) {
    tmp0 = dataptr[dctsize * 0] + dataptr[dctsize * 7];
    tmp7 = dataptr[dctsize * 0] - dataptr[dctsize * 7];
    tmp1 = dataptr[dctsize * 1] + dataptr[dctsize * 6];
    tmp6 = dataptr[dctsize * 1] - dataptr[dctsize * 6];
    tmp2 = dataptr[dctsize * 2] + dataptr[dctsize * 5];
    tmp5 = dataptr[dctsize * 2] - dataptr[dctsize * 5];
    tmp3 = dataptr[dctsize * 3] + dataptr[dctsize * 4];
    tmp4 = dataptr[dctsize * 3] - dataptr[dctsize * 4];

    tmp10 = tmp0 + tmp3;
    tmp13 = tmp0 - tmp3;
    tmp11 = tmp1 + tmp2;
    tmp12 = tmp1 - tmp2;

    dataptr[dctsize * 0] = tmp10 + tmp11;
    dataptr[dctsize * 4] = tmp10 - tmp11;

    z1 = (tmp12 + tmp13) * 0.707106781f;
    dataptr[dctsize * 2] = tmp13 + z1;
    dataptr[dctsize * 6] = tmp13 - z1;

    tmp10 = tmp4 + tmp5;
    tmp11 = tmp5 + tmp6;
    tmp12 = tmp6 + tmp7;

    z5 = (tmp10 - tmp12) * 0.382683433f;
    z2 = 0.541196100f * tmp10 + z5;
    z4 = 1.306562965f * tmp12 + z5;
    z3 = tmp11 * 0.707106781f;

    z11 = tmp7 + z3;
    z13 = tmp7 - z3;

    dataptr[dctsize * 5] = z13 + z2;
    dataptr[dctsize * 3] = z13 - z2;
    dataptr[dctsize * 1] = z11 + z4;
    dataptr[dctsize * 7] = z11 - z4;

    dataptr++;
  }
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


void PGMwriting(JSAMPARRAY pich, int w, int h, char *filename, int num)
{
  FILE * infile;
  int n = (int)strlen(filename);
  char *newname = new char[n + 2];
  sprintf(newname, "%.*s%i.pgm", n - 4, filename, num);
  infile = fopen(newname, "wb");
  fprintf(infile, "P5\n%i %i\n255\n", w, h);
  for (int i = 0; i < (int)(h); i++)
  {
    fwrite(pich[i], 1, w, infile);
  }
  fclose(infile);
}


int read_JPEG_file(char * filename, u8 *pich, int *w, int *wreal, int *h, int*hreal, float fquant[dctsize2], float iquant[dctsize2])
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
    u16 quant[dctsize2];
    for (int i = 0; i < dctsize2; i++) {
      quant[i] = 1;
    }
    ///uint16 -> u16
    JQUANT_TBL** quants = cinfo.quant_tbl_ptrs;
    for (int i = 0; i < dctsize2; i++) {
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

  PGMwriting(a, cinfo.output_width, cinfo.output_height, filename, 0);
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

  //get platform id
  cl_uint refCount;
  cl_int err, err1;

  //get device's ids
  cl_uint platformIdCount = 0;//
  clGetPlatformIDs(NULL, NULL, &platformIdCount);
  cl_platform_id* platforms = new cl_platform_id[platformIdCount];
  cl_platform_id  useplatfo = 0;
  clGetPlatformIDs(platformIdCount, platforms, &platformIdCount);
  printf("===%d OpenCL platform(s) found: ===\n", platformIdCount);
  for (unsigned int i = 0; i < platformIdCount; ++i)
  {
    static char buffer[10240];
    clGetPlatformInfo(platforms[i], CL_PLATFORM_VENDOR, 10240, buffer, NULL);
    printf("PLATFORM_VENDOR = %s\n", buffer);
    if ((INTELWEWANTSOMEANALYSIS || INTELWEWANTDEBUG) && (buffer[0] == 'I'))
    {
      useplatfo = platforms[i];
    }
    else useplatfo = platforms[0];
  }

  cl_device_id devices[3];
  cl_device_id device;
  cl_uint devicesn = 0;
  int errordevice = 1;
  if (INTELWEWANTDEBUG) {
    errordevice = clGetDeviceIDs(useplatfo, CL_DEVICE_TYPE_CPU, 3, &device, &devicesn);
    printf("DEBUG MODE ON\n");
  }
  else if (errordevice != CL_SUCCESS) {
    clGetDeviceIDs(useplatfo, CL_DEVICE_TYPE_ALL, 3, devices, &devicesn);
    device = devices[0];
    printf("DESTINY MODE ON\n");
  }
  printf("===========<ACTUAL DEVICE>=========\n");
  size_t bufferst[10240];
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
  const char *strings = buff;

  cl_program program = clCreateProgramWithSource(context, 1, &strings, &result, &err1);
  printf("ProgramCreationError = %i\n", err1);

  int err6;

  // if (INTELWEWANTDEBUG){
  //before using this, change path into your absolute path to .cl file
  clBuildProgram(program, 0, NULL, "", NULL, NULL);
  //}
  //else
  //{
  //  clBuildProgram(program, 0, NULL, "", NULL, NULL);  
  //}

  //clBuildProgram(program, 0, NULL, "", NULL, NULL);
  clBuildProgram(program, 1, &device, NULL, NULL, NULL);

  static char buffer2[20480];
  clGetProgramBuildInfo(program, device, CL_PROGRAM_BUILD_LOG, sizeof(buffer2), buffer2, &len_prog);
  printf("ProgramBuildInfo = %s \n", buffer2);

  //////////////////////////////////////////////////////////////////////
  //////FINDING KERNELS/////////////////////////////////////////////////
  ////////////////////////////////////////////////////////////////////// 
  cl_kernel kernel;
  kernel = clCreateKernel(program, "mov_est", &err6);
  //////////////////////////////////////////////////////////////////////
  //////reading file////////////////////////////////////////////////////
  //////////////////////////////////////////////////////////////////////
  char *filename0 = "exp/smallmov0.jpg";
  char *filename1 = "exp/smallmov1.jpg";
  u8 *pich0 = NULL;
  u8 *pich1 = NULL;
  float fquant[dctsize2], iquant[dctsize2];
  int w, h, wreal, hreal;
  read_JPEG_file(filename0, pich0, &w, &wreal, &h, &hreal, fquant, iquant); //to-do - add color component or write new function for that
  pich0 = new u8[w*h];
  read_JPEG_file(filename1, pich1, &w, &wreal, &h, &hreal, fquant, iquant);
  pich1 = new u8[w*h];
  read_JPEG_file(filename0, pich0, &w, &wreal, &h, &hreal, fquant, iquant);
  read_JPEG_file(filename1, pich1, &w, &wreal, &h, &hreal, fquant, iquant);
  //nu heb ik een pich, w, h, fquant, iquant

  unsigned short *resu = new  unsigned short[w*h];

  //cl_mem in_pich, ou_resu, in_fquant, in_iquant;
 
  //in_pich = clCreateBuffer(context, CL_MEM_READ_ONLY, sizeof(u8) * w * h, NULL, NULL);
  //in_fquant = clCreateBuffer(context, CL_MEM_READ_ONLY, sizeof(float) * DCTSIZE2, NULL, NULL);
  //in_iquant = clCreateBuffer(context, CL_MEM_READ_ONLY, sizeof(float) * DCTSIZE2, NULL, NULL);
  //ou_resu = clCreateBuffer(context, CL_MEM_READ_WRITE, sizeof(unsigned short) * w * h, NULL, NULL);
  //if (!in_pich || !in_fquant || !in_iquant || !ou_resu) {
  //  printf("Error of cl_memes!!!!!!\n");
  //  exit(1);
  //}

  //cl_event event;
  //// cl_ulong time_end, time_start;

  //int err_writebuff = 0;
  //int err_readbuff = 0;

  //clSetKernelArg(kernel, 0, sizeof(cl_mem), &in_pich);
  //clSetKernelArg(kernel, 1, sizeof(cl_mem), &ou_resu);
  //clSetKernelArg(kernel, 2, sizeof(cl_mem), &in_fquant);
  //clSetKernelArg(kernel, 3, sizeof(cl_mem), &in_iquant);
  //clSetKernelArg(kernel, 4, sizeof(cl_int), &w);
  //int offset = 0;
  //size_t global[2];
  //size_t local[2] = { DCTSIZE2, 1 };

  //err_writebuff = clEnqueueWriteBuffer(command, in_pich, CL_FALSE, 0, sizeof(u8) * w * h, pich, 0, NULL, NULL);
  //printf("Error WriteBuff inpich = %i\n", err_writebuff);
  //err_writebuff = clEnqueueWriteBuffer(command, in_fquant, CL_FALSE, 0, sizeof(float) * dctsize2, fquant, 0, NULL, NULL);
  //printf("Error WriteBuff infquant = %i\n", err_writebuff);
  //err_writebuff = clEnqueueWriteBuffer(command, in_iquant, CL_FALSE, 0, sizeof(float) * dctsize2, iquant, 0, NULL, NULL);
  //printf("Error WriteBuff iniquant = %i\n", err_writebuff);
  //err_writebuff = clEnqueueWriteBuffer(command, ou_resu, CL_TRUE, 0, sizeof(unsigned short) * w * h, resu, 0, NULL, NULL);
  //printf("Error WriteBuff ouresu = %i\n", err_writebuff);


  ////devnull
  //clSetKernelArg(kerneldevnull, 0, sizeof(cl_mem), &ou_resu);
  //global[0] = w*h / 2;
  //global[1] = 1;
  //err = clEnqueueNDRangeKernel(command, kerneldevnull, 1, 0, global, NULL, NULL, NULL, &event);
  //printf("Error NDRangeKernel division = %i\n", err);
  //clFinish(command);
  ///////////////////////////////////////////////////////////////////////////
  ///////MOST WANTED/////////////////////////////////////////////////////////
  ///////////////////////////////////////////////////////////////////////////
  //////offset 0 kernel 1
  //offset = 0;
  //clSetKernelArg(kernel, 5, sizeof(cl_int), &offset);
  //global[0] = w / 16 * DCTSIZE2;
  //global[1] = h / 16;
  //err = clEnqueueNDRangeKernel(command, kernel, 2, 0, global, local, NULL, NULL, &event);
  //printf("Error NDRangeKernel with off %i = %i\n", offset, err);
  //clFinish(command);
  //////offset 8 kernel 2
  //offset = 8;
  //clSetKernelArg(kernel, 5, sizeof(cl_int), &offset);
  //global[0] = (w - 8) / 16 * DCTSIZE2;
  //err = clEnqueueNDRangeKernel(command, kernel, 2, 0, global, local, NULL, NULL, &event);
  //printf("Error NDRangeKernel  with off %i = %i\n", offset, err);
  //clFinish(command);
  //////offset 8*w kernel 3
  //offset = 8 * w;
  //clSetKernelArg(kernel, 5, sizeof(cl_int), &offset);
  //global[0] = w / 16 * DCTSIZE2;
  //global[1] = (h - 8) / 16;
  //err = clEnqueueNDRangeKernel(command, kernel, 2, 0, global, local, NULL, NULL, &event);
  //printf("Error NDRangeKernel  with off %i = %i\n", offset, err);
  //clFinish(command);
  //////offset 8*w + 8 kernel 4
  //offset = 8 * w + 8;
  //clSetKernelArg(kernel, 5, sizeof(cl_int), &offset);
  //global[0] = (w - 8) / 16 * DCTSIZE2;
  //global[1] = (h - 8) / 16;
  //err = clEnqueueNDRangeKernel(command, kernel, 2, 0, global, local, NULL, NULL, &event);
  //printf("Error NDRangeKernel  with off %i = %i\n", offset, err);
  //clFinish(command);
  ///////////////////////////////////////////////////////////////////////////
  ///////Kernel's EDGE///////////////////////////////////////////////////////
  ///////////////////////////////////////////////////////////////////////////

  ////vertical kernels
  //offset = w - 8;
  //clSetKernelArg(kernelvert, 0, sizeof(cl_mem), &in_pich);
  //clSetKernelArg(kernelvert, 1, sizeof(cl_mem), &ou_resu);
  //clSetKernelArg(kernelvert, 2, sizeof(cl_mem), &in_fquant);
  //clSetKernelArg(kernelvert, 3, sizeof(cl_mem), &in_iquant);
  //clSetKernelArg(kernelvert, 4, sizeof(cl_int), &w);
  //clSetKernelArg(kernelvert, 5, sizeof(cl_int), &h);
  //clSetKernelArg(kernelvert, 6, sizeof(cl_int), &offset);
  //global[0] = 1;
  //global[1] = (h / 16) * 8;
  //local[0] = 1;
  //local[1] = 8;
  //err = clEnqueueNDRangeKernel(command, kernelvert, 2, 0, global, local, NULL, NULL, &event);
  //clFinish(command);
  //printf("Error NDRangeKernel vert with off %i = %i\n", offset, err);
  //offset = w * 8 + w - 8;
  //clSetKernelArg(kernelvert, 6, sizeof(cl_int), &offset);
  //global[0] = 1;
  //global[1] = (h - 8) / 16 * 8;
  //local[0] = 1;
  //local[1] = 8;
  //err = clEnqueueNDRangeKernel(command, kernelvert, 2, 0, global, local, NULL, NULL, &event);
  //clFinish(command);
  //printf("Error NDRangeKernel vert with off %i = %i\n", offset, err);
  //////horizontal kernels
  //offset = (h - 8)*w;
  //clSetKernelArg(kernelhori, 0, sizeof(cl_mem), &in_pich);
  //clSetKernelArg(kernelhori, 1, sizeof(cl_mem), &ou_resu);
  //clSetKernelArg(kernelhori, 2, sizeof(cl_mem), &in_fquant);
  //clSetKernelArg(kernelhori, 3, sizeof(cl_mem), &in_iquant);
  //clSetKernelArg(kernelhori, 4, sizeof(cl_int), &w);
  //clSetKernelArg(kernelhori, 5, sizeof(cl_int), &h);
  //clSetKernelArg(kernelhori, 6, sizeof(cl_int), &offset);
  //global[0] = (w / 16) * 8;
  //global[1] = 1;
  //local[0] = 8;
  //local[1] = 1;
  //err = clEnqueueNDRangeKernel(command, kernelhori, 2, 0, global, local, NULL, NULL, &event);
  //clFinish(command);
  //printf("Error NDRangeKernel hori with off %i = %i\n", offset, err);
  //offset = (h - 8)*w + 8;
  //clSetKernelArg(kernelhori, 6, sizeof(cl_int), &offset);
  //global[0] = (w - 8) / 16 * 8;
  //global[1] = 1;
  //local[0] = 8;
  //local[1] = 1;
  //err = clEnqueueNDRangeKernel(command, kernelhori, 2, 0, global, local, NULL, NULL, &event);
  //clFinish(command);
  //printf("Error NDRangeKernel hori with off %i = %i\n", offset, err);
  //////kernel op de hoek
  //offset = (h - 8)*w + w - 8;
  //clSetKernelArg(kernelnook, 0, sizeof(cl_mem), &in_pich);
  //clSetKernelArg(kernelnook, 1, sizeof(cl_mem), &ou_resu);
  //clSetKernelArg(kernelnook, 2, sizeof(cl_mem), &in_fquant);
  //clSetKernelArg(kernelnook, 3, sizeof(cl_mem), &in_iquant);
  //clSetKernelArg(kernelnook, 4, sizeof(cl_int), &w);
  //clSetKernelArg(kernelnook, 5, sizeof(cl_int), &h);
  //clSetKernelArg(kernelnook, 6, sizeof(cl_int), &offset);
  ////если распарсить дктидкт
  //global[0] = 8;
  //global[1] = 1;
  //local[0] = 8;
  //local[1] = 1;
  ////подумоть, а нужна ли так локальная группа
  //err = clEnqueueNDRangeKernel(command, kernelnook, 2, 0, global, local, NULL, NULL, &event);
  //printf("Error NDRangeKernel op de hoek met off %i = %i\n", offset, err);
  //clFinish(command);
  ///////////////////////////////////////////////////////////////////////////
  ///////Pichoy Division/////////////////////////////////////////////////////
  ///////////////////////////////////////////////////////////////////////////
  //clSetKernelArg(kerneldivi, 0, sizeof(cl_mem), &ou_resu);
  //clSetKernelArg(kerneldivi, 1, sizeof(cl_int), &w);
  //clSetKernelArg(kerneldivi, 2, sizeof(cl_int), &h);
  //global[0] = w;
  //global[1] = h;
  //err = clEnqueueNDRangeKernel(command, kerneldivi, 2, 0, global, NULL, NULL, NULL, &event);
  //printf("Error NDRangeKernel division = %i\n", err);
  //clFinish(command);
  ///////////////////////////////////////////////////////////////////////////
  ///////READING BUFFER//////////////////////////////////////////////////////
  ///////////////////////////////////////////////////////////////////////////
  //err = clEnqueueReadBuffer(command, ou_resu, CL_TRUE, 0, sizeof(unsigned short) * w * h, resu, 0, NULL, NULL);
  //printf("Error ReadBuff ouresu = %i\n", err);
  ////clGetEventProfilingInfo(event, CL_PROFILING_COMMAND_START, sizeof(time_start), &time_start, NULL);
  ////clGetEventProfilingInfo(event, CL_PROFILING_COMMAND_END, sizeof(time_end), &time_end, NULL);
  ////printf("TIME = %llu\n", (time_end - time_start));

  //JSAMPARRAY a = new JSAMPROW[h];
  //for (int y = 0; y < h; y++)
  //{
  //  a[y] = new JSAMPLE[w];
  //  for (int z = 0; z < w; z++)
  //  {
  //    a[y][z] = (JSAMPLE)resu[y*w + z];
  //  }
  //}

  //PGMwriting(a, w, h, filename, 5);

  //clReleaseMemObject(in_pich);
  //clReleaseMemObject(in_fquant);
  //clReleaseMemObject(in_iquant);
  //clReleaseMemObject(ou_resu);
  //clReleaseProgram(program);
  //clReleaseCommandQueue(command);
  //clReleaseContext(context);
  //clReleaseEvent(event);
  //clReleaseKernel(kernel);
  //clReleaseKernel(kernelvert);
  //clReleaseKernel(kernelhori);
  //clReleaseKernel(kernelnook);
  //clReleaseKernel(kerneldivi);
  //clReleaseKernel(kerneldevnull);

  //free(buff);
  /////вот чтобы этого не было, а так же ворнинга о переполнении стека, лучше все перевести в malloc, а не гребанные new
  //for (int y = 0; y < h; y++)
  //{
  //  delete a[y];
  //}
  //delete[] a;
  //delete[] pich;
  //delete[] resu;
  return;
}

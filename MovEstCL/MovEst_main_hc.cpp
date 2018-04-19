#define _CRT_SECURE_NO_WARNINGS

//CL
#include <CL\cl.h>
#if __has_include(<CL\cl_ext.h>)
# include <CL\cl_ext.h>
#endif

//OpenMP
#include <omp.h>

//io und strings
#include <string>
#include <fstream>
#include <sstream>
#include <iostream>
#include <cstdio>
//#include <cstdlib>
using namespace std;
//timing
#include <chrono>

//console
#include <conio.h>
#include <Windows.h>



typedef unsigned char u8;
typedef short i16;
typedef unsigned short u16;
#define MotBlo 16
#define LOCUS 16
#define DOZEN 12
#define LOCUSII LOCUS*2
#define DOZENII DOZEN*2



int PGMReader(char* filename, u8 *pixel_value, int &num_of_rows, int &num_of_cols)
{
  bool flag = false;
  int row = 0, col = 0;
  stringstream ss;
  
  ifstream infile(filename, ios::binary);
  if (pixel_value != NULL) {
    flag = true;
  }
  if (!flag) {
    cout << "<Reading PGM file>" << endl;
  }
  string inputLine = "";
  getline(infile, inputLine);      // read the first line : P5
  if (inputLine.compare("P5") != 0) cerr << "Version error" << endl;
  if (!flag) {
    cout << "Version : " << inputLine << endl;
  }
  ss << infile.rdbuf();   //read the third line : width and height
  ss >> num_of_cols >> num_of_rows;
 
  if (!flag) {
    cout << num_of_cols << " columns and " << num_of_rows << " rows" << endl;
  }
  int max_val;  //maximum intensity value : 255
  ss >> max_val;
  ss.ignore();
  if (!flag) {
    cout << max_val << endl;
  }
  if (flag) {
    unsigned char pixel;
    //pixel_value = new u8[w*h];
    for (row = 0; row < num_of_rows; row++) {    //record the pixel values
      for (col = 0; col < num_of_cols; col++) {
        ss.read((char*)&pixel, 1);
        pixel_value[row*num_of_cols + col] = pixel;
      }
    }
    cout << "</Reading PGM file>" << endl;
  }
  infile.close();
  return 0;
}
int PGMWriter(char* filename, u8 *pixel_value, int num_of_rows, int num_of_cols, char num)
{
  FILE * outfile;
  int n = (int)strlen(filename);
  char *newname = new char[n + 1];
  sprintf(newname, "%.*s%c.pgm", n - 4, filename, num);
  outfile = fopen(newname, "wb");
  fprintf(outfile, "P5\n%i %i\n255\n", num_of_cols, num_of_rows);
  #pragma omp for ordered schedule(guided)
  for (int i = 0; i < num_of_rows; i++)
  {
    #pragma omp ordered
    fwrite(&pixel_value[i*num_of_cols], 1, num_of_cols, outfile);
  }
  fclose(outfile);
  return 0;
}
u8* PGMExtend8(u8* datum, int &height, int &width)
{
  int dw8 = width + 7 & ~7;
  dw8 = dw8 - width;
  int dh8 = height + 7 & ~7;
  dh8 = dh8 - height;
  u8* new_data = new u8[(width + dw8)*(height + dh8)];
  memset(new_data, 0, (width + dw8)*(height + dh8));
  for (int i = 0; i < height; i++)
  {
    for (int j = 0; j < width; j++)
    {
      *(new_data + i*(width + dw8) + j) = *(datum + i * width + j);
    }
  }
  width += dw8;
  height += dh8;
#pragma omp parallel for schedule(guided)
  for (int j = 0; j < width; j++)
  {
    for (int i = 0; i <= dh8; i++)
    {
      *(new_data + (height - 1 - dh8 + i) * width + j) = *(new_data + (height - 1 - dh8 - i) * width + j);
    }
  }
#pragma omp parallel for schedule(guided)
  for (int i = 0; i < height; i++)
  {
    for (int j = 0; j <= dw8; j++)
    {
      *(new_data + i * width + j + width- 1 - dw8) = *(new_data + i * width + width- 1 - dw8 - j);
    }
  }
  return new_data;

}
u8* PGMExtend(u8* datum, int &height, int &width, int areay, int areax)
{
  int dw8 = width + 7 & ~7;
  dw8 = dw8 - width;
  int dh8 = height + 7 & ~7;
  dh8 = dh8 - height;
  u8* new_data = new u8[(width + dw8 + areax)*(height + dh8 + areay)];
  memset(new_data, 0, (width + dw8 + areax)*(height + dh8 + areay));
  //black border
  #pragma omp parallel for schedule(guided)
  for (int i = 0; i < height; i++)
  {
    for (int j = 0; j < width; j++)
    {
      *(new_data + (i + areay/2)*(width+dw8 + areax) + (j + areax/2)) = *(datum + i * width + j);
    }
  }
  width += dw8 + areax;
  height += dh8 + areay;
  #pragma omp parallel for schedule(guided)
  for (int j = 0; j < width; j++)
  {
    for (int i = 0; i < areay/2+dh8; i++)
    {
      if (i < areay / 2) {
        *(new_data + i * width + j) = *(new_data + (areay - i) * width + j);
      }
      *(new_data + (height- areay / 2 -1 - dh8 +i) * width + j) = *(new_data + (height - areay / 2 -1 - dh8 - i) * width + j);
    }
    *(new_data + (height - 1) * width + j) = *(new_data + (height - 1- areay) * width + j);
  }
  #pragma omp parallel for schedule(guided)
  for (int i = 0; i < height; i++)
  {
    for (int j = 0; j < areax/2+dw8; j++)
    {
      if (j < areax / 2) {
        *(new_data + i * width + j) = *(new_data + i * width + (areax  - j));
      }
      *(new_data + i * width + j+ width- areax / 2 -1 - dw8) = *(new_data + i * width + width - areax / 2 -1 -dw8 -j);
    }
    *(new_data + i * width + width-1) = *(new_data + i * width + (width - 1-areax));
  }
  return new_data;
}

int getDeviceInfoAllemaal(char typdev, cl_device_id &dev,cl_platform_id &pla, bool &extflag) 
{
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
      if ((typdev == 'I') && (buffer[0] == 'I'))
      {
        platform = platforms[i];
      }
      else platform = platforms[0];
    }
    pla = platform;


    cl_device_id devices[3];
    cl_device_id device;
    cl_uint devicesn = 0;

    err = clGetDeviceIDs(platform, CL_DEVICE_TYPE_ALL, 3, devices, &devicesn);
    device = devices[0];
    dev = device;
    printf("DESTINY MODE ON\n");
    printf("===========<ACTUAL DEVICE>=========\n");
    static size_t bufferst[10240];
    cl_uint buf_uint;
    cl_ulong buf_ulong;
    static char buffer[10240];
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
    clGetDeviceInfo(device, CL_DEVICE_EXTENSIONS, sizeof(buffer), buffer, NULL);
    printf(" EXTENSIONS = %s\n", buffer);
    printf("==========</ACTUAL DEVICE>=========\n");

    string bufex(buffer);
    string extme ("motion_estimation");
    //string extme("pumpum");
    int found = bufex.find(extme);
    if (found >= 0)
    {
      extflag = true;
    }
    return err;
}



void MBCut(int width, int i, int j, u8*data, u8*res, int areay, int areax)
{
#pragma omp parallel for schedule(guided)
  for (int l = 0; l < areay; l++)
  {
    for (int k = 0; k < areax; k++)
    {
      res[l * areax + k] = data[(i + l) * width + j + k];
    }
  }
}
//OpenMP: return short MBvector - coordinations x y
void MBSearch(u8 * tmp1, u8 *tmp2, i16 &minI, i16 &minJ)
{
  int min = INT_MAX;
  #pragma omp parallel for schedule(guided)
  for (int i = 0; i <= LOCUS + MotBlo /2; i++)
  {
    for (int j = 0; j <= LOCUS + MotBlo / 2; j++)
    {
      int TMPmin = 0;
      for (int k = 0; k < MotBlo; k++)
      {
        for (int z = 0; z < MotBlo; z++)
        {
          TMPmin += pow((tmp2[(i + k)*LOCUS*3 + j + z] - tmp1[k*MotBlo + z]), 2);
        }
      }
      TMPmin = sqrt(TMPmin);
      #pragma omp critical
      {
        if (TMPmin < min)
        {
          min = TMPmin;
          minI = i;
          minJ = j;
        }
      }
    }
  }
}
//OpenMP: return short MBvector - full
i16* OMPMotBloEstus(u8* pSrcBuf, u8* pRefBuf, int ws, int hs, int wr, int hr)
{
  printf("<OpenMP>\n");
  const int mbSize = MotBlo; // size of the (input) pixel motion block
  size_t widthInMB = (ws + mbSize - 1) / mbSize;
  size_t heightInMB = (hs + mbSize - 1) / mbSize;
  printf("widthInMB %zu X heightInMB %zu \n", widthInMB, heightInMB);
  i16* pMBv = new i16[widthInMB*heightInMB * 2];

  using milli = std::chrono::milliseconds;
  auto start = std::chrono::high_resolution_clock::now();
#pragma omp parallel for schedule(dynamic)
  for (int i = 0; i < hs; i += MotBlo)
  {
    for (int j = 0; j < ws; j += MotBlo)
    {
      u8 *totMB = new u8[MotBlo*MotBlo];
      u8 *totLO = new u8[(LOCUS * 3)*(LOCUS * 3)];
      memset(totMB, 0, MotBlo*MotBlo);
      memset(totLO, 0, (LOCUS * 3)*(LOCUS * 3));
      MBCut(ws, i, j, pSrcBuf, totMB, MotBlo, MotBlo);
      MBCut(wr, i, j, pRefBuf, totLO, LOCUS * 3, LOCUS * 3);
      int ui = i / mbSize;
      int uj = j / mbSize;
      MBSearch(totMB, totLO, pMBv[ui * widthInMB + uj], pMBv[ui * widthInMB + uj + widthInMB * heightInMB]);
    }
  }
  auto finish = std::chrono::high_resolution_clock::now();
  std::cout << "OMP main cycle took "
    << std::chrono::duration_cast<milli>(finish - start).count()
    << " milliseconds\n";
  //for (int y = 0; y < heightInMB; y+=2)
  //{
  //    for (int z = 0; z < widthInMB; z+=2)
  //    {
  //      printf("_%i,%i_  ", pMBv[y*widthInMB + z], pMBv[y*widthInMB + z + widthInMB * heightInMB]);
  //    }
  //    printf("\n");
  // }
  printf("</OpenMP>\n");
  //OpenMP Reanimation : void

  return pMBv;
}
void OMPReanimation(char *filenamesrc, u8* pRefBuf, int wr, int hr, int wtrue, int htrue, i16 *pMBv) {
  const int mbSize = MotBlo; // size of the (input) pixel motion block
  size_t widthInMB = (wr - LOCUSII + mbSize - 1) / mbSize;
  size_t heightInMB = (hr - LOCUSII + mbSize - 1) / mbSize;
  u8 *res = new u8[(wr - LOCUSII)*(hr - LOCUSII)];
  printf("<OpenMP Reanimation>\n");

  using milli = std::chrono::milliseconds;
  auto start = std::chrono::high_resolution_clock::now();
#pragma omp parallel for schedule(guided)
  for (int i = 0; i < hr - LOCUSII; i += MotBlo)
  {
    for (int j = 0; j < wr - LOCUSII; j += MotBlo)
    {
      u8 *totMB = new u8[MotBlo*MotBlo];
      int ui = i / mbSize;
      int uj = j / mbSize;
      MBCut(wr, i + pMBv[ui * widthInMB + uj], j + pMBv[ui* widthInMB + uj + widthInMB * heightInMB], pRefBuf, totMB, MotBlo, MotBlo);
      for (int l = 0; l < MotBlo; l++)
      {
        for (int k = 0; k < MotBlo; k++)
        {
          res[(i + l)*(wr - LOCUSII) + j + k] = totMB[l * MotBlo + k];
        }
      }
    }
  }
  auto finish = std::chrono::high_resolution_clock::now();
  std::cout << "OMP Reanimation took "
    << std::chrono::duration_cast<milli>(finish - start).count()
    << " milliseconds\n";


  u8* trueres = new u8[wtrue*htrue];
  MBCut(wr - LOCUSII, 0, 0, res, trueres, htrue, wtrue);
  PGMWriter(filenamesrc, trueres, htrue, wtrue, 'M');
  printf("</OpenMP Reanimation>\n");
}
void OCLReanimation(char *filenamesrc, cl_context context, cl_command_queue command, cl_program programsour, cl_mem outMVBuffer, cl_mem srcImage, cl_mem refImage, int w, int h, int wtrue, int htrue, bool intelflag);


//OpenCL external MotEst: return short MBvector
i16* OCLIntelMotBloEstus(char* filenamesrc, u8* pSrcBuf, u8* pRefBuf, int w, int h, cl_device_id device, cl_platform_id platform)
{

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
    size_t sizeContext, len_prog;
    cl_int err;
    //creating context
    cl_uint refCount;
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
    clBuildProgram(programsour, 1, &device, "", NULL, NULL);
    static char buffer2[20480];
    clGetProgramBuildInfo(programsour, device, CL_PROGRAM_BUILD_LOG, sizeof(buffer2), buffer2, &len_prog);
    printf("ProgramBuildInfo = %s \n", buffer2);
    cl_kernel kernel = clCreateKernel(program, "block_motion_estimate_intel", NULL);
  
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
    const int mbSize = MotBlo; // size of the (input) pixel motion block
    size_t widthInMB = (w + mbSize - 1) / mbSize;
    size_t heightInMB = (h + mbSize - 1) / mbSize;
    printf("Quantity of MB  = %zd X %zd \n", heightInMB, widthInMB);
    // Output buffer for MB motion vectors
    cl_mem outMVBuffer = clCreateBuffer(context, CL_MEM_READ_WRITE, widthInMB * heightInMB * sizeof(cl_short2), 0, NULL);
    printf("OutImage Error = %i\n", err);
  
    short *pMVOut = new short[widthInMB*heightInMB*2];
    // Setup params for the built-in kernel 
    err = clSetKernelArg(kernel, 0, sizeof(cl_accelerator_intel), &accelerator);
    printf("Extensial Estus KernArg 0 = %i\n", err);
    err = clSetKernelArg(kernel, 1, sizeof(cl_mem), &srcImage);
    printf("Extensial Estus KernArg 1 = %i\n", err);
    err = clSetKernelArg(kernel, 2, sizeof(cl_mem), &refImage);
    printf("Extensial Estus KernArg 2 = %i\n", err);
    err = clSetKernelArg(kernel, 3, sizeof(cl_mem), NULL); // disable predictor motion vectors
    printf("Extensial Estus KernArg 3 = %i\n", err);
    err = clSetKernelArg(kernel, 4, sizeof(cl_mem), &outMVBuffer);
    printf("Extensial Estus KernArg 4 = %i\n", err);
    err = clSetKernelArg(kernel, 5, sizeof(cl_mem), NULL); // disable extra motion block info output
    printf("Extensial Estus KernArg 5 = %i\n", err);
                                                     // Run the kernel
                                                     // Notice that it *requires* to let runtime determine the local size, and requires 2D ndrange
    const size_t originROI[2] = { 0, 0 };
    const size_t sizeROI[2] = { w, h };
    err = clEnqueueNDRangeKernel(command, kernel, 2, originROI, sizeROI, NULL, 0, 0, 0);
    printf("Extensial Estus KernelEnqueue err = %i\n", err);
    // Read resulting motion vectors
    err = clEnqueueReadBuffer(command, outMVBuffer, CL_TRUE, 0, widthInMB * heightInMB * sizeof(cl_short2), pMVOut, 0, 0, 0);
    printf("Read buff err = %i\n", err);
    OCLReanimation(filenamesrc, context, command, programsour, outMVBuffer, srcImage, refImage, w, h, true);
    //for (int y = 0; y < heightInMB; y++)
    //{
    //    for (int z = 0; z < widthInMB*2; z+=2)
    //    {
    //      printf("_%i,%i_  ", pMVOut[y*widthInMB + z]/4, pMVOut[y*widthInMB + z + 1]/4);
    //    }
    //    printf("\n");
    // }
    clReleaseAcceleratorINTEL(accelerator);
    clReleaseMemObject(srcImage);
    clReleaseMemObject(refImage);
    clReleaseMemObject(outMVBuffer);
    clReleaseProgram(program);
    clReleaseProgram(programsour);
    clReleaseCommandQueue(command);
    clReleaseContext(context);
    clReleaseKernel(kernel);
    //OpenCL Reanimation : void
    return pMVOut;
    free(buff);

}

//OpenCL MotEst MAD: return short MBvector
i16* OCLMyMADMotBloEstus(char* filenamesrc ,u8* pSrcBuf, u8* pRefBuf, int w, int h, cl_device_id device, cl_platform_id platform)
{
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


  //sizes van allemaal
  size_t ws = w + 7 & ~7;
  size_t hs = h + 7 & ~7;

  size_t wr = ws + LOCUSII;
  size_t hr = hs + DOZENII;
  u8 *pOutBuf = new u8[ws*hs];
  cl_uint refCount;
  cl_int err;
  size_t sizeContext, len_prog;
  cl_context context;
  context = clCreateContext(NULL, 1, &device, NULL, NULL, NULL);
  if (!context) 
  {
    printf("%s\n", "ContextError");
  }
  clGetContextInfo(context, CL_CONTEXT_REFERENCE_COUNT, sizeof(refCount), &refCount, &sizeContext);
  printf("CONTEXT_REFERENCE_COUNT = %u \n", refCount);
  const char *strings = buff;
  //creating command queue
  cl_command_queue command = clCreateCommandQueue(context, device, CL_QUEUE_PROFILING_ENABLE, &err);
  cl_program programsour = clCreateProgramWithSource(context, 1, &strings, &result, &err);
  printf("ProgramCreationError = %i\n", err);
  //if (INTELWEWANTDEBUG) {
  //  //before using this, change path into your absolute path to .cl file
  //  clBuildProgram(programsour, 1, &device, "-g -s C:\\Users\\Savva\\Source\\Repos\\MovementEstimationCL\\MovEstCL\\MovEst_cl.cl", NULL, NULL);
  //}
  //else
  //{
  clBuildProgram(programsour, 1, &device, "", NULL, NULL);
  //}
  static char buffer2[20480];
  clGetProgramBuildInfo(programsour, device, CL_PROGRAM_BUILD_LOG, sizeof(buffer2), buffer2, &len_prog);
  printf("ProgramBuildInfo = %s \n", buffer2);
  
  cl_kernel kern_meMAD = clCreateKernel(programsour, "mov_estimationMAD", &err);
  // Compute number of output motion vectors 
  const int mbSize = MotBlo; // size of the (input) pixel motion block
  size_t widthInMB = (ws + mbSize - 1) / mbSize;
  size_t heightInMB = (hs + mbSize - 1) / mbSize;
  printf("Quantity of MB  = %zd X %zd \n", heightInMB, widthInMB);
  // Output buffer for MB motion vectors
  cl_mem outMVBuffer = clCreateBuffer(context, CL_MEM_READ_WRITE, widthInMB * heightInMB * sizeof(cl_short2), 0, NULL);
  printf("OutImage Error = %i\n", err);
  short *pMVOut = new short[widthInMB*heightInMB * 2];
  cl_mem srcImage = clCreateBuffer(context, CL_MEM_READ_ONLY, ws * hs * sizeof(cl_uchar), pSrcBuf, NULL);
  cl_mem refImage = clCreateBuffer(context, CL_MEM_READ_ONLY, wr * hr * sizeof(cl_uchar), pRefBuf, NULL);ь  ь ь ь 

  err = clSetKernelArg(kern_meMAD, 0, sizeof(cl_mem), &srcImage);
  printf("MAD Nvidia Estus KernArg 0 = %i\n", err);
  err = clSetKernelArg(kern_meMAD, 1, sizeof(cl_mem), &refImage);
  printf("MAD Nvidia Estus KernArg 1 = %i\n", err);

  return 0;
}
//OpenCl MotEst SAD: return short MBvector
i16* OCLMySADMotBloEstus(char* filenamesrc, u8* pSrcBuf, u8* pRefBuf, int w, int h, cl_device_id device, cl_platform_id platform)
{
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
  u8 *pOutBuf = new u8[w*h];
  return 0;
}
//OpenCL Reanimation : void
void OCLReanimation(char *filenamesrc, cl_context context, cl_command_queue command, cl_program programsour, cl_mem outMVBuffer, cl_mem srcImage, cl_mem refImage, int wtrue, int htrue, bool intelflag)
{
  cl_int err;

  cl_kernel kern_mr_i = clCreateKernel(programsour, "mov_reanimation_intl", &err);
  cl_kernel kern_mr_n = clCreateKernel(programsour, "mov_reanimation_mijn", &err);
  size_t w = wtrue + 7 & ~7;
  size_t h = htrue + 7 & ~7;
  u8 *pOutBuf = new u8[w*h];
  u8* trueres = new u8[wtrue*htrue];
  if (intelflag) {
    cl_image_format format = { CL_R, CL_UNORM_INT8 };
    cl_mem outImage = clCreateImage2D(context, CL_MEM_WRITE_ONLY, &format, wtrue, htrue, 0, 0, &err);
    printf("OutImage Error = %i\n", err);
    const int mbSize = 16; // size of the (input) pixel motion block
    size_t widthInMB = (wtrue + mbSize - 1) / mbSize;
    size_t heightInMB = (htrue + mbSize - 1) / mbSize;
    printf("Qunatity of MB  = %zd X %zd \n", heightInMB, widthInMB);
    err = clSetKernelArg(kern_mr_i, 0, sizeof(cl_mem), &srcImage);
    printf("Anne Reanimation Kern 0 = %i\n", err);
    err = clSetKernelArg(kern_mr_i, 1, sizeof(cl_mem), &refImage);
    printf("Anne Reanimation Kern 1 = %i\n", err);
    err = clSetKernelArg(kern_mr_i, 2, sizeof(cl_mem), &outMVBuffer);
    printf("Anne Reanimation Kern 2 = %i\n", err);
    err = clSetKernelArg(kern_mr_i, 3, sizeof(cl_mem), &outImage);
    printf("Anne Reanimation Kern 3 = %i\n", err);
    err = clSetKernelArg(kern_mr_i, 4, sizeof(cl_int), &widthInMB);
    printf("Anne Reanimation Kern 4 = %i\n", err);

    const size_t originROI[2] = { 0, 0 };
    const size_t sizeROI_mr[2] = { widthInMB * mbSize, heightInMB * mbSize };
    const size_t localROI_mr[2] = { mbSize , mbSize };
    err = clEnqueueNDRangeKernel(command, kern_mr_i, 2, originROI, sizeROI_mr, localROI_mr, 0, NULL, NULL);
    printf("Kernel Reanimation err = %i\n", err);
    size_t origin[] = { 0,0,0 }; // Defines the offset in pixels in the image from where to write.
    size_t region[] = { wtrue, htrue, 1 }; // Size of object to be transferred
    err = clEnqueueReadImage(command, outImage, CL_TRUE, origin, region, 0, 0, pOutBuf, 0, NULL, NULL);
    printf("Read Reanimated Image err = %i\n", err);


    //MBCut(w, 0, 0, pOutBuf, trueres, htrue, wtrue);
    PGMWriter(filenamesrc, pOutBuf, htrue, wtrue, 'I');
    clReleaseMemObject(outImage);
  }
  else {

  }
  clReleaseKernel(kern_mr_n);
  clReleaseKernel(kern_mr_i);
  delete[] trueres;
  delete[] pOutBuf;
}

//consolas
int main(int argc, char* argv[])
{
  //Aan het lezen foto's
  char* filename0 = "exp/hercule0.pgm";
  char* filename1 = "exp/hercule1.pgm";
  //char* filename0 = "exp/002182b.pgm";
  //char* filename1 = "exp/002184b.pgm";
  int wr, hr;
  int ws, hs;
  u8 *pSrcBuf = NULL;
  u8 *pRefBuf = NULL;
  PGMReader(filename0, pSrcBuf, hs, ws);
  pSrcBuf = new u8[ws*hs];
  PGMReader(filename0, pSrcBuf, hs, ws);
  PGMReader(filename1, pRefBuf, hr, wr);
  pRefBuf = new u8[wr*hr];
  PGMReader(filename1, pRefBuf, hr, wr);
  int hreal = hr;
  int wreal = wr;

  u8* pSrcBufn = PGMExtend8(pSrcBuf, hs, ws);
  u8* pRefBufn = PGMExtend(pRefBuf, hr, wr, LOCUSII, LOCUSII);


  //cl_device_id dev;
  //cl_platform_id pla;
  //bool extvrage = false ;
  //getDeviceInfoAllemaal('I', dev, pla, extvrage);
  //printf("Intel Motion Estimation Extension = ");
  //printf("%s\n", extvrage ? "true" : "false");
  //i16 *mbv = OCLIntelMotBloEstus(filename0, pSrcBuf, pRefBuf, wreal, hreal, dev, pla);
  ////i16 *oclmadmbv = OCLMyMADMotBloEstus(filename0, pSrcBufn, pRefBufn, wreal, hreal, dev, pla);
  ////i16 *oclsadmbv = OCLMySADMotBloEstus(filename0, pSrcBufn, pRefBufn, wreal, hreal, dev, pla);


  //i16 *pMVOut = OMPMotBloEstus(pSrcBufn, pRefBufn, ws, hs, wr, hr);
  //OMPReanimation(filename0, pRefBufn, wr, hr, wreal, hreal, pMVOut);

  //Aan het typen
  //PGMWriter(filename1, pOutBuf, h, w, 'I');
  //PGMWriter(filename1, pOutBuf, h, w, 'N');
  //PGMWriter(filename0, pSrcBufn, hs, ws, 'P');
  //PGMWriter(filename1, pRefBufn, hr, wr, 'P');
  
  return 0;
}
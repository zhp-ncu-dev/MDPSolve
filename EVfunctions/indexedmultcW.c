#if !defined(_WIN32)
#define dgemm dgemm_
#define dgemv dgemv_
#endif

#include "mex.h"
#include "blas.h"
#include <math.h>
// #include <time.h>
// clock_t start, end;


//#define matvecmult matvecmult3
typedef void (*function)(double *b, double *A, double *x, mwSize m, mwSize n, double w);
function matvecmult;



/*
% EVmergefuncW Specialized indexed multiplication operation for use with EV functions
% Similar to EVmergefunc but allows a weighting vector to be used
% USAGE
%   z=EVmergefuncmW(x,xind,y,yind,w);
% INPUTS
%   x    : mx x nx matrix
%   xind : p-vector of uint32 values on [1,...,nx] (if empty nx must equal p)
%   y    : my x ny matrix
%   yind : p-vector of uint32 values on [1,...,ny] (if empty ny must equal p)
%   w    : nw-vector of positive values summing to 1
% OUTPUT
%   z    : mx/my x p matrix
%
% mx/my must be an integer
% p/nw must be an integer
*/

// basic matrix-vector multiply b=w*A*x where A is m x n
void matvecmult1(double *b, double *A, double *x, mwSize m, mwSize n, double w){
  double *bi, *bend, xj, *xend;
  bend = b + m;
  xend = x + n;
  while (x < xend){
    xj = w * *x++;
    //if (xj!=0) {
      bi=b;
      while (bi < bend) *bi++ += *A++ * xj;
    //}
    //else {
    //  A+=m;
    //}
  }
}

// basic matrix-vector multiply b=A*x where A is m x n - uses dgemm
void matvecmult2(double *b, double *A, double *x, size_t m, size_t n, double w){
/* scalar values to use in dgemm */
  char *chn = "N";
  size_t ione=1;
  double done = 1.0, dzero = 0.0;
  /* Pass arguments to Fortran by reference */
  dgemm(chn, chn, &m, &ione, &n, &w, A, &m, x, &n, &done, b, &m);
}


// basic matrix-vector multiply b=A*x where A is m x n - uses dgemv
void matvecmult3(double *b, double *A, double *x, size_t m, size_t n, double w){
/* scalar values to use in dgemm */
  char *chn = "N";
  size_t ione=1;
  double done = 1.0, dzero = 0.0;
  /* Pass arguments to Fortran by reference */
  dgemv(chn, &m, &n, &w, A, &m, x, &ione, &done, b, &ione);
}

// the indexcheck functions check index vectors for valid values and convert
// them to mwSize type
bool indexcheck8(mwSize *indout, uint8_T *indexin, mwSize p, mwSize n){
bool okay=true;
mwSize i;
  for (i=0; i<p; i++){
    if (indexin[i]>n) {okay=false; break; }
    indout[i] = (mwSize) indexin[i]; 
  }
  return(okay);
}

bool indexcheck16(mwSize *indout, uint16_T *indexin, mwSize p, mwSize n){
bool okay=true;
mwSize i;
  for (i=0; i<p; i++){
    if (indexin[i]>n) {okay=false; break; }
    indout[i] = (mwSize) indexin[i]; 
  }
  return(okay);
}

bool indexcheck32(mwSize *indout, uint32_T *indexin, mwSize p, mwSize n){
bool okay=true;
mwSize i;
  for (i=0; i<p; i++){
    if (indexin[i]>n) {okay=false; break; }
    indout[i] = (mwSize) indexin[i]; 
  }
  return(okay);
}

bool indexcheck64(mwSize *indout, uint64_T *indexin, mwSize p, mwSize n){
bool okay=true;
mwSize i;
  for (i=0; i<p; i++){
    if (indexin[i]>n) {okay=false; break; }
    indout[i] = (mwSize) indexin[i]; 
  }
  return(okay);
}

bool indexcheckD(mwSize *indout, double *indexin, mwSize p, mwSize n){
bool okay=true;
mwSize i;
  for (i=0; i<p; i++){
    if (indexin[i]>n) {okay=false; break; }
    indout[i] = (mwSize) indexin[i]; 
  }
  return(okay);
}

// 4 versions of the basic operation which depend on whether x and y have
// associated index vectors or whether every column is used
void merge00w(double *z, double *x, double *y, mwSize p, mwSize mx, mwSize my, double *w, mwSize nw){
  mwSize rx, i, iw;
  double wi, *zi;
  rx = mx/my;
  p /= nw;
  for (iw=0; iw<nw; iw++){
    wi = *w++;
    zi = z;
    for (i=0; i<p; i++){
      matvecmult(zi, x, y, rx, my, wi);
      x += mx;
      y += my;
      zi += rx;
    }
  }
}


void merge01w(double *z, double *x, double *y, mwSize *yind,   
             mwSize p, mwSize mx, mwSize my, double *w, mwSize nw){
  mwSize rx, i, iw;
  double wi, *zi;
  rx = mx/my;
  y -= my;    // convert to 1-base indexing
  p /= nw;
  for (iw=0; iw<nw; iw++){
    wi = *w++;
    zi = z;
    for (i=0; i<p; i++){
      matvecmult(zi, x, y+my*yind[i], rx, my, wi);
      x += mx;
      zi += rx;
    }
    yind += p;
  }
}

void merge10w(double *z, double *x, double *y, mwSize *xind,    
             mwSize p, mwSize mx, mwSize my, double *w, mwSize nw){
  mwSize rx, i, iw;
  double wi, *zi;
  rx = mx/my;
  x -= mx;      // convert to 1-base indexing
  p /= nw;
  for (iw=0; iw<nw; iw++){
    wi = *w++;
    zi = z;
    for (i=0; i<p; i++){
      matvecmult(zi, x+mx*xind[i], y, rx, my, wi);
      y += my;
      zi += rx;
    }
    xind += p;
  }
}

void merge11w(double *z, double *x, double *y, mwSize *xind, mwSize *yind, 
             mwSize p, mwSize mx, mwSize my, double *w, mwSize nw){
  mwSize rx, i, iw;
  double wi, *zi;
  rx = mx/my;
  x -= mx; y -= my;    // convert to 1-base indexing
  p /= nw;
  for (iw=0; iw<nw; iw++){
    wi = *w++;
    zi = z;
    for (i=0; i<p; i++){
      matvecmult(zi, x+mx*xind[i], y+my*yind[i], rx, my, wi);
      zi += rx;
    }
    xind += p;
    yind += p;
  }
}

// 4 versions for sparse y (x is assumed to be full)
void merge00sw(double *z, double *x, double *y, 
             mwSize p, mwSize mx, mwSize my, mwSize *Iry, mwSize *Jcy, double *w, mwSize nw){
  double *xj, *xji, *yj, yji;
  mwSize i, j, k, ij, nc, rx, *Iryj, Jcyij, iw;
  double wi, *zi;
  rx = mx/my;
  p /= nw;
  for (iw=0; iw<nw; iw++){
    wi = *w++;
    zi = z;
    for (j=0; j<p; ){
      xj = x + mx*j;
      Jcyij=Jcy[j++];
      nc=Jcy[j]-Jcyij;
      yj = y + Jcyij;
      Iryj = Iry + Jcyij;
      for (i=0; i<nc; i++) {
        yji=yj[i] * wi;
        ij = Iryj[i];
        xji = xj + rx*ij;
        for (k=0; k<rx; k++) zi[k] += xji[k] * yji;
      }
      zi += rx;
    }
  }
}

void merge10sw(double *z, double *x, double *y, mwSize *xind,  
             mwSize p, mwSize mx, mwSize my, mwSize *Iry, mwSize *Jcy, double *w, mwSize nw){
  double *xj, *xji, *yj, yji;
  mwSize i, j, k, ij, nc, rx, *Iryj, Jcyij, iw;
  double wi, *zi;
  rx = mx/my;
  x -= mx;  // convert to 1-base indexing
  p /= nw;
  for (iw=0; iw<nw; iw++){
    wi = *w++;
    zi = z;
    for (j=0; j<p; ){
      xj = x + mx*xind[j];
      Jcyij=Jcy[j++];
      nc=Jcy[j]-Jcyij;
      yj = y + Jcyij;
      Iryj = Iry + Jcyij;
      for (i=0; i<nc; i++) {
        yji=yj[i] * wi;
        ij = Iryj[i];
        xji = xj + rx*ij;
        for (k=0; k<rx; k++) zi[k] += xji[k] * yji;
      }
      zi += rx;
    }
  }
}

void merge01sw(double *z, double *x, double *y, mwSize *yind, 
             mwSize p, mwSize mx, mwSize my, mwSize *Iry, mwSize *Jcy, double *w, mwSize nw){
  double *xj, *xji, *yj, yji, iw;
  double wi, *zi;
  mwSize i, j, k, ij, nc, rx, *Iryj, Jcyij;  
  rx = mx/my;
  Jcy -= 1; // convert to 1-base indexing
  p /= nw;
  for (iw=0; iw<nw; iw++){
    wi = *w++;
    zi = z;
    for (j=0; j<p; j++){
      xj = x + mx*j;
      ij = yind[j];
      Jcyij=Jcy[ij++];
      nc=Jcy[ij]-Jcyij;
      yj = y + Jcyij;
      Iryj = Iry + Jcyij;
      for (i=0; i<nc; i++) {
        yji=yj[i] * wi;
        ij = Iryj[i];
        xji = xj + rx*ij;
        for (k=0; k<rx; k++) zi[k] += xji[k] * yji;
      }
      zi += rx;
    }
  }
}

void merge11sw(double *z, double *x, double *y, mwSize *xind, mwSize *yind, 
             mwSize p, mwSize mx, mwSize my, mwSize *Iry, mwSize *Jcy, double *w, mwSize nw){
  double *xj, *xji, *yj, yji;
  mwSize i, j, k, ij, nc, rx, *Iryj, Jcyij, iw;
  double wi, *zi;
  rx = mx/my;
  x -= mx; Jcy -= 1; // convert to 1-base indexing
  p /= nw;
  for (iw=0; iw<nw; iw++){
    wi = *w++;
    zi = z;
    for (j=0; j<p; j++){
      xj = x + mx*xind[j];
      ij = yind[j];
      Jcyij=Jcy[ij++];
      nc=Jcy[ij]-Jcyij;
      yj = y + Jcyij;
      Iryj = Iry + Jcyij;
      for (i=0; i<nc; i++) {
        yji=yj[i] * wi;
        ij = Iryj[i];
        xji = xj + rx*ij;
        for (k=0; k<rx; k++) zi[k] += xji[k] * yji;
      }
      zi += rx;
    }
  }
}


void mexFunction(
   int nlhs, mxArray *plhs[],
   int nrhs, const mxArray *prhs[])
{
  double  *x, *y, *z, *w;
  void *xy;
  mwSize *xind, *yind, *Iry, *Jcy;
  mwSize p, px, py, mx, my, nx, ny, i, nw;
  int alg;
  bool okay=true, ysparse;
  bool getxind=true, getyind=true;
  
  /* Error checking on inputs */  
  if (nrhs>6 || nrhs<4) mexErrMsgTxt("Invalid number of input arguments");
  if (!mxIsDouble(prhs[0]))
      mexErrMsgTxt("Input 1 must be double");  
  if (!mxIsDouble(prhs[2]))
      mexErrMsgTxt("Input 3 must be double");  
  for (i=0; i<nrhs; i++){
    if (mxIsSparse(prhs[i]) && i!=2)
      mexErrMsgTxt("Inputs cannot be sparse");
    if (mxIsComplex(prhs[i]))
      mexErrMsgTxt("Inputs cannot be complex");
  }
  if (mxIsSparse(prhs[2])) ysparse=true; else ysparse=false;
 
  mx=mxGetM(prhs[0]);
  my=mxGetM(prhs[2]);
  
  p=mx/my;
  if (p*my != mx)
    mexErrMsgTxt("rows numbers of x and y are incompatible");
  
  nx=mxGetN(prhs[0]);
  ny=mxGetN(prhs[2]);

  px=mxGetNumberOfElements(prhs[1]);
  py=mxGetNumberOfElements(prhs[3]);
  if (px>0 && py>0 && px!=py)
      mexErrMsgTxt("Index vectors must have the same number of elements");
  if (px==0) p=nx; else p=px;
  if (py==0 && ny!=p)
      mexErrMsgTxt("y has an incorrect # of columns");
  if (py>0 && py!=p)
      mexErrMsgTxt("yind has an incorrect # of elements");
  
  if (nrhs>5) {
    nw=mxGetNumberOfElements(prhs[5]);
    w=mxGetPr(prhs[5]);
  }
  else nw=1;
  if ((p/nw)*nw!=p)
     mexErrMsgTxt("yind has an incorrect # of elements relative to w");

  //start = clock();
  
  if (px>0){  
    if (mxGetClassID(prhs[1])==mxUINT64_CLASS && sizeof(mwSize)==8){
      xind=mxGetData(prhs[1]);
      getxind=false;
    }
    else {
      xind = mxMalloc(p*sizeof(mwSize));
      xy   = mxGetData(prhs[1]);
      switch (mxGetClassID(prhs[1])) {
        case mxUINT8_CLASS:  okay=indexcheck8(xind, xy, p, nx);  break; 
        case mxUINT16_CLASS: okay=indexcheck16(xind, xy, p, nx);  break; 
        case mxUINT32_CLASS: okay=indexcheck32(xind, xy, p, nx);  break; 
        case mxUINT64_CLASS: okay=indexcheck64(xind, xy, p, nx);  break; 
        case mxDOUBLE_CLASS: okay=indexcheckD(xind, xy, p, nx);  break; 
        default:
          mexErrMsgTxt("xind is an improper data type - it must be an unsigned integer type");
      }
    }
  }
  if (!okay)
     mexErrMsgTxt("xind has invalid values");
  

  if (py>0){ 
    if (mxGetClassID(prhs[3])==mxUINT64_CLASS && sizeof(mwSize)==8){
      yind=mxGetData(prhs[3]);
      getyind=false;
    }
    else {
      yind = mxMalloc(p*sizeof(mwSize));
      xy   = mxGetData(prhs[3]);
      switch (mxGetClassID(prhs[3])) {
        case mxUINT8_CLASS:  okay=indexcheck8(yind, xy, p, ny);  break; 
        case mxUINT16_CLASS: okay=indexcheck16(yind, xy, p, ny); break; 
        case mxUINT32_CLASS: okay=indexcheck32(yind, xy, p, ny); break; 
        case mxUINT64_CLASS: okay=indexcheck64(yind, xy, p, ny); break; 
        case mxDOUBLE_CLASS: okay=indexcheckD(yind, xy, p, ny);  break; 
        default:
          mexErrMsgTxt("yind is an improper data type - it must be an unsigned integer type");
      }
    }
  }
  if (!okay)
     mexErrMsgTxt("yind has invalid values");
  
  
    // end = clock();
    // printf("%12.8e  ", ((double) (end - start)) / CLOCKS_PER_SEC);

  x    = mxGetPr(prhs[0]);
  y    = mxGetPr(prhs[2]);
  
  plhs[0]=mxCreateDoubleMatrix(mx/my, p/nw, mxREAL);
  z=mxGetPr(plhs[0]);
  
  
  matvecmult = &matvecmult3;
  if (nrhs>4){
    alg = (int) *mxGetPr(prhs[4]);
    switch (alg){
      case 1:
        matvecmult = &matvecmult1;
        break;
      case 2:
        matvecmult = &matvecmult2;
        break;
    }
  }

   //start = clock();
  if (ysparse) {
    Iry = mxGetIr(prhs[2]);
    Jcy = mxGetJc(prhs[2]);
    if (px>0)
      if (py>0) merge11sw(z,x,y,xind,yind,p,mx,my,Iry,Jcy,w,nw);
      else      merge10sw(z,x,y,xind,     p,mx,my,Iry,Jcy,w,nw);  
    else
      if (py>0) merge01sw(z,x,y,     yind,p,mx,my,Iry,Jcy,w,nw); 
      else      merge00sw(z,x,y,          p,mx,my,Iry,Jcy,w,nw);
  }
  else {
    if (px>0)
      if (py>0) merge11w(z,x,y,xind,yind,p,mx,my,w,nw);
      else      merge10w(z,x,y,xind,     p,mx,my,w,nw);  
    else
      if (py>0) merge01w(z,x,y,     yind,p,mx,my,w,nw); 
      else      merge00w(z,x,y,          p,mx,my,w,nw);
  }
  
  
    // end = clock();
    // printf("    %12.8e\n", ((double) (end - start)) / CLOCKS_PER_SEC);
     
  if (px>0 && getxind) mxFree(xind);
  if (py>0 && getyind) mxFree(yind);
}


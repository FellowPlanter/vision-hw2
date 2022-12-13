#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <assert.h>
#include "image.h"
#include "matrix.h"
#include <time.h>


// Frees an array of descriptors.
// descriptor *d: the array.
// int n: number of elements in array.
void free_descriptors(descriptor *d, int n)
{
    int i;
    for(i = 0; i < n; ++i){
        free(d[i].data);
    }
    free(d);
}

// Create a feature descriptor for an index in an image.
// image im: source image.
// int i: index in image for the pixel we want to describe.
// returns: descriptor for that index.
descriptor describe_index(image im, int i)
{
    int w = 5;
    descriptor d;
    d.p.x = i%im.w;
    d.p.y = i/im.w;
    d.data = calloc(w*w*im.c, sizeof(float));
    d.n = w*w*im.c;
    int c, dx, dy;
    int count = 0;
    // If you want you can experiment with other descriptors
    // This subtracts the central value from neighbors
    // to compensate some for exposure/lighting changes.
    for(c = 0; c < im.c; ++c){
        float cval = im.data[c*im.w*im.h + i];
        for(dx = -w/2; dx < (w+1)/2; ++dx){
            for(dy = -w/2; dy < (w+1)/2; ++dy){
                float val = get_pixel(im, i%im.w+dx, i/im.w+dy, c);
                d.data[count++] = cval - val;
            }
        }
    }
    return d;
}

// Marks the spot of a point in an image.
// image im: image to mark.
// ponit p: spot to mark in the image.
void mark_spot(image im, point p)
{
    int x = p.x;
    int y = p.y;
    int i;
    for(i = -9; i < 10; ++i){
        set_pixel(im, x+i, y, 0, 1);
        set_pixel(im, x, y+i, 0, 1);
        set_pixel(im, x+i, y, 1, 0);
        set_pixel(im, x, y+i, 1, 0);
        set_pixel(im, x+i, y, 2, 1);
        set_pixel(im, x, y+i, 2, 1);
    }
}

// Marks corners denoted by an array of descriptors.
// image im: image to mark.
// descriptor *d: corners in the image.
// int n: number of descriptors to mark.
void mark_corners(image im, descriptor *d, int n)
{
    int i;
    for(i = 0; i < n; ++i){
        mark_spot(im, d[i].p);
    }
}

// Creates a 1d Gaussian filter.
// float sigma: standard deviation of Gaussian.
// returns: single row image of the filter.
image make_1d_gaussian(float sigma)
{
    printf("hi");
    int dim = (int)ceilf(6*sigma); 
    dim = dim&1 ? dim : dim+1;
    image f = make_image(dim,1,1);
    float m = 2.5f * sigma;
    for(int x=0;x<f.w;++x){
        float x1 = f.w/2 - x;
        float v = expf(-(x1*x1) / (2*sigma*sigma));
        set_pixel(f,x,0,0,v*m);
    }
    l1_normalize(f);
    return f;
}

// Smooths an image using separable Gaussian filter.
// image im: image to smooth.
// float sigma: std dev. for Gaussian.
// returns: smoothed image.
image smooth_image(image im, float sigma)
{
    image f = make_1d_gaussian(sigma);
    image res = convolve_image(im,f,1);
    f.h = f.w;
    f.w = 1;
    res = convolve_image(res,f,1);
    return res;
}

// Calculate the structure matrix of an image.
// image im: the input image.
// float sigma: std dev. to use for weighted sum.
// returns: structure matrix. 1st channel is Ix^2, 2nd channel is Iy^2,
//          third channel is IxIy.
image structure_matrix(image im, float sigma)
{
    image S = make_image(im.w, im.h, 3);
    image Ix = convolve_image(im, make_gx_filter(), 0);
    image Iy = convolve_image(im, make_gy_filter(), 0);
    for(int y=0;y<S.h;++y){
        for(int x=0;x<S.w;++x){
            float ix = get_pixel(Ix,x,y,0);
            set_pixel(S,x,y,0,ix*ix);
        }
    }
    for(int y=0;y<S.h;++y){
        for(int x=0;x<S.w;++x){
            float iy = get_pixel(Iy,x,y,0);
            set_pixel(S,x,y,1,iy*iy);
        }
    }
    for(int y=0;y<S.h;++y){
        for(int x=0;x<S.w;++x){
            float iy = get_pixel(Iy,x,y,0);
            float ix = get_pixel(Ix,x,y,0);
            set_pixel(S,x,y,2,ix*iy);
        }
    }
    S = smooth_image(S,sigma);
    return S;
}

// Estimate the cornerness of each pixel given a structure matrix S.
// image S: structure matrix for an image.
// returns: a response map of cornerness calculations.
image cornerness_response(image S)
{
    image R = make_image(S.w, S.h, 1);
    float alpha = .06f;
    for(int y=0;y<S.h;++y){
        for(int x=0;x<S.w;++x){
            float a00 = get_pixel(S,x,y,0);
            float a11 = get_pixel(S,x,y,1);
            float a10 = get_pixel(S,x,y,2);
            float det = a00*a11 - a10*a10;
            float trace2 = (a00 + a11) * (a00 + a11);
            set_pixel(R,x,y,0,det - alpha * trace2);
        }
    }
    return R;
}

// Perform non-max supression on an image of feature responses.
// image im: 1-channel image of feature responses.
// int w: distance to look for larger responses.
// returns: image with only local-maxima responses within w pixels.
image nms_image(image im, int w)
{
    image r = copy_image(im);
    for(int c=0;c<im.c;++c){
        for (int y=0;y<im.h;++y){
            for (int x=0;x<im.w;++x){
                float v = get_pixel(im,x,y,c);
                int suppressed = 0;
                for(int y1= y-w>0?y-w:0; y1 < (y+w+1<im.h?y+w+1:im.h)&&(!suppressed);++y1){
                    for(int x1=0>x-w?0:x-w; x1 < (im.w<x+w+1?im.w:x+w+1);++x1){
                        if(get_pixel(im,x1,y1,c) > v){
                            set_pixel(r,x,y,c,-999999);
                            suppressed = 1;
                            break;
                        }
                    }
                }
            } 
        }
    }
    // for every pixel in the image:
    //     for neighbors within w:
    //         if neighbor response greater than pixel response:
    //             set response to be very low (I use -999999 [why not 0??])
    return im;
}

// Perform harris corner detection and extract features from the corners.
// image im: input image.
// float sigma: std. dev for harris.
// float thresh: threshold for cornerness.
// int nms: distance to look for local-maxes in response map.
// int *n: pointer to number of corners detected, should fill in.
// returns: array of descriptors of the corners in the image.
descriptor *harris_corner_detector(image im, float sigma, float thresh, int nms, int *n)
{
    // Calculate structure matrix
    image S = structure_matrix(im, sigma);

    // Estimate cornerness
    image R = cornerness_response(S);

    // Run NMS on the responses
    image Rnms = nms_image(R, nms);


    int count = 0;
    for (int y=0;y<Rnms.h;++y){
        for (int x=0;x<Rnms.w;++x){
            float v = get_pixel(Rnms,x,y,0);
            count += v>thresh;
        }
    }

    
    *n = count; // <- set *n equal to number of corners in image.
    descriptor *d = calloc(count, sizeof(descriptor));
    int i = 0;
    for (int y=0;y<Rnms.h;++y){
        for (int x=0;x<Rnms.w;++x){
            float v = get_pixel(Rnms,x,y,0);
            if(v>thresh){
                int index = y*Rnms.w + x;
                d[i++] = describe_index(im,index);
            }
        }
    }

    free_image(S);
    free_image(R);
    free_image(Rnms);
    return d;
}

// Find and draw corners on an image.
// image im: input image.
// float sigma: std. dev for harris.
// float thresh: threshold for cornerness.
// int nms: distance to look for local-maxes in response map.
void detect_and_draw_corners(image im, float sigma, float thresh, int nms)
{
    int n = 0;
    descriptor *d = harris_corner_detector(im, sigma, thresh, nms, &n);
    mark_corners(im, d, n);
}

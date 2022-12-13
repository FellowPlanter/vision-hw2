#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <assert.h>
#include "image.h"

void l1_normalize(image im)
{
    float sums[im.c];
    for(int i=0;i<im.c;++i) sums [i]=0;
    for(int c=0;c<im.c;++c){
        for(int y=0;y<im.h;++y){
            for(int x=0;x<im.w;++x){
                sums[c] += get_pixel(im,x,y,c);
            }
        }
    }
    //printf("sums has size %d and first value %.6f", im.c, sums[0]);
    for(int c=0;c<im.c;++c){
        for(int y=0;y<im.h;++y){
            for(int x=0;x<im.w;++x){
               set_pixel(im,x,y,c,get_pixel(im,x,y,c) / sums[c]);
            }
        }
    }
}

//TODO: remove this experiment or implement properly 
typedef struct
{
    float r,g,b;
} color;
int _similar(color* a, color* b, float tolerance){
    return 0.3f*abs(a->r - b->r) + 0.5f*abs(a->g - b->g) + 0.2f*abs(a->b - b->b) < tolerance; 
}


image convolve_image(image im, image filter, int preserve)
{
    assert(filter.c == 1 || filter.c == im.c);
    if(!preserve){
        image res = make_image(im.w,im.h,1);
        for(int y=0;y<res.h;++y){
            for(int x=0;x<res.w;++x){
                float res1 = 0;
                for(int c=0;c<im.c;++c){
                    for(int y1=0;y1<filter.h;++y1){
                        for(int x1=0;x1<filter.w;++x1){
                            res1 += get_pixel(filter,x1,y1,filter.c==1?0:c) *
                                    get_pixel(im,x-filter.w/2+x1,y-filter.h/2+y1,c);
                        }
                    }
                }
                set_pixel(res,x,y,0,res1 / (float)im.c);
            }
        }
        return res;
    }
    else{
        image res = make_image(im.w,im.h,im.c);
        for(int c=0;c<im.c;++c){
            for(int y=0;y<res.h;++y){
                for(int x=0;x<res.w;++x){
                    float res1 = 0;
                    for(int y1=0;y1<filter.h;++y1){
                        for(int x1=0;x1<filter.w;++x1){
                            res1 += get_pixel(filter,x1,y1,filter.c==1?0:c) *
                                    get_pixel(im,x-filter.w/2+x1,y-filter.h/2+y1,c);
                        }
                        
                    }
                    set_pixel(res,x,y,c,res1);
                }
            }
        }
        return res;
    }
    return make_image(1,1,1);
}

image make_box_filter(int w)
{
    image res = make_image(w,w,1);
    
    float unit = 1.0f / ((float)w*(float)w);
    for(int y=0;y<res.h;++y){
        for(int x=0;x<res.w;++x){
            set_pixel(res,x,y,0,unit);
        }
    }
    return res;
}


image make_highpass_filter()
{
    image res = make_image(3,3,1);
    float data[9] = {0.0f,-1.0f,0.0f,
                    -1.0f,4.0f,-1.0f,
                     0.0f,-1.0f,0.0f};
    for(int i = 0;i<9;i++) res.data[i] = data[i];
    return res;
}

image make_sharpen_filter()
{
    image res = make_image(3,3,1);
    float data[9] = {0.0f,-1.0f,0.0f,
                     -1.0f,5.0f,-1.0f, 
                     0.0f,-1.0f,0.0f};
    for(int i = 0;i<9;i++) res.data[i] = data[i];
    return res;
}

image make_emboss_filter()
{
    image res = make_image(3,3,1);
    float data[9] = {-2.0f,-1.0f,0.0f, 
                     -1.0f,1.0f,1.0f,
                     0.0f,1.0f,2.0f};
    for(int i = 0;i<9;i++) res.data[i] = data[i];
    return res;
}

// Question 2.2.1: Which of these filters should we use preserve when we run our convolution and which ones should we not? Why?
// Answer: TODO

// Question 2.2.2: Do we have to do any post-processing for the above filters? Which ones and why?
// Answer: TODO

image make_gaussian_filter(float sigma)
{
    int dim = (int)ceilf(6*sigma); 
    dim = dim%2 ? dim : dim+1;
    image res = make_image(dim,dim,1);
    for(int y=0;y<res.h;++y){
        float y1 = res.h/2 - y;
        for(int x=0;x<res.w;++x){
            float x1 = res.w/2 - x;
            float v = expf(-(x1*x1+y1*y1) / (2*sigma*sigma));
            set_pixel(res,x,y,0,v);
        }
    }
    l1_normalize(res);
    return res;
}

image fast_gaussian_blur(image im, float sigma)
{
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
    image res = convolve_image(im,f,1);
    f.h = dim;
    f.w = 1;
    res = convolve_image(res,f,1);
    return res;
}

image add_image(image a, image b)
{
    assert(a.h == b.h && a.w == b.w);
    image res = make_image(a.w,a.h,a.c);
    for(int c=0;c<res.c;++c){
        for(int y=0;y<res.h;++y){
            for(int x=0;x<res.w;++x){
                float v = get_pixel(a,x,y,c)+get_pixel(b,x,y,c);
                v = v>1?1.0f : v;
                v = v<0 ? 0.0f : v;
                set_pixel(res,x,y,c,v);
            }
        }
    }
    return res;
}

image sub_image(image a, image b)
{
    assert(a.h == b.h && a.w == b.w);
    image res = make_image(a.w,a.h,a.c);
    for(int c=0;c<res.c;++c){
        for(int y=0;y<res.h;++y){
            for(int x=0;x<res.w;++x){
                float v = get_pixel(a,x,y,c)-get_pixel(b,x,y,c);
                set_pixel(res,x,y,c,v);
            }
        }
    }
    return res;
}

image make_edge_filter()
{
    image res = make_image(3,3,1);
    float data[9] = {-1.0f,-1.0f,0.0f,
                     -1.0f,4.0f,0.0f,
                     0.0f,0.0f,0.0f};
    for(int i = 0;i<9;i++) res.data[i] = data[i];
    return res;
}

image make_gx_filter()
{
    image res = make_image(3,3,1);
    float data[9] = {-1.0f,0.0f,1.0f,
                     -2.0f,0.0f,2.0f,
                     -1.0f,0.0f,1.0f};
    for(int i = 0;i<9;i++) res.data[i] = data[i];
    return res;
}

image make_gy_filter()
{
    image res = make_image(3,3,1);
    float data[9] = {-1.0f,-2.0f,-1.0f,
                     0.0f,0.0f,0.0f,
                     1.0f,2.0f,1.0f};
    for(int i = 0;i<9;i++) res.data[i] = data[i];
    return res;
}

void feature_normalize(image im)
{
    float max[im.c];
    float min[im.c];
    for(int i=0;i<im.c;++i){
        max[i]=get_pixel(im,0,0,i);
        min[i]=max[i];
    }
    for(int c=0;c<im.c;++c){
        for(int y=0;y<im.h;++y){
            for(int x=0;x<im.w;++x){
               float v = get_pixel(im,x,y,c);
               if(v < min[c]) min[c] = v;
               if(v > max[c]) max[c] = v;
            }
        }
    }
    float range[im.c];
    for(int i=0;i<im.c;++i){
        range[i] = max[i] - min[i];
    }
     
    for(int c=0;c<im.c;++c){
        for(int y=0;y<im.h;++y){
            for(int x=0;x<im.w;++x){
                float v = get_pixel(im,x,y,c);
                set_pixel(im,x,y,c, range[c]!=0 ? (v-min[c]) / range[c] : 0.0f);
            }
        }
    }
}

image *sobel_image(image im)
{
    image gximg = convolve_image(im, make_gx_filter(), 0);
    image gyimg = convolve_image(im, make_gy_filter(), 0);
    image* res = (image*)calloc(2, sizeof(image));
    image mag = make_image(im.w,im.h,1);
    image dir = make_image(im.w,im.h,1);
    *res = mag;
    *(res+1) = dir;
    
    for(int y=0;y<im.h;++y){
        for(int x=0;x<im.w;++x){
            float gx = get_pixel(gximg,x,y,0);
            float gy = get_pixel(gyimg,x,y,0);
            float v = sqrtf(gx*gx + gy*gy);
            float d = atan2f(gy,gx);
            set_pixel(mag,x,y,0,v);
            set_pixel(dir,x,y,0,d);
        }
    }
    //feature_normalize(dir);
    //clamp_image(mag);
    return res;

}

image colorize_sobel(image im)
{
    image ret = make_image(im.w, im.h, 3);
    im = fast_gaussian_blur(im, 3);
    image* res = sobel_image(im);
    feature_normalize(*res);
    feature_normalize(*(res+1));
    for(int y=0;y<im.h;++y){
        for(int x=0;x<im.w;++x){
            set_pixel(ret, x, y, 0, get_pixel(*(res+1),x,y,0));
            set_pixel(ret, x, y, 1, 1.0f);
            set_pixel(ret, x, y, 2, get_pixel(*res,x,y,0));
        }
    }
    hsv_to_rgb(ret);
    return ret;
}
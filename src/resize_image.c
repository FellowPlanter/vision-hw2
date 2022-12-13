#include <math.h>
#include <stdio.h>
#include "image.h"

float nn_interpolate(image im, float x, float y, int c)
{
    //if(x<0||y<0||x>=im.w||y>=im.h) printf("invalid coordinates");
    return get_pixel(im,round(x),round(y),c);
}

image nn_resize(image im, int w, int h)
{
    image res = make_image(w, h, im.c);
    float x_step = (float)im.w / (float)w;
    float y_step = (float)im.h / (float)h;
    for(int y=0;y<h;++y){
        for(int x=0;x<w;++x){
            for(int c=0;c<im.c;++c){
                set_pixel(res, x, y, c, nn_interpolate(im, x*x_step - 0.5f + (x_step/2.0f), y*y_step - 0.5f + (y_step/2.0f), c));
            }
        }
    }
    return res;
}

//-0.5 + 4/7 / 2

float bilinear_interpolate(image im, float x, float y, int c)
{
    float X = trunc(x);
    float Y = trunc(y);
    return get_pixel(im,X+1,Y+1,c) * (x-X) * (y-Y) +
           get_pixel(im,X+1,Y,c) * (x-X) * (Y+1-y) +
           get_pixel(im,X,Y+1,c) * (X+1-x) * (y-Y) +
           get_pixel(im,X,Y,c) * (X+1-x) * (Y+1-y);
}

image bilinear_resize(image im, int w, int h)
{
    image res = make_image(w, h, im.c);
    float x_step = (float)im.w / (float)w;
    float y_step = (float)im.h / (float)h;
    for(int y=0;y<h;++y){
        for(int x=0;x<w;++x){
            for(int c=0;c<im.c;++c){
                set_pixel(res, x, y, c, bilinear_interpolate(im, x*x_step - 0.5f + (x_step/2.0f), y*y_step - 0.5f + (y_step/2.0f), c));
            }
        }
    }
    return res;
}

#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <math.h>
#include "image.h"

float get_pixel(image im, int x, int y, int c)
{
    if(im.c<0||c>=im.c){
        printf("Invalid channel\n");
        return 0;
    }
    if(x<0) x=0;
    if(x>=im.w) x = im.w-1;
    if(y<0) y=0;
    if(y>=im.h) y = im.h-1;
    return im.data[im.h*im.w*c + y*im.w + x];
}

void set_pixel(image im, int x, int y, int c, float v)
{
    if(x<0||y<0||y>=im.h||x>=im.w||c<0||c>im.c){
        printf("Invalid arguments!");
        return;
    }
    im.data[im.h*im.w*c + y*im.w + x] = v;
}

image copy_image(image im)
{
    image copy = make_image(im.w, im.h, im.c);
    memcpy(copy.data, im.data, im.h*im.w*im.c *sizeof(float));
    return copy;
}

image rgb_to_grayscale(image im)
{
    assert(im.c == 3);
    image gray = make_image(im.w, im.h, 1);
    for(int y=0;y<im.h;++y){
        for(int x=0;x<im.w;++x){
            float r = get_pixel(im,x,y,0);
            float g = get_pixel(im,x,y,1);
            float b = get_pixel(im,x,y,2);
            set_pixel(gray, x,y,0,r*0.299f + g*0.587f + b*0.114f);
        }
    }
    return gray;
}

void shift_image(image im, int c, float v)
{
    for(int y=0;y<im.h;++y){
        for(int x=0;x<im.w;++x){
            set_pixel(im, x, y, c, get_pixel(im,x,y,c) + v);
        }
    }
}

void scale_image(image im, int c, float v)
{
    for(int y=0;y<im.h;++y){
        for(int x=0;x<im.w;++x){
            set_pixel(im, x, y, c, get_pixel(im,x,y,c) * v);
        }
    }
}


void clamp_image(image im)
{
    for(int y=0;y<im.h;++y){
        for(int x=0;x<im.w;++x){
            for(int c=0;c<im.c;++c){
                float v = get_pixel(im,x,y,c);
                if(v<0.0) v = 0.0;
                if(v>1.0) v = 1.0;
                set_pixel(im, x, y, c, v);
            }
        }
    }
}

float three_way_max(float a, float b, float c)
{
    return (a > b) ? ( (a > c) ? a : c) : ( (b > c) ? b : c) ;
}

float three_way_min(float a, float b, float c)
{
    return (a < b) ? ( (a < c) ? a : c) : ( (b < c) ? b : c) ;
}

void rgb_to_hsv(image im)
{
    for(int y=0;y<im.h;++y){
        for(int x=0;x<im.w;++x){
            float r = get_pixel(im,x,y,0);
            float g = get_pixel(im,x,y,1);
            float b = get_pixel(im,x,y,2);

            float V = three_way_max(r,g,b);
            
            float m = three_way_min(r,g,b);
            float C = V - m;
            float S = V ? C / V : 0;
            
            float H;
            if(C==0) H = 0;
            else if(r>=g && r>=b) H = (g-b)/C;
            else if(g>=r && g>=b) H = (b-r)/C + 2.0f;
            else H = (r-g)/C + 4.0f;
            H = H<0 ? H/6.0f + 1.0f : H/6.0f;

            set_pixel(im,x,y,0,H);
            set_pixel(im,x,y,1,S);
            set_pixel(im,x,y,2,V);
            
        }
    }
}

void hsv_to_rgb(image im){
    for(int y=0;y<im.h;++y){
        for(int x=0;x<im.w;++x){
            float h = get_pixel(im,x,y,0)*360.0f;
            float s = get_pixel(im,x,y,1);
            float v = get_pixel(im,x,y,2);
            
            double r = 0, g = 0, b = 0;

            if (s == 0){
                r = v;
                g = v;
                b = v;
            }
            else{
                int i;
                double f, p, q, t;

                if (h >= 360)
                    h = 0;
                else
                    h = h / 60;

                i = (int)trunc(h);
                f = h - i;

                p = v * (1.0 - s);
                q = v * (1.0 - (s * f));
                t = v * (1.0 - (s * (1.0 - f)));
                switch (i){
                case 0:
                    r = v;g = t;b = p;break;
                case 1:
                    r = q;g = v;b = p;break;
                case 2:
                    r = p;g = v;b = t;break;
                case 3:
                    r = p;g = q;b = v;break;
                case 4:
                    r = t;g = p;b = v;break;
                default:
                    r = v;g = p;b = q;break;
                }
            }
            set_pixel(im,x,y,0,(float)r);
            set_pixel(im,x,y,1,(float)g);
            set_pixel(im,x,y,2,(float)b);
        }
    }
}

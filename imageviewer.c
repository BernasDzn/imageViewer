#include<stdio.h>
#include<stdlib.h>
#include<SDL2/SDL.h>
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#include "tinyfiledialogs.h"

int imageWidth, imageHeight;

char ** getImagesInDirectory(char * imageFile){
    char actualpath [PATH_MAX];
    realpath(imageFile, actualpath);
    return NULL;
}

int calculateImageRatio(int windowWidth, int windowHeight, int imageWidth, int imageHeight, SDL_Rect * rect){
    float windowAspect = (float)windowWidth / windowHeight;
    float imageAspect = (float)imageWidth / imageHeight;
    
    int destWidth, destHeight;
    
    if (imageAspect > windowAspect) {
        destWidth = windowWidth;
        destHeight = (int)(windowWidth / imageAspect);
    } else {
        destHeight = windowHeight;
        destWidth = (int)(windowHeight * imageAspect);
    }
    int x = (windowWidth - destWidth) / 2;
    int y = (windowHeight - destHeight) / 2;
    
    *rect = (SDL_Rect){x, y, destWidth, destHeight};
    return 0;
}

void doRenderHUD(SDL_Renderer * prenderer, SDL_Texture * ptexture, int windowWidth, int windowHeight, int hudSize){
    // draw current image
    SDL_Rect * curImagePreview = &(SDL_Rect){
        windowWidth/2 - hudSize/2,
        windowHeight - hudSize*2,
        hudSize,
        hudSize
    };
    SDL_RenderCopy(prenderer, ptexture, NULL, curImagePreview);
    SDL_SetRenderDrawColor(prenderer,0xff,0xff,0xff,0xff);
    SDL_RenderDrawRect(prenderer,curImagePreview);
    SDL_SetRenderDrawColor(prenderer,0x00,0x00,0x00,0xff);

    // draw next images
    // to be later changed by actual logic
    if(1){
        // draw previous
        SDL_Rect * curImagePreview = &(SDL_Rect){
            windowWidth/2 - hudSize/2 - hudSize*1.5,
            windowHeight - hudSize*2,
            hudSize,
            hudSize
        };
        SDL_SetRenderDrawColor(prenderer,0xff,0xff,0xff,0xff);
        SDL_RenderDrawRect(prenderer,curImagePreview);
        SDL_SetRenderDrawColor(prenderer,0x00,0x00,0x00,0xff);
    }
    if(1){
        // draw next
        SDL_Rect * curImagePreview = &(SDL_Rect){
            windowWidth/2 - hudSize/2 + hudSize*1.5,
            windowHeight - hudSize*2,
            hudSize,
            hudSize
        };
        SDL_SetRenderDrawColor(prenderer,0xff,0xff,0xff,0xff);
        SDL_RenderDrawRect(prenderer,curImagePreview);
        SDL_SetRenderDrawColor(prenderer,0x00,0x00,0x00,0xff);
    }
}

void doRenderCycle(SDL_Window* pwindow, SDL_Renderer * prenderer, SDL_Texture * ptexture, SDL_Rect * ratioPreservedSize){
    SDL_Event event;
    int running = 1;
    int dragging = 0;
    int w,h;
    SDL_GetWindowSize(pwindow,&w,&h);
    float zoom = 1.0f;
    SDL_Rect scaledRect;
    int offsetX = 0, offsetY = 0;
    int dragStartX = 0, dragStartY = 0;
    
    while(running){
        while(SDL_PollEvent(&event)){
            if((event.type == SDL_QUIT) ||
               (event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_ESCAPE)){
                running = 0;
            }
            if(event.type == SDL_WINDOWEVENT && event.window.event == SDL_WINDOWEVENT_RESIZED){
                SDL_GetWindowSize(pwindow,&w,&h);
                calculateImageRatio(w,h,imageWidth,imageHeight,ratioPreservedSize);
            }
            if(event.type == SDL_MOUSEWHEEL){
                if(event.wheel.y > 0){
                    zoom = SDL_clamp(zoom*1.1,0.25,10);
                } else if(event.wheel.y < 0){
                     zoom = SDL_clamp(zoom*0.9,0.25,10);
                }
            }
            if(event.type == SDL_MOUSEBUTTONDOWN && event.button.button == SDL_BUTTON_LEFT){
                dragging = 1;
                dragStartX = event.button.x - offsetX;
                dragStartY = event.button.y - offsetY;
            }
            if(event.type == SDL_MOUSEBUTTONUP && event.button.button == SDL_BUTTON_LEFT){
                dragging = 0;
            }
            if(event.type == SDL_MOUSEMOTION && dragging){
                offsetX = event.motion.x - dragStartX;
                offsetY = event.motion.y - dragStartY;
            }
        }
        
        scaledRect.w = (int)(ratioPreservedSize->w * zoom);
        scaledRect.h = (int)(ratioPreservedSize->h * zoom);
        
        int minX = -(scaledRect.w - 50);
        int maxX = w - 50;
        int minY = -(scaledRect.h - 50);
        int maxY = h - 50;
        
        int centeredX = (w - scaledRect.w) / 2;
        int centeredY = (h - scaledRect.h) / 2;
        
        offsetX = SDL_clamp(offsetX, minX - centeredX, maxX - centeredX);
        offsetY = SDL_clamp(offsetY, minY - centeredY, maxY - centeredY);
        
        scaledRect.x = centeredX + offsetX;
        scaledRect.y = centeredY + offsetY;

        SDL_RenderClear(prenderer);
        SDL_RenderCopy(prenderer, ptexture, NULL, &scaledRect);
        doRenderHUD(prenderer, ptexture, w, h, 50);
        SDL_RenderPresent(prenderer);
    }
}

char * pickFile(){
    char const * filterPatterns[5] = { "*.jpg", "*.jpeg", "*.png", "*.bmp", "*.gif" };
    return (char *)tinyfd_openFileDialog(
        "Select an Image",
        "",
        5,
        filterPatterns,
        "Image files",
        0
    );
}

int main(){
    const char *title = "Image Viewer";
    int x, y, w, h, i, j, imageChannels;

    // pick file
    char *filename = pickFile();
    if(filename==NULL){
        printf("No image selected.\n");
        SDL_Init(SDL_INIT_VIDEO);
        tinyfd_notifyPopup("Error","No image was selected.","warning");
        SDL_Quit();
        return 0; 
    }
    
    //window settings
    x = SDL_WINDOWPOS_CENTERED;
    y = SDL_WINDOWPOS_CENTERED;
    w = 900;
    h = 600;

    // load image
    unsigned char *img = stbi_load(filename, &imageWidth, &imageHeight, &imageChannels, 0);
    if(img == NULL){
        printf("Error loading image.\n");
        return 0;
    }
    // initialize window and related resources
    SDL_Window *pwindow = SDL_CreateWindow(title, x, y, w, h, 0);
    SDL_SetWindowResizable(pwindow, (SDL_bool)SDL_WINDOW_RESIZABLE);
    SDL_Surface *psurface = SDL_CreateRGBSurface(0,imageWidth,imageHeight,32,0,0,0,0);
    SDL_Renderer *prenderer = SDL_CreateRenderer(pwindow,-1,0);
    // fill surface with data from image
    Uint32 color;
    SDL_Rect pixel = (SDL_Rect){0,0,1,1};
    for(i=0;i<imageWidth;i++){
        for(j=0;j<imageHeight;j++){
            unsigned int bytePerPixel = imageChannels;
            unsigned char * pixelOffset = img + (i + imageWidth * j) * bytePerPixel;
            unsigned char r = pixelOffset[0];
            unsigned char g = pixelOffset[1];
            unsigned char b = pixelOffset[2];
            unsigned char a = imageChannels >= 4 ? pixelOffset[3] : 0xff;
            pixel.x=i;
            pixel.y=j;
            color = SDL_MapRGBA(psurface->format,r,g,b,a);
            SDL_FillRect(psurface, &pixel, color);
        }
    }
    // image file no longer needed
    stbi_image_free(img);
    // create texture from surface
    SDL_Texture * ptexture = SDL_CreateTextureFromSurface(prenderer,psurface);
    // surface no longer needed
    SDL_FreeSurface(psurface);
    // render cycle

    SDL_Rect ratioPreservedSize;
    calculateImageRatio(w,h,imageWidth,imageHeight, &ratioPreservedSize);
    doRenderCycle(pwindow, prenderer, ptexture, &ratioPreservedSize);
    // clean up
    SDL_DestroyTexture(ptexture);
    SDL_DestroyRenderer(prenderer);
    SDL_DestroyWindow(pwindow);
    SDL_Quit();
    return 0;
}


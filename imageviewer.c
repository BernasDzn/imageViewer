#include<stdio.h>
#include<stdlib.h>
#define SDL_MAIN_HANDLED
#include<SDL2/SDL.h>
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#include "tinyfiledialogs.h"
#include <limits.h>
#include <string.h>
#include <dirent.h>
#include <libgen.h>

#ifdef _WIN32
    #include <windows.h>

    char *realpath(const char *path, char *resolved_path)
    {
        // le troll face
        strcpy(resolved_path, path);
        return resolved_path;
    }
#endif

int imageWidth, imageHeight;

char * currentImagePath;
char * previousImagePath;
char * nextImagePath;
char ** images;

int currentIndex = 0;
int imageCount = 0;

void updateImagePaths() {

    if(currentIndex >= 0 && currentIndex < imageCount) {

        printf("Current image: %s\n", images[currentIndex]);

        if(currentImagePath) free(currentImagePath);
        if(previousImagePath) free(previousImagePath);
        if(nextImagePath) free(nextImagePath);
        
        currentImagePath = strdup(images[currentIndex]);
        previousImagePath = (currentIndex > 0) ? strdup(images[currentIndex - 1]) : NULL;
        nextImagePath = (currentIndex < imageCount - 1) ? strdup(images[currentIndex + 1]) : NULL;
    }
}

void nextImage() {
    if(currentIndex < imageCount - 1) {
        currentIndex++;
        updateImagePaths();
    }
}

void previousImage() {
    if(currentIndex > 0) {
        currentIndex--;
        updateImagePaths();
    }
}

char **getImagesInDirectory(char *imageFile, int *count) {
    char actualpath[PATH_MAX];
    if (!realpath(imageFile, actualpath)) return NULL;
    
    char *dir = dirname(strdup(actualpath));
    DIR *d = opendir(dir);
    if (!d) return NULL;
    
    // count images
    struct dirent *entry;
    *count = 0;
    while ((entry = readdir(d)) != NULL) {
        char *ext = strrchr(entry->d_name, '.');
        if (ext && (strcasecmp(ext, ".jpg") == 0 || strcasecmp(ext, ".jpeg") == 0 ||
                    strcasecmp(ext, ".png") == 0 || strcasecmp(ext, ".bmp") == 0 ||
                    strcasecmp(ext, ".gif") == 0)) {
            (*count)++;
        }
    }
    
    // allocate array
    char **images = malloc(sizeof(char*) * (*count + 1));
    
    // fill array
    rewinddir(d);
    int i = 0;
    while ((entry = readdir(d)) != NULL && i < *count) {
        char *ext = strrchr(entry->d_name, '.');
        if (ext && (strcasecmp(ext, ".jpg") == 0 || strcasecmp(ext, ".jpeg") == 0 ||
                    strcasecmp(ext, ".png") == 0 || strcasecmp(ext, ".bmp") == 0 ||
                    strcasecmp(ext, ".gif") == 0)) {
            char fullpath[PATH_MAX];
            snprintf(fullpath, PATH_MAX, "%s/%s", dir, entry->d_name);
            images[i++] = strdup(fullpath);
        }
    }
    images[*count] = NULL;
    
    closedir(d);
    free(dir);
    return images;
}

int calculateImageRatio(int windowWidth, int windowHeight, SDL_Rect * rect){
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

void doRenderHUD(SDL_Renderer * prenderer, SDL_Texture * ptexture, int windowWidth, int windowHeight, int hudSize, 
                SDL_Texture * pprevimagetexture, SDL_Texture * pnextimagetexture){
    
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

    // draw prev and next image previews
    // draw previous
    if(pprevimagetexture!=NULL){
        SDL_Rect * prevImagePreview = &(SDL_Rect){
            windowWidth/2 - hudSize/2 - hudSize*1.5,
            windowHeight - hudSize*2,
            hudSize,
            hudSize
        };
        SDL_RenderCopy(prenderer, pprevimagetexture, NULL, prevImagePreview);
        SDL_SetRenderDrawColor(prenderer,0xff,0xff,0xff,0xff);
        SDL_RenderDrawRect(prenderer,prevImagePreview);
        SDL_SetRenderDrawColor(prenderer,0x00,0x00,0x00,0xff);
    }
    // draw next
    if(pnextimagetexture!=NULL){
        SDL_Rect * nextImagePreview = &(SDL_Rect){
            windowWidth/2 - hudSize/2 + hudSize*1.5,
            windowHeight - hudSize*2,
            hudSize,
            hudSize
        };
        SDL_RenderCopy(prenderer, pnextimagetexture, NULL, nextImagePreview);
        SDL_SetRenderDrawColor(prenderer,0xff,0xff,0xff,0xff);
        SDL_RenderDrawRect(prenderer,nextImagePreview);
        SDL_SetRenderDrawColor(prenderer,0x00,0x00,0x00,0xff);
    }
}

SDL_Texture* loadImageTexture(const char* imagePath, SDL_Renderer* prenderer, int* outWidth, int* outHeight) {
    int imageChannels;
    
    printf("Opening path: %s\n", imagePath);

    // load image
    unsigned char *img = stbi_load(imagePath, outWidth, outHeight, &imageChannels, 0);
    if(img == NULL){
        printf("Failed to load image: %s\n", stbi_failure_reason());
        return NULL;
    }
    
    // create surface
    SDL_Surface *psurface = SDL_CreateRGBSurface(0, *outWidth, *outHeight, 32, 0, 0, 0, 0);
    
    // fill surface with data from image
    Uint32 color;
    SDL_Rect pixel = (SDL_Rect){0, 0, 1, 1};
    for(int i = 0; i < *outWidth; i++){
        for(int j = 0; j < *outHeight; j++){
            unsigned int bytePerPixel = imageChannels;
            unsigned char * pixelOffset = img + (i + *outWidth * j) * bytePerPixel;
            unsigned char r = pixelOffset[0];
            unsigned char g = pixelOffset[1];
            unsigned char b = pixelOffset[2];
            unsigned char a = imageChannels >= 4 ? pixelOffset[3] : 0xff;
            pixel.x = i;
            pixel.y = j;
            color = SDL_MapRGBA(psurface->format, r, g, b, a);
            SDL_FillRect(psurface, &pixel, color);
        }
    }
    
    // image file no longer needed
    stbi_image_free(img);
    
    // create texture from surface
    SDL_Texture * ptexture = SDL_CreateTextureFromSurface(prenderer, psurface);
    
    // surface no longer needed
    SDL_FreeSurface(psurface);
    
    return ptexture;
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
                calculateImageRatio(w,h,ratioPreservedSize);
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
            if(event.type == SDL_KEYUP){
                if(event.key.keysym.sym == SDLK_LEFT){
                    previousImage();
                }
                if(event.key.keysym.sym == SDLK_RIGHT){
                    nextImage();
                }
                ptexture = loadImageTexture(currentImagePath, prenderer, &imageWidth, &imageHeight);
                calculateImageRatio(w,h,ratioPreservedSize);
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
        doRenderHUD(prenderer, ptexture, w, h, 50, 
            loadImageTexture(previousImagePath,prenderer,&imageWidth,&imageHeight), 
            loadImageTexture(nextImagePath,prenderer,&imageWidth,&imageHeight));
        SDL_RenderPresent(prenderer);
    }
}

char * pickFileUI(){
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

void pickFile() {

    // pick file
    char *filename = pickFileUI();
    if(filename==NULL){
        printf("No image selected.\n");
        SDL_Init(SDL_INIT_VIDEO);
        tinyfd_notifyPopup("Error","No image was selected.","warning");
        SDL_Quit();
        exit(1);
    }

    setupImageList(filename);
}

void setupImageList(char* imageFile) {

    printf("Selected file: %s\n", filename);
    
    images = getImagesInDirectory(imageFile, &imageCount);
    // get current index
    char actualpath[PATH_MAX];
    realpath(imageFile, actualpath);
    for(int k = 0; k < imageCount; k++){
        if(strcmp(images[k], actualpath) == 0){
            currentIndex = k;
            break;
        }
    }

    updateImagePaths();
}

int main(int argc, char* argv[]){

    const char *title = "Image Viewer";
    int x, y, w, h, minW, minH;

    if (argc == 1 ) {
        
        // Pick file UI
        pickFile();

    } else if (argc == 2 ) {

        // Use file from command line argument
        char* imageFile = argv[1];
        setupImageList(imageFile);

    } else {
        printf("Usage: %s [image_file]\n", argv[0]);
        exit(1);
    }
    
    //window settings
    x = SDL_WINDOWPOS_CENTERED;
    y = SDL_WINDOWPOS_CENTERED;
    w = 900;
    h = 600;
    minW=300;
    minH=200;

    // initialize window and related resources
    SDL_Window *pwindow = SDL_CreateWindow(title, x, y, w, h, 0);
    SDL_SetWindowResizable(pwindow, (SDL_bool)SDL_WINDOW_RESIZABLE);
    SDL_SetWindowMinimumSize(pwindow, minW, minH);
    SDL_Renderer *prenderer = SDL_CreateRenderer(pwindow,-1,0);
    
    // load initial image
    SDL_Texture * ptexture = loadImageTexture(currentImagePath, prenderer, &imageWidth, &imageHeight);
    if(ptexture == NULL){
        printf("Error loading image.\n");
        SDL_DestroyRenderer(prenderer);
        SDL_DestroyWindow(pwindow);
        SDL_Quit();
        return 0;
    }
    
    printf("Just loaded image: %s\n", currentImagePath);
    printf("Cached images count: %d\n", imageCount);

    // render cycle
    SDL_Rect ratioPreservedSize;
    calculateImageRatio(w,h, &ratioPreservedSize);
    doRenderCycle(pwindow, prenderer, ptexture, &ratioPreservedSize);
    // clean up
    if(currentImagePath) free(currentImagePath);
    if(previousImagePath) free(previousImagePath);
    if(nextImagePath) free(nextImagePath);
    for(int l = 0; l<imageCount;l++){
        free(images[l]);
    }
    free(images);
    SDL_DestroyTexture(ptexture);
    SDL_DestroyRenderer(prenderer);
    SDL_DestroyWindow(pwindow);
    SDL_Quit();
    return 0;
}
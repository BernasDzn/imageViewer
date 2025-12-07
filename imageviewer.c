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

typedef struct {
    char previewCount;
} settings;

// The current image index
int currentIndex = 0;

char ** images; // Full image path buffer
SDL_Texture ** imageTextures; // Image texture sparsebuffer
int imageCount = 0; // Image buffer size

settings appSettings = {
    .previewCount = 1
};

char* currentImage() {
    return images[currentIndex];
}

char* indexImage(int i) {
    if(currentIndex + i < imageCount)
        return images[currentIndex + i];
    return NULL;
}

void decrementIndex() {
    if(currentIndex > 0)
        currentIndex--;
}

void incrementIndex() {
    if(currentIndex < imageCount - 1)
        currentIndex++;
}

char** getImagesInDirectory(char* imageFile, int* count) {

    char *dir = dirname(strdup(imageFile));
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

    // Allocate sparse texture buffer with same size
    imageTextures = malloc(sizeof(SDL_Texture*) * (*count));
    for (int j = 0; j < *count; j++) {
        imageTextures[j] = NULL;
    }

    return images;
}

int calculateImageRatio(int windowWidth, int windowHeight, SDL_Rect * rect, SDL_Texture * ptexture) {

    int imageWidth, imageHeight;
    SDL_QueryTexture(ptexture, NULL, NULL, &imageWidth, &imageHeight);

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

void loadImageTexture(const char* imagePath, int index, SDL_Renderer* prenderer) {
    int imageChannels;
    
    printf("Opening path: %s\n", imagePath);

    // load image
    int imageWidth, imageHeight;
    unsigned char *img = stbi_load(imagePath, &imageWidth, &imageHeight, &imageChannels, 0);
    if (!img) {
        printf("Failed to load image: %s\n", stbi_failure_reason());
        exit(1);
    }
    
    // create surface
    SDL_Surface *psurface = SDL_CreateRGBSurface(0, imageWidth, imageHeight, 32, 0,0,0,0);
    
    // fill surface with data from image
    Uint32 color;
    SDL_Rect pixel = (SDL_Rect){0, 0, 1, 1};
    for(int i = 0; i < imageWidth; i++){
        for(int j = 0; j < imageHeight; j++){
            unsigned int bytePerPixel = imageChannels;
            unsigned char * pixelOffset = img + (i + imageWidth * j) * bytePerPixel;
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
    
    // cache texture
    imageTextures[index] = ptexture;
}

void doRenderHUD(SDL_Renderer * prenderer, SDL_Texture * ptexture, int windowWidth, int windowHeight, int hudSize){

    for (int i = -appSettings.previewCount; i <= appSettings.previewCount; i++) {

        // Draw previews
        SDL_Rect previewRect = {
            windowWidth / 2 + i * (hudSize + 10) - hudSize / 2,
            windowHeight - hudSize - 10,
            hudSize,
            hudSize
        };

        int previewIndex = currentIndex + i;
        if (previewIndex >= 0 && previewIndex < imageCount) {
         
            // Load texture if not already loaded
            if (imageTextures[previewIndex] == NULL) {
                loadImageTexture(images[previewIndex], previewIndex, prenderer);
            }

            SDL_RenderCopy(prenderer, imageTextures[previewIndex], NULL, &previewRect);
            SDL_SetRenderDrawColor(prenderer, 0xff, 0xff, 0xff, 0xff);
            SDL_RenderDrawRect(prenderer, &previewRect);
            SDL_SetRenderDrawColor(prenderer, 0x00, 0x00, 0x00, 0xff);
        }
    }

    // draw current image
    // SDL_Rect * curImagePreview = &(SDL_Rect){
    //     windowWidth/2 - hudSize/2,
    //     windowHeight - hudSize*2,
    //     hudSize,
    //     hudSize
    // };

    // SDL_RenderCopy(prenderer, ptexture, NULL, curImagePreview);
    // SDL_SetRenderDrawColor(prenderer,0xff,0xff,0xff,0xff);
    // SDL_RenderDrawRect(prenderer,curImagePreview);
    // SDL_SetRenderDrawColor(prenderer,0x00,0x00,0x00,0xff);

    // draw prev and next image previews
    // draw previous
    // if(pprevimagetexture!=NULL){
    //     SDL_Rect * prevImagePreview = &(SDL_Rect){
    //         windowWidth/2 - hudSize/2 - hudSize*1.5,
    //         windowHeight - hudSize*2,
    //         hudSize,
    //         hudSize
    //     };
    //     SDL_RenderCopy(prenderer, pprevimagetexture, NULL, prevImagePreview);
    //     SDL_SetRenderDrawColor(prenderer,0xff,0xff,0xff,0xff);
    //     SDL_RenderDrawRect(prenderer,prevImagePreview);
    //     SDL_SetRenderDrawColor(prenderer,0x00,0x00,0x00,0xff);
    // }
    // // draw next
    // if(pnextimagetexture!=NULL){
    //     SDL_Rect * nextImagePreview = &(SDL_Rect){
    //         windowWidth/2 - hudSize/2 + hudSize*1.5,
    //         windowHeight - hudSize*2,
    //         hudSize,
    //         hudSize
    //     };
    //     SDL_RenderCopy(prenderer, pnextimagetexture, NULL, nextImagePreview);
    //     SDL_SetRenderDrawColor(prenderer,0xff,0xff,0xff,0xff);
    //     SDL_RenderDrawRect(prenderer,nextImagePreview);
    //     SDL_SetRenderDrawColor(prenderer,0x00,0x00,0x00,0xff);
    // }
}

void loadCurrentTexture(SDL_Renderer* prenderer) {

    char* imagePath = currentImage();
    if(imageTextures[currentIndex] == NULL){
        loadImageTexture(imagePath, currentIndex, prenderer);
    }
}

void doRenderCycle(SDL_Window* pwindow, SDL_Renderer * prenderer, SDL_Rect * ratioPreservedSize){
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

        // load texture if not already loaded
        loadCurrentTexture(prenderer);

        while(SDL_PollEvent(&event)){

            if((event.type == SDL_QUIT) ||
               (event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_ESCAPE)){
                running = 0;
            }
            if(event.type == SDL_WINDOWEVENT && event.window.event == SDL_WINDOWEVENT_RESIZED){
                SDL_GetWindowSize(pwindow,&w,&h);
                calculateImageRatio(w,h,ratioPreservedSize, imageTextures[currentIndex]);
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
                    decrementIndex();
                }
                if(event.key.keysym.sym == SDLK_RIGHT){
                    incrementIndex();
                }
                calculateImageRatio(w,h,ratioPreservedSize, imageTextures[currentIndex]);
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

        // SDL_RenderClear(prenderer);
        // SDL_RenderCopy(prenderer, ptexture, NULL, &scaledRect);
        // doRenderHUD(prenderer, ptexture, w, h, 50);
        // SDL_RenderPresent(prenderer);
        SDL_SetRenderDrawColor(prenderer,0x00,0x00,0x00,0xff);
        SDL_RenderClear(prenderer);
        SDL_RenderCopy(prenderer, imageTextures[currentIndex], NULL, &scaledRect);
        doRenderHUD(prenderer, imageTextures[currentIndex], w, h, 50);
        SDL_SetRenderDrawColor(prenderer,0xff,0xff,0xff,0xff);
        SDL_RenderPresent(prenderer);
    }
}

void setupImageList(char* imageFile) {

    printf("Selected file: %s\n", imageFile);
    
    images = getImagesInDirectory(imageFile, &imageCount);
    for(int k = 0; k < imageCount; k++){
        if(strcmp(images[k], imageFile) == 0){
            currentIndex = k;
            break;
        }
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
    loadCurrentTexture(prenderer);
    
    printf("Cached images count: %d\n", imageCount);

    // render cycle
    SDL_Rect ratioPreservedSize;
    calculateImageRatio(w,h, &ratioPreservedSize, imageTextures[currentIndex]);
    doRenderCycle(pwindow, prenderer, &ratioPreservedSize);
    // clean up
    for(int l = 0; l<imageCount;l++){
        free(images[l]);
    }
    for (int m = 0; m<imageCount;m++){
        if(imageTextures[m]!=NULL){
            SDL_DestroyTexture(imageTextures[m]);
        }
    }

    free(images);
    free(imageTextures);
    SDL_DestroyRenderer(prenderer);
    SDL_DestroyWindow(pwindow);
    SDL_Quit();
    return 0;
}
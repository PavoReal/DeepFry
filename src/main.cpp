#include <stdio.h>
#include <stdint.h>
#include <assert.h>
#include <math.h>
#define SDL_MAIN_HANDLED

#include "imgui.h"
#include "imgui_impl_sdl.h"
#include "imgui_impl_opengl3.h"

#include <SDL.h>
#include <GL/gl3w.h>
#include <GL/gl3w.c>

#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#define STB_IMAGE_RESIZE_IMPLEMENTATION

#include "stb_image.h"
#include "stb_image_write.h"
#include "stb_image_resize.h"

#define UNUSED(a) (void) a
#define MIN(a,b) ((a) < (b) ? (a) : (b))
#define MAX(a,b) ((a) > (b) ? (a) : (b))

typedef int8_t  s8;
typedef int16_t s16;
typedef int32_t s32;
typedef int64_t s64;

typedef unsigned int uint;
typedef uint8_t  u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

struct v3
{
    union
    {
        struct
        {
            u8 x;
            u8 y;
            u8 z;
        };

        struct
        {
            u8 r;
            u8 g;
            u8 b;
        };
    };
};

inline v3
V3(u8 x, u8 y, u8 z)
{
    v3 result = {x, y, z};

    return result;
}

inline GLuint
GenGLTexture(int width, int height, u8 *data)
{
    GLuint result;

    glGenTextures(1, &result);
    glBindTexture(GL_TEXTURE_2D, result);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);

    return result;
}

inline void
DeleteGLTexture(GLuint id)
{
    glDeleteTextures(1, &id);
}

inline unsigned char*
LoadTextureFromFile(char* filename, GLuint *textureID, int *width, int *height)
{
    unsigned char *data = stbi_load(filename, width, height, NULL, 4);

    if (data == NULL)
    {
        return 0;
    }

    DeleteGLTexture(*textureID);
   *textureID = GenGLTexture(*width, *height, data);

    return data;
}

struct DeepFryEffects
{
    v3 black;
    v3 white;

    v3 colors;

    int downSampleCount;
    float downSampleMultiply;
    int jpegQuality;
};

inline DeepFryEffects
DefaultDeepFry()
{
    DeepFryEffects result = {};

    result.black = V3(10, 10, 10);
    result.white = V3(245, 245, 245);

    result.colors = V3(255, 255, 255);

    result.downSampleCount    = 0;
    result.downSampleMultiply = 0.5;

    result.jpegQuality = 6;

    return result;
}

struct JPEGWriteContext
{
    u8 *dest;
};

void 
JPEGWriteToBuffer(void *context, void *data, int size)
{
    JPEGWriteContext *info = (JPEGWriteContext*) context;

    u8 *dest = info->dest;
    u8 *src  = (u8*) data;

    for (int i = 0; i < size; ++i)
    {
        *dest++ = *src++;
    }

    info->dest = dest;
}

inline u8*
DeepFry(unsigned char *input, int width, int height, DeepFryEffects *flags)
{
    size_t bufferSize = width *  height * 4;

    u8* result    = (u8*) malloc((size_t) (bufferSize * 2.1));
    u8* resizeBuf = result + bufferSize;

    u8 *srcIndex  = input;
    u8 *destIndex = result;

    for (u64 i = 0; i < (width * height); ++i)
    {
        u8 red   = *srcIndex++;
        u8 green = *srcIndex++;
        u8 blue  = *srcIndex++;
        u8 alpha = *srcIndex++;

        bool isBlack = (red <= flags->black.r) && (green <= flags->black.g) && (blue <= flags->black.b);
        bool isWhite = (red >= flags->white.r) && (green >= flags->white.g) && (blue >= flags->white.b);

        if (isBlack)
        {
            red   = 0x00;
            green = 0x00;
            blue  = 0x00;
        }
        else if (isWhite)
        {
            red   = 0xff;
            green = 0xff;
            blue  = 0xff;
        }
        else
        {
            bool isRed   = (red   >= flags->colors.r);
            bool isGreen = (green >= flags->colors.g);
            bool isBlue  = (blue  >= flags->colors.b);

            if (isRed)
            {
                red = 0xff;
            }

            if (isGreen)
            {
                green = 0xff;
            }

            if (isBlue)
            {
                blue = 0xff;
            }
        }

        *destIndex++ = red;
        *destIndex++ = green;
        *destIndex++ = blue;
        *destIndex++ = alpha;
    }

    for (int i = 0; i < flags->downSampleCount; ++i)
    {
        stbir_resize_uint8(result, width, height, 0,
                            resizeBuf,
                            (int) (width * flags->downSampleMultiply), 
                            (int) (height * flags->downSampleMultiply), 0, 4);

        stbir_resize_uint8(resizeBuf, 
            (int) (width * flags->downSampleMultiply), 
            (int) (height * flags->downSampleMultiply), 
            0, result, width, height, 0, 4);
    }

    JPEGWriteContext context;
    context.dest = resizeBuf;

    int w;
    int h;

    stbi_write_jpg_to_func(JPEGWriteToBuffer, &context, width, height, 4, result, flags->jpegQuality);
    u8 *jpegImage = stbi_load_from_memory(resizeBuf, (int)(context.dest - resizeBuf), &w, &h, 0, 4);

    memcpy(result, jpegImage, (width * height * 4));

    stbi_image_free(jpegImage);

    return result;
}

int
main()
{
    SDL_SetMainReady();

      // Setup SDL
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER | SDL_INIT_GAMECONTROLLER) != 0)
    {
        printf("Error: %s\n", SDL_GetError());
        exit(1);
    }

    // GL 3.0 + GLSL 130
    const char* glsl_version = "#version 130";
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, 0);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);

    // Create window with graphics context
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
    SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);
    SDL_WindowFlags window_flags = (SDL_WindowFlags)(SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI);
    SDL_Window* window = SDL_CreateWindow("Peacock's Deepfryer", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 1280, 720, window_flags);
    SDL_GLContext gl_context = SDL_GL_CreateContext(window);
    SDL_GL_MakeCurrent(window, gl_context);
    SDL_GL_SetSwapInterval(1); // Enable vsync

    // Initialize OpenGL loader
    bool err = gl3wInit() != 0;
    if (err)
    {
        fprintf(stderr, "Failed to initialize OpenGL loader!\n");
        exit(1);
    }

    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;

    // Setup Dear ImGui style
    //ImGui::StyleColorsDark();
    ImGui::StyleColorsClassic();
    ImGui::SetColorEditOptions(ImGuiColorEditFlags__OptionsDefault | ImGuiColorEditFlags_NoAlpha);

    // Setup Platform/Renderer bindings
    ImGui_ImplSDL2_InitForOpenGL(window, gl_context);
    ImGui_ImplOpenGL3_Init(glsl_version);

    float srcScale  = 0.5f;
    char srcImgName[1024];
    char destImgName[1024];
    char errorImgName[1024];

    GLuint srcID = 0;
    int srcWidth = 0;
    int srcHeight = 0;
    unsigned char *srcData = 0;

    bool showError = false;
    bool showDest  = false;

    DeepFryEffects effects = DefaultDeepFry();

    GLuint destID = 0;
    u8 *destData = 0;
    float destScale = srcScale;

    strcpy(srcImgName, "FILE NAME (WITH EXTENSION) HERE");

#define FRAME_RATE 30
#define TICKS_FOR_NEXT_FRAME (1000 / FRAME_RATE)

    // Main loop
    bool done = false;
    int lastTime = 0;
    while (!done)
    {
        while (lastTime - SDL_GetTicks() < TICKS_FOR_NEXT_FRAME) 
        {
            SDL_Delay(1);
        }

        SDL_Event event;
        while (SDL_PollEvent(&event))
        {
            ImGui_ImplSDL2_ProcessEvent(&event);
            if (event.type == SDL_QUIT)
            {
                done = true;
            }
            if (event.type == SDL_WINDOWEVENT && event.window.event == SDL_WINDOWEVENT_CLOSE 
                                              && event.window.windowID == SDL_GetWindowID(window))
            {
                done = true;
            }
        }

        // Start the Dear ImGui frame
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplSDL2_NewFrame(window);
        ImGui::NewFrame();

        // #if defined(DEBUG)
        // ImGui::Begin("Deep fry info");
        // ImGui::Text("Black point: \n\tR: %d\n\tG: %d\n\tB: %d", blackPoint.r, blackPoint.g, blackPoint.b);
        // ImGui::End();
        // #endif

        // 2. Show a simple window that we create ourselves. We use a Begin/End pair to created a named window.
        ImGui::Begin("Source image");

        if (showError)
        {
            ImGui::Text("Could not load %s...", errorImgName);
        }

        ImGui::Text("Filename:");
        ImGui::SameLine();
        ImGui::InputText("", srcImgName, sizeof(srcImgName));
        ImGui::SameLine();

        if (ImGui::Button("Load"))
        {
            unsigned char *newSrcData = LoadTextureFromFile(srcImgName, &srcID, &srcWidth, &srcHeight);
            
            if (newSrcData)
            {
                showError = false;
                showDest = true;
                stbi_image_free(srcData);
                free(destData);

                srcData = newSrcData;
                destData = DeepFry(srcData, srcWidth, srcHeight, &effects);

                DeleteGLTexture(destID);
                destID = GenGLTexture(srcWidth, srcHeight, destData);

                sprintf(destImgName, "deepfried_%s.jpg", srcImgName);
            }
            else
            {
                showError = true;
                strcpy(errorImgName, srcImgName);
            }
        }

        ImGui::SliderFloat("Image scale (Viewport only)", &srcScale, 0.1f, 2.0f);
        if (srcID)
        {
            ImGui::Image((void*)(intptr_t)srcID, ImVec2((float) srcWidth * srcScale, (float) srcHeight * srcScale));
        }

        ImGui::End();

        if (showDest)
        {
            ImGui::Begin("Deep fried settings");

            ImGui::Text("Filename:");
            ImGui::SameLine();
            ImGui::InputText("", destImgName, sizeof(destImgName));
            if (ImGui::Button("Save"))
            {
                stbi_write_jpg(destImgName, srcWidth, srcHeight, 4, destData, 50);
            }

            bool shouldRedraw = false;

            static float colors[3] = {1, 1, 1};
            if (ImGui::ColorPicker3("Color points", colors))
            {
                shouldRedraw = true;
                effects.colors = V3(
                    (u8) (colors[0] * 255), 
                    (u8) (colors[1] * 255), 
                    (u8) (colors[2] * 255));
            }

            static float blackColors[3] = {};
            if (ImGui::ColorEdit3("Black point", blackColors))
            {
                shouldRedraw = true;
                effects.black = V3(
                    (u8) (blackColors[0] * 255), 
                    (u8) (blackColors[1] * 255), 
                    (u8) (blackColors[2] * 255));
            }

            static float whiteColors[3] = {1, 1, 1};
            if (ImGui::ColorEdit3("White point", whiteColors))
            {
                shouldRedraw = true;
                effects.white = V3(
                    (u8) (whiteColors[0] * 255), 
                    (u8) (whiteColors[1] * 255), 
                    (u8) (whiteColors[2] * 255));
            }

            shouldRedraw |= ImGui::SliderInt("JPEG Quality (Lower is worse)", &effects.jpegQuality, 1, 100);

            shouldRedraw |= ImGui::SliderFloat("Downsample multiply (Lower is worse)", &effects.downSampleMultiply, 0.01f, 0.5f);
            shouldRedraw |= ImGui::SliderInt("Downsample iterations", &effects.downSampleCount, 0, 100);
            ImGui::SameLine();
            ImGui::Text("(Higher values will be very slow)");

            if (shouldRedraw)
            {
                free(destData);
                destData = DeepFry(srcData, srcWidth, srcHeight, &effects);

                DeleteGLTexture(destID);
                destID = GenGLTexture(srcWidth, srcHeight, destData);
            }

            ImGui::End();

            ImGui::Begin("Image preview");

            ImGui::SliderFloat("Image scale", &destScale, 0.1f, 2.0f);
            ImGui::SameLine();
            ImGui::Text("(Viewport only)");

            if (destID)
            {
                ImGui::Image((void*)(intptr_t)destID, ImVec2((float) srcWidth * destScale, (float) srcHeight * destScale));
            }

            ImGui::End();
        }

        // Rendering
        ImGui::Render();
        glViewport(0, 0, (int)io.DisplaySize.x, (int)io.DisplaySize.y);
        glClearColor(0, 0, 0, 1);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        SDL_GL_SwapWindow(window);
        lastTime = SDL_GetTicks();
    }

    // Cleanup
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplSDL2_Shutdown();
    ImGui::DestroyContext();

    SDL_GL_DeleteContext(gl_context);
    SDL_DestroyWindow(window);
    SDL_Quit();

	return 0;
}

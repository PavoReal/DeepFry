#include <stdio.h>
#include <stdint.h>
#include <assert.h>
#define SDL_MAIN_HANDLED

#include "imgui.h"
#include "imgui_impl_sdl.h"
#include "imgui_impl_opengl3.h"

#include <SDL.h>
#include <GL/gl3w.h>
#include <GL/gl3w.c>

#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION

#include "stb_image.h"
#include "stb_image_write.h"

#define UNUSED(a) (void) a

typedef int8_t  s8;
typedef int16_t s16;
typedef int32_t s32;
typedef int64_t s64;

typedef unsigned int uint;
typedef uint8_t  u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

inline GLuint
GenGLTexture(int width, int height, u8 *data)
{
    GLuint result;

    glGenTextures(1, &result);
    glBindTexture(GL_TEXTURE_2D, result);

    // Setup filtering parameters for display
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    // Upload pixels into texture
    glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);

    return result;
}

inline unsigned char*
LoadTextureFromFile(char* filename, GLuint *textureID, int *width, int *height)
{
    unsigned char *data = stbi_load(filename, width, height, NULL, 4);

    if (data == NULL)
    {
        return 0;
    }

   *textureID = GenGLTexture(*width, *height, data);

    return data;
}

inline u8*
DeepFry(unsigned char *input, int width, int height, int a, int b, int c, int d)
{
    u8* result = (u8*) malloc(width * height * 4);

    u8 *srcIndex  = input;
    u8 *destIndex = result;

    double avgLum   = 69;
    double lumCount = 0;
    for (u64 i = 0; i < (width * height); ++i)
    {
        u8 red   = *srcIndex++;
        u8 green = *srcIndex++;
        u8 blue  = *srcIndex++;
        u8 alpha = *srcIndex++;

        double lum = (red * 0.2126f) * (green * 0.7152f) * (blue * 0.0722f);
        avgLum   += lum;
        lumCount += 1;

        double aa        = avgLum / lumCount;
        double contrast = (aa - lum) / aa;

        *destIndex++ = (u8) (((red + blue) / 2) * contrast) & (u32) a; // Red
        *destIndex++ = (u8) ((green + red)      * contrast) & (u32) b;   // Green
        *destIndex++ = (u8) ((blue + green)     * contrast) & (u32) c;  // Blue
        *destIndex++ = (u8) (alpha * contrast) & (u32) d; // Alpha
    }

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

    bool eA = true; // Sports
    bool eB = true;
    bool eC = true;
    bool eD = false;

    int yA = INT_MAX / 4;
    int yB = INT_MAX / 4; 
    int yC = INT_MAX / 4;
    int yD = INT_MAX / 4;

    GLuint destID = 0;
    u8 *destData = 0;
    float destScale = srcScale;

    strcpy(srcImgName, "FILE NAME (WITH EXTENSION) HERE");

    // Main loop
    bool done = false;
    while (!done)
    {
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

        // 2. Show a simple window that we create ourselves. We use a Begin/End pair to created a named window.
        {
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
                    destData = DeepFry(srcData, srcWidth, srcHeight, 
                        (eA ? yA : ~0),
                        (eB ? yB : ~0),
                        (eC ? yC : ~0),
                        (eD ? yD : ~0));

                    destID = GenGLTexture(srcWidth, srcHeight, destData);

                    sprintf(destImgName, "deepfried_%s.png", srcImgName);
                }
                else
                {
                    showError = true;
                    strcpy(errorImgName, srcImgName);
                }
            }

            ImGui::SliderFloat("Scale", &srcScale, 0.0f, 1.0f);
            if (srcID)
            {
                ImGui::Image((void*)(intptr_t)srcID, ImVec2((float) srcWidth * srcScale, (float) srcHeight * srcScale));
            }

            ImGui::End();

            if (showDest)
            {
                ImGui::Begin("Deep fryed image");

                ImGui::Text("Filename:");
                ImGui::SameLine();
                ImGui::InputText("", destImgName, sizeof(destImgName));
                if (ImGui::Button("Save"))
                {
                    stbi_write_png(destImgName, srcWidth, srcHeight, 4, destData, srcWidth * 4);
                }

                bool changeA = ImGui::SliderInt("Retarded red",   &yA,  0, INT_MAX / 2);
                ImGui::SameLine();
                ImGui::Checkbox("Enabled", &eA);

                bool changeB = ImGui::SliderInt("Retarded green", &yB,  0, INT_MAX / 2);
                ImGui::SameLine();
                ImGui::Checkbox("Enabled", &eB);

                bool changeC = ImGui::SliderInt("Retarded blue",  &yC,  0, INT_MAX / 2);
                ImGui::SameLine();
                ImGui::Checkbox("Enabled", &eC);

                bool changeD = ImGui::SliderInt("Retarded alpha", &yD,  0, INT_MAX / 2);
                ImGui::SameLine();
                ImGui::Checkbox("Enabled", &eD);

                if (changeA || changeB || changeC || changeD)
                {
                    free(destData);
                    destData = DeepFry(srcData, srcWidth, srcHeight, 
                        (eA ? yA : ~0),
                        (eB ? yB : ~0),
                        (eC ? yC : ~0),
                        (eD ? yD : ~0));
                    destID   = GenGLTexture(srcWidth, srcHeight, destData);
                }

                ImGui::SliderFloat("Scale", &destScale, 0.0f, 1.0f);
                if (destID)
                {
                    ImGui::Image((void*)(intptr_t)destID, ImVec2((float) srcWidth * destScale, (float) srcHeight * destScale));
                }

                ImGui::End();
            }
        }

        // Rendering
        ImGui::Render();
        glViewport(0, 0, (int)io.DisplaySize.x, (int)io.DisplaySize.y);
        glClearColor(0, 0, 0, 1);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        SDL_GL_SwapWindow(window);
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

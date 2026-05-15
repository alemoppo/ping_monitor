#include <SDL.h>
#include <SDL_opengl.h>
#include "imgui.h"
#include "imgui_impl_sdl2.h"
#include "imgui_impl_opengl3.h"
#include "app.h"
#include "config.h"
#include "icon_data.h"
#include <stdio.h>

#include <stdlib.h>

#ifdef _WIN32
#include <windows.h>
#endif

int main(int argc, char* argv[]) {
    (void)argc;
    (void)argv;

    AppConfig cfg;
    config_load(&cfg);

    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        fprintf(stderr, "SDL_Init failed: %s\n", SDL_GetError());
        return 1;
    }

    const char* glsl_version = "#version 130";
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, 0);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);

    SDL_Window* window = SDL_CreateWindow(
        "Ping Monitor",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        1200, 900,
        SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI
    );
    if (!window) {
        fprintf(stderr, "SDL_CreateWindow failed: %s\n", SDL_GetError());
        SDL_Quit();
        return 1;
    }

    SDL_Surface* icon = SDL_CreateRGBSurfaceFrom((void*)icon_pixels,
        ICON_WIDTH, ICON_HEIGHT, 32, ICON_WIDTH * 4,
        0x000000ff, 0x0000ff00, 0x00ff0000, 0xff000000);
    if (icon) {
        SDL_SetWindowIcon(window, icon);
        SDL_FreeSurface(icon);
    }

    SDL_GLContext gl_context = SDL_GL_CreateContext(window);
    SDL_GL_MakeCurrent(window, gl_context);
    SDL_GL_SetSwapInterval(1);

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.FontGlobalScale = 1.0f;

    if (cfg.dark_mode) {
        ImGui::StyleColorsDark();
    } else {
        ImGui::StyleColorsLight();
    }

    ImGuiStyle& style = ImGui::GetStyle();
    style.WindowRounding = 0.0f;
    style.ChildRounding = 0.0f;
    style.FrameRounding = 0.0f;
    style.GrabRounding = 0.0f;
    style.PopupRounding = 0.0f;
    style.TabRounding = 0.0f;
    style.WindowPadding = ImVec2(10, 10);
    style.FramePadding = ImVec2(6, 4);
    style.ItemSpacing = ImVec2(8, 4);

    ImGui_ImplSDL2_InitForOpenGL(window, gl_context);
    ImGui_ImplOpenGL3_Init(glsl_version);

    ImVec4 clear_color = cfg.dark_mode
        ? ImVec4(0.15f, 0.15f, 0.18f, 1.0f)
        : ImVec4(0.85f, 0.85f, 0.88f, 1.0f);

    App* app = (App*)calloc(1, sizeof(App));
    if (!app) {
        fprintf(stderr, "Failed to allocate memory\n");
        SDL_Quit();
        return 1;
    }
    app_init(app, &cfg);
    app->window = window;

    int prev_dark = app->dark_mode;
    int quit = 0;
    while (!quit) {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            ImGui_ImplSDL2_ProcessEvent(&event);
            if (event.type == SDL_QUIT) quit = 1;
            if (event.type == SDL_WINDOWEVENT &&
                event.window.event == SDL_WINDOWEVENT_CLOSE &&
                event.window.windowID == SDL_GetWindowID(window))
                quit = 1;
        }

        if (app->dark_mode != prev_dark) {
            if (app->dark_mode) {
                ImGui::StyleColorsDark();
                clear_color = ImVec4(0.15f, 0.15f, 0.18f, 1.0f);
            } else {
                ImGui::StyleColorsLight();
                clear_color = ImVec4(0.85f, 0.85f, 0.88f, 1.0f);
            }
            style = ImGui::GetStyle();
            style.WindowRounding = 0.0f;
            style.ChildRounding = 0.0f;
            style.FrameRounding = 0.0f;
            style.GrabRounding = 0.0f;
            style.PopupRounding = 0.0f;
            style.TabRounding = 0.0f;
            style.WindowPadding = ImVec2(10, 10);
            style.FramePadding = ImVec2(6, 4);
            style.ItemSpacing = ImVec2(8, 4);
            prev_dark = app->dark_mode;
        }

        glViewport(0, 0, (int)io.DisplaySize.x, (int)io.DisplaySize.y);
        glClearColor(clear_color.x * clear_color.w, clear_color.y * clear_color.w,
                     clear_color.z * clear_color.w, clear_color.w);
        glClear(GL_COLOR_BUFFER_BIT);

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplSDL2_NewFrame();
        ImGui::NewFrame();

        app_render(app);

        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        SDL_GL_SwapWindow(window);
    }

    app_shutdown(app);

    AppConfig save_cfg;
    save_cfg.target_count = app->target_count;
    for (int i = 0; i < app->target_count; i++) {
        strncpy(save_cfg.targets[i].ip, app->targets[i].ip, 127);
        save_cfg.targets[i].ip[127] = '\0';
        save_cfg.targets[i].interval = app->targets[i].interval;
    }
    save_cfg.max_points = app->max_points;
    save_cfg.rolling_avg = app->rolling_avg;
    save_cfg.dark_mode = app->dark_mode;
    save_cfg.use_mean = app->use_mean;
    config_save(&save_cfg);

    free(app);

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplSDL2_Shutdown();
    ImGui::DestroyContext();

    SDL_GL_DeleteContext(gl_context);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}

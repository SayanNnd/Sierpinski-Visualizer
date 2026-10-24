#include <algorithm>
#include <array>
#include  <chrono>
#include <cmath>
#include <cstdint>
#include <iterator>
#include <vector>
#include "raylib.h"

static void reset_window(int &warmup, int &whole, double &rate, double &credit, uint64_t &points_counter, uint64_t &lastCount, bool &paused, bool &started, bool &firstTime, std::vector<Color> &points);
static Vector2 midpoint(const Vector2 &a, const Vector2 &b);
static uint64_t random_next(uint64_t &num);

constexpr float ScreenWidth = 1080;
constexpr float ScreenHeight = 1080;
#define FPS 120

int main() {
    //Window
    InitWindow(static_cast<int>(ScreenWidth), static_cast<int>(ScreenHeight), "Sierpinski Triangle");
    const Image img = GenImageColor(ScreenWidth, ScreenHeight, BLACK);
    Texture2D tex = LoadTextureFromImage(img);
    UnloadImage(img);
    SetTargetFPS(FPS);

    //Initializations
    constexpr double budget = 0.9/FPS;
    constexpr std::array<Vector2,3> corners = {
        Vector2{.x = ScreenWidth/2, .y = ScreenHeight/8},
        Vector2{.x = ScreenWidth/8, .y = ScreenHeight/8+(ScreenWidth*3/4)*0.8660254f},
        Vector2{.x = ScreenWidth*7/8, .y = ScreenHeight/8+(ScreenWidth*3/4)*0.8660254f}
    };
    constexpr size_t n = std::size(corners);
    Vector2 point;

    double credit = 0;
    double rate = 1;
    int whole = 0;
    double cost = 100e-9;
    int cap = static_cast<int>(budget/cost);
    int warmup = 20;
    uint64_t next = std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::system_clock::now().time_since_epoch()).count();;
    uint64_t points_counter = 0;
    uint64_t lastCount = 0;
    bool paused = false;
    bool started = false;
    bool firstTime = true;
    bool benchmark = false;
    double currTime = 0;
    double lastTime = 0;
    double draw_rate = 0;
    double drawTime = 0;
    std::vector<Color> points(ScreenHeight*ScreenWidth,BLACK);

    //Loop
    while (!WindowShouldClose()) {
        //Key Press
        if (IsKeyPressed(KEY_B)) {
            benchmark = !benchmark;
        }
        if (IsKeyPressed(KEY_SPACE)) {
            paused = !paused;
        }
        if (IsKeyPressed(KEY_R)) {
            reset_window(warmup, whole, rate, credit, points_counter, lastCount, paused, started, firstTime, ref(points));
        }
        if (!started && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
            point = GetMousePosition();
            next = random_next(next);
            started = true;
            lastTime = GetTime();
        }


        //Points Generation
        const double t0 = GetTime();
        if (!paused && started) {
            credit += rate*GetFrameTime();
            whole = static_cast<int>(credit);
            credit -= whole;
            whole = std::min(cap,whole);
            if (warmup) rate *= std::pow(1.005, GetFrameTime());
            else  if (rate*GetFrameTime() <= cap) rate *= std::pow(1.75, GetFrameTime());

            for (int i{0}; i<whole; i++) {
                point = midpoint(point, corners[next%3]);
                next = random_next(next);
                if (warmup>0) {
                    points_counter++;
                    warmup--;
                }
                else {
                    const int index = static_cast<int>(point.y)*static_cast<int>(ScreenWidth)+static_cast<int>(point.x);
                    points[index] = RED;
                    points_counter++;
                }
            }

            const double calcTime = GetTime();
            if (whole>1000) {
                const double TimeForOP= (calcTime-t0)/whole;
                cost = 0.9*cost + 0.1*TimeForOP;
            }
            cap=static_cast<int>((budget-drawTime)/cost);
        }
        const double drawStart = GetTime();
        UpdateTexture(tex, points.data()); //Texture Update
        currTime = GetTime();
        drawTime = currTime-drawStart;
        if (currTime - lastTime >= 0.5) {
            draw_rate = static_cast<double>(points_counter-lastCount)/(currTime-lastTime);
            lastCount = points_counter;
            lastTime = currTime;
        }

        //Draw
        BeginDrawing();
            ClearBackground(BLACK);
            const Rectangle src = { .x = 0, .y = 0, .width = static_cast<float>(tex.width), .height = static_cast<float>(tex.height) };
            DrawTextureRec(tex, src, {.x = 0, .y = 0}, WHITE);
            //Triangle
            for (int curr{0}; curr<static_cast<int>(n); curr++) {
                DrawLineV(corners[curr], corners[(curr+1)%n], DARKGRAY);
            }
            //Warmup-Phase Temporary Drawing
            if (started && warmup >= 0 && rate <= 1e5) {
                const Vector2 target = corners[next%3];
                DrawLineV(point, target, BLUE);
                DrawCircleV(point, 3, YELLOW);
                DrawCircleV(midpoint(point, target), 5, GREEN);
            }
            if (benchmark) {
                DrawFPS(10, 10);
                DrawText(TextFormat("Points :- %.2fM",static_cast<double>(points_counter)/1e6),10, 40, 20, GREEN);
                DrawText(TextFormat("Rate :- %.2fM Points/Second",draw_rate/1e6),10, 70, 20, GREEN);
            }
            DrawText(TextFormat("Press \"R\" Key to Reset the Screen"),10, ScreenHeight-60, 15, RED);
            DrawText(TextFormat("Press \"B\" Key to Enable/Disable Benchmark Mode"),10, ScreenHeight-40, 15, GREEN);
            DrawText(TextFormat("Press \"SPACE\" Key to Pause/Unpause the Screen"),10, ScreenHeight-20, 15, YELLOW);
        EndDrawing();
    }
    UnloadTexture(tex);
    CloseWindow();
    return 0;
}

uint64_t random_next(uint64_t &num) {
    num ^= num<<13;
    num ^= num>>7;
    num ^= num<<17;
    return num;
}

Vector2 midpoint(const Vector2 &a, const Vector2 &b) {
    return {.x = (a.x+b.x)/2, .y = (a.y+b.y)/2};
}

void reset_window(int &warmup, int &whole,double &rate, double &credit, uint64_t &points_counter, uint64_t &lastCount, bool &paused, bool &started, bool &firstTime, std::vector<Color> &points) {
    //Re-Initialization
    warmup = 20;
    points_counter = 0;
    lastCount = 0;
    rate = 1;
    credit = 0;
    whole = 0;
    paused = false;
    started = false;
    firstTime = true;

    //Reset Canvas
    std::fill(points.begin(), points.end(), BLACK);
}
#include <algorithm>
#include <array>
#include  <chrono>
#include <cmath>
#include <cstdint>
#include <iterator>
#include <vector>
#include <thread>
#include "raylib.h"

#define FPS 120
constexpr float ScreenWidth = 1080;
constexpr float ScreenHeight = 1080;
constexpr double budget = 0.9/FPS;
constexpr size_t pixelCount = ScreenWidth*ScreenHeight;
size_t thread_count = std::max<size_t>(1,std::thread::hardware_concurrency()/2);
namespace {
    struct Worker {
        Vector2  point{};
        uint64_t rng = 1;
        mutable uint64_t done = 0;
        std::vector<uint8_t> flags;
    };
}

static void reset_window(int &warmup, int &whole, double &rate, double &credit, uint64_t &points_counter, uint64_t &lastCount, bool &paused, bool &started,
    bool &workersReady,bool &firstTime, std::vector<Color> &points, std::vector<Worker> &thread_details);
static void worker(Worker &details, const std::vector<Vector2> &corners, int steps);
static void merge(const std::vector<Worker> &ws, std::vector<Color> &points, uint64_t &points_counter);
static Vector2 midpoint(const Vector2 &a, const Vector2 &b);
static uint64_t random_next(uint64_t &num);
static uint64_t splitmix64(uint64_t &state);

int main(int argc, char** argv) {
    for (int i = 1; i < argc; i++) {
        const std::string a = argv[i];
        if ((a == "--threads" || a == "-t") && i+1 < argc) {
            const long v = std::strtol(argv[++i], nullptr, 10);
            if (v < 1 || v > static_cast<long>(std::thread::hardware_concurrency())) {
                std::fprintf(stderr, "threads must be between 1 and %zu\n", std::thread::hardware_concurrency());
                return 1;
            }
            thread_count = static_cast<size_t>(v);
        } else {
            std::fprintf(stderr, "usage: %s [--threads N]\n", argv[0]);
            return 1;
        }
    }

    //Window
    InitWindow(static_cast<int>(ScreenWidth), static_cast<int>(ScreenHeight), "Sierpinski Triangle");
    const Image img = GenImageColor(ScreenWidth, ScreenHeight, BLACK);
    const Texture2D tex = LoadTextureFromImage(img);
    UnloadImage(img);
    SetTargetFPS(FPS);

    //Initializations
    const std::vector<Vector2> corners = {
        Vector2{.x = ScreenWidth/2, .y = ScreenHeight/8},
        Vector2{.x = ScreenWidth/8, .y = ScreenHeight/8+(ScreenWidth*3/4)*0.8660254f},
        Vector2{.x = ScreenWidth*7/8, .y = ScreenHeight/8+(ScreenWidth*3/4)*0.8660254f}
    };
    const size_t n = corners.size();
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
    std::vector<Color> points(pixelCount,BLACK);
    std::vector<Worker> threads_details (thread_count-1);
    for (auto &details : threads_details) details.flags.assign(pixelCount,0);
    bool workersReady = false;
    double at = 0;
    double bt = 0;
    double ct = 0;

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
            reset_window(warmup, whole, rate, credit, points_counter, lastCount,
                paused, started, workersReady, firstTime, std::ref(points), std::ref(threads_details));
        }
        if (!started && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
            point = GetMousePosition();
            next = random_next(next);
            started = true;
            lastTime = GetTime();
        }

        //Generation of Thread Details
        if (warmup==0 && !workersReady) {
            uint64_t s = next^0x9E3779B97F4A7C15ull;
            for (auto& details : threads_details) {
                details.point = point;
                details.rng = splitmix64(s);
                if (details.rng==0) details.rng=1;
            }
            workersReady = true;
        }

        //Points Generation
        const double t0 = GetTime();
        if (!paused && started) {
            //Rate Limiter
            credit += rate*GetFrameTime();
            whole = static_cast<int>(credit);
            credit -= whole;
            whole = std::min(cap,whole);
            if (warmup) rate *= std::pow(1.005, GetFrameTime());
            else  if (rate*GetFrameTime() <= cap) rate *= std::pow(1.75, GetFrameTime());

            //Actual Work
            if (warmup==0 && whole>=1000) {               //Threaded Work
                const int baseCount = whole/static_cast<int>(thread_count);
                const int mainCount = whole-static_cast<int>(baseCount*(thread_count-1));
                std::vector<std::thread> threads;
                for (auto &details : threads_details) {
                    threads.emplace_back(worker, std::ref(details), std::ref(corners), baseCount);
                }
                for (int i{0}; i<mainCount; i++) {
                    point = midpoint(point, corners[next%3]);
                    next = random_next(next);
                    const int index = static_cast<int>(point.y)*static_cast<int>(ScreenWidth)+static_cast<int>(point.x);
                    points[index] = RED;
                    points_counter++;
                }
                at = GetTime();
                for (auto& t : threads) t.join();
                bt = GetTime();
                merge(threads_details, points, points_counter);
                ct = GetTime();
            }
            else {                                        //Standard Work
                for (int i{0}; i<whole; i++) {
                    point = midpoint(point, corners[next%3]);
                    next = random_next(next);
                    if (warmup>0) {
                        warmup--;
                    }
                    else {
                        const int index = static_cast<int>(point.y)*static_cast<int>(ScreenWidth)+static_cast<int>(point.x);
                        points[index] = RED;
                    }
                    points_counter++;
                }
            }

            //Rate Limiter
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

        // Rate Limiter for Points/Second Counter
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
            if (started && warmup >= 0 && rate <= 100) {
                const Vector2 target = corners[next%3];
                DrawLineV(point, target, BLUE);
                DrawCircleV(point, 3, YELLOW);
                DrawCircleV(midpoint(point, target), 5, GREEN);
            }
            if (benchmark) {
                DrawFPS(10, 10);
                DrawText(TextFormat("Points :- %.2fM",static_cast<double>(points_counter)/1e6),10, 40, 20, GREEN);
                DrawText(TextFormat("Rate :- %.2fM Points/Second",draw_rate/1e6),10, 70, 20, GREEN);
                DrawText(TextFormat("cap %d  whole %d  cost %.1f ns  join %.2f ms  merge %.2f ms  draw %.2f ms",
                    cap, whole, cost*1e9, (bt-at)*1e3, (ct-bt)*1e3, drawTime*1e3), 10, 100, 20, WHITE);
            }
            DrawText(TextFormat("threads %d", static_cast<int>(thread_count)), 10, 130, 20, WHITE);
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

uint64_t splitmix64(uint64_t &state) {
    uint64_t z = (state += 0x9E3779B97F4A7C15ull);
    z = (z ^ (z >> 30)) * 0xBF58476D1CE4E5B9ull;
    z = (z ^ (z >> 27)) * 0x94D049BB133111EBull;
    return z ^ (z >> 31);
}

Vector2 midpoint(const Vector2 &a, const Vector2 &b) {
    return {.x = (a.x+b.x)/2, .y = (a.y+b.y)/2};
}

void reset_window(int &warmup, int &whole,double &rate, double &credit, uint64_t &points_counter, uint64_t &lastCount, bool &paused, bool &started,
    bool &workersReady, bool &firstTime, std::vector<Color> &points, std::vector<Worker> &thread_details) {
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
    workersReady = false;
    thread_details.clear();

    //Reset Canvas
    std::fill(points.begin(), points.end(), BLACK);
}

void worker(Worker &details, const std::vector<Vector2> &corners, const int steps) {
    Vector2 p = details.point;
    uint64_t rng = details.rng;
    for (int i{0}; i<steps; i++) {
        p=midpoint(p, corners[random_next(rng)%3]);
        const int ix = static_cast<int>(p.x), iy = static_cast<int>(p.y);
        if (ix < static_cast<int>(ScreenWidth) && iy < static_cast<int>(ScreenHeight)) details.flags[iy*static_cast<int>(ScreenWidth)+ix] = 1;
    }
    details.point = p;
    details.rng = rng;
    details.done+=steps;
}

void merge(const std::vector<Worker> &ws, std::vector<Color> &points, uint64_t &points_counter) {
    for (auto &w : ws) {
        points_counter += w.done;
        w.done = 0;
    }
    auto slice = [&](const size_t lo, const size_t hi) {
        for (auto &w : ws) {
            for (size_t i = lo; i < hi; i++) if (w.flags[i]) points[i] = RED;
        }
    };
    std::vector<std::thread> ts;
    for (size_t t = 1; t < thread_count; t++)
        ts.emplace_back(slice, t * pixelCount / thread_count, (t + 1) * pixelCount / thread_count);
    slice(0, pixelCount / thread_count);
    for (auto &t : ts) t.join();
}